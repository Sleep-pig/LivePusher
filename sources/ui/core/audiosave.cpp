#include "audiosave.hpp"
#include <cstdint>
#include <cstring>
#include <qaudiodeviceinfo.h>
#include <qaudioformat.h>
#include <qaudioinput.h>
#include <qdebug.h>
#include <qfileinfo.h>
#include <qglobal.h>
#include <qmutex.h>
#include <qthread.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavcodec/codec.h"
#include "libavformat/avformat.h"
#include "libavutil/error.h"
#include "libavutil/frame.h"
#include "libswresample/swresample.h"
}

AudioSave::AudioSave(QObject *parent) : QObject(parent) {
    QAudioFormat out_format;
    out_format.setSampleRate(44100);
    out_format.setChannelCount(2);
    out_format.setSampleSize(16);
    out_format.setCodec("audio/pcm");
    out_format.setByteOrder(QAudioFormat::LittleEndian);
    out_format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(out_format)) {
        qDebug() << "Default format not supported - trying to use nearest";
        out_format = info.nearestFormat(out_format);
    }
    audioInput = new QAudioInput(out_format, this);

    // audioInput->setBufferSize(4096);
}

AudioSave::~AudioSave() {
    stopRecord();
}

bool AudioSave::open(QString const &filename) {
    if (filename.isEmpty()) {
        return false;
    }
    if (!initFFmpeg(filename)) {
        return false;
    }
    inputDevice = audioInput->start();
    return true;
}

void AudioSave::startRecord() {
    if (!isRecording) {
        isRecording = true;
    }
}

void AudioSave::stopRecord() {
    if (isRecording) {
        inputDevice->close();
        audioInput->stop();
        // flush();
        av_write_trailer(fmtCtx);
        cleanupFFmpeg();
        isRecording = false;
        return;
    }
}

bool AudioSave::initFFmpeg(QString const &filename) {
    int ret = avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr,
                                             filename.toStdString().c_str());
    if (ret < 0) {
        ShowError(ret);
        return false;
    }

    ret =
        avio_open(&fmtCtx->pb, filename.toStdString().data(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        ShowError(ret);
        return false;
    }

    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) {
        qDebug() << "find encoder failed";
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        qDebug() << "alloc context failed";
        return false;
    }

    codecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    codecCtx->sample_rate = 44100;
    codecCtx->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    // codecCtx->bit_rate = 128000;
    codecCtx->ch_layout.nb_channels = 2;
    codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(codecCtx, codec, nullptr);
    if (ret < 0) {
        ShowError(ret);
        return false;
    }

    AVStream *stream = avformat_new_stream(fmtCtx, nullptr);
    if (!stream) {
        qDebug() << "new stream failed";
        return false;
    }

    ret = avcodec_parameters_from_context(stream->codecpar, codecCtx);
    if (ret < 0) {
        ShowError(ret);
        return false;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        qDebug() << "alloc packet failed";
        return false;
    }

    AVChannelLayout out_ch_layout = codecCtx->ch_layout;
    AVChannelLayout in_ch_layout = AV_CHANNEL_LAYOUT_STEREO;

    ret = swr_alloc_set_opts2(&swrCtx, &out_ch_layout, codecCtx->sample_fmt,
                              codecCtx->sample_rate, &in_ch_layout,
                              AV_SAMPLE_FMT_S16, 44100, 0, nullptr);
    if (ret < 0 || !swrCtx) {
        ShowError(ret);
        swr_free(&swrCtx);
        return false;
    }
    swr_init(swrCtx);

    ret = avformat_write_header(fmtCtx, nullptr);
    if (ret < 0) {
        ShowError(ret);
        return false;
    }
    return true;
}

