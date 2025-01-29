#pragma once
#include <QAudioInput>
#include <cstdint>
#include <qmutex.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <QFile>
struct AVCodecParameters;
struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVPacket;
struct SwrContext;
struct AVCodec;

class AudioSave : public QObject {
    Q_OBJECT
    friend class ReadThread;

public:
    explicit AudioSave(QObject *parent = nullptr);
    ~AudioSave();
    bool open(QString const &filename);
    void startRecord();
    void stopRecord();
    char* ReadData();

private:
    bool isRecording = false;
    QAudioInput *audioInput = nullptr;
    QIODevice *inputDevice = nullptr;

    AVFormatContext *fmtCtx = nullptr;
    AVCodecContext *codecCtx = nullptr;
    AVCodec const *codec = nullptr;
    SwrContext *swrCtx = nullptr;
    // AVFrame *pcmFrame = nullptr;
    char *out_buffer = nullptr;
    AVFrame *swrFrame = nullptr;
    AVPacket *pkt = nullptr;
    QMutex mutex;
    bool isInitFFmpeg = false;
    bool initFFmpeg(QString const &filename);
    void cleanupFFmpeg();
    void ShowError(int ret);

    bool Swrconvert(char **data);
    void writeAudioData();
    void flush();
    int count = 0;
};