void AudioSave::cleanupFFmpeg() {
    count = 0;
    if (swrCtx) {
        swr_free(&swrCtx);
        swrCtx = nullptr;
    }
    if (fmtCtx) {
        avformat_free_context(fmtCtx);
        fmtCtx = nullptr;
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
    if (codec) {
        avcodec_close(codecCtx);
        codecCtx = nullptr;
    }
    if (swrFrame) {
        av_frame_free(&swrFrame);
        swrFrame = nullptr;
    }
    if (pkt) {
        av_packet_free(&pkt);
        pkt = nullptr;
    }
    isInitFFmpeg = false;
    delete [] out_buffer;
}

void AudioSave::ShowError(int err) {
    static char str[1024] = {0};
    av_strerror(err, str, sizeof(str));
    qDebug() << "error: " << str;
}

bool AudioSave::Swrconvert(char **buf) {
    int ret = swr_convert(swrCtx, swrFrame->data, swrFrame->nb_samples,
                          (uint8_t const **)buf, 1024);
    if (ret < 0) {
        ShowError(ret);
        return false;
    }
    return true;
}

char *AudioSave::ReadData() {
    if (fmtCtx == nullptr) {
        return 0;
    }
    AVSampleFormat fmt = AV_SAMPLE_FMT_S16;
    int pcmSize = av_get_bytes_per_sample((AVSampleFormat)fmt) * 2 * 1024;
    out_buffer = (char *)av_malloc(pcmSize);
    if (audioInput->bytesReady() >= pcmSize) {
        int size = 0;
        while (size != pcmSize) {
            int len = inputDevice->read(out_buffer + size, pcmSize - size);
            if (len < 0) {
                qDebug() << "read data failed";
                return nullptr;
            }

            size += len;
        }
        return out_buffer;
    }
    return nullptr;
}

void AudioSave::writeAudioData() {
    if (!isInitFFmpeg) {
        isInitFFmpeg = true;
        // pcmFrame = av_frame_alloc();
        swrFrame = av_frame_alloc();
        // if (pcmFrame == nullptr || swrFrame == nullptr) {
        //     qDebug() << "alloc frame failed";
        //     return;
        // }
        // pcmFrame->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
        // pcmFrame->sample_rate = 44100;
        // pcmFrame->format = AV_SAMPLE_FMT_S16;
        // pcmFrame->nb_samples =
        //     data.size() / 4; // 2 channels, 16 bits per sample
        // int ret = av_frame_get_buffer(pcmFrame, 0);
        // if (ret < 0) {
        //     ShowError(ret);
        //     return;
        // }
        swrFrame->ch_layout = codecCtx->ch_layout;
        swrFrame->sample_rate = 44100;
        swrFrame->format = codecCtx->sample_fmt;
        swrFrame->nb_samples = 1024;
        int ret = av_frame_get_buffer(swrFrame, 0);
        if (ret < 0) {
            ShowError(ret);
            return;
        }
    }

    if (!Swrconvert(&out_buffer)) {
        qDebug() << "swr convert failed";
        return;
    }

    qDebug() << "frame size:" << swrFrame->nb_samples;
    int ret = avcodec_send_frame(codecCtx, swrFrame);
    if (ret < 0) {
        ShowError(ret);
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codecCtx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            ShowError(ret);
            return;
        }
        pkt->pts = pkt->dts = count;
        count += 1024;
        ret = av_write_frame(fmtCtx, pkt);
        if (ret < 0) {
            ShowError(ret);
            return;
        }
        qDebug() << "write frame";
        av_packet_unref(pkt);
    }
}

void AudioSave::flush() {
    QMutexLocker locker(&mutex);
    int ret = 0;
    // 停止时冲洗编码器
    avcodec_send_frame(codecCtx, NULL); // 发送冲洗信号
    while (1) {
        ret = avcodec_receive_packet(codecCtx, pkt);
        if (ret == AVERROR_EOF) {
            break; // 编码器已无剩余数据
        } else if (ret < 0 && ret != AVERROR(EAGAIN)) {
            // 处理错误
            break;
        }
        ret = av_write_frame(fmtCtx, pkt);
        if (ret < 0) {
            ShowError(ret);
            return;
        }
        av_packet_unref(pkt);
    }
}
