#pragma once

#include <QSize>
#include <QString>
#include <qglobal.h>


struct AVFormatContext;
struct AVCodecContext;
struct AVRational;
struct AVPacket;
struct AVFrame;
struct SwsContext;
struct AVBufferRef;
struct AVInputFormat;
struct AVStream;
class QImage;


class VideoDecode {
public:
    VideoDecode();
    ~VideoDecode();
    bool open(const QString &filename = QString());
    void close();
    AVFrame *read();
    AVStream *getVideoStream() const;


private:
    void initFFmpeg();
    void showError(int err);
    qreal rationalToDouble(AVRational* rational);
    bool toYUV420P();
    void clear();
    void free();

private:
    const AVInputFormat* m_inputFormat = nullptr;
    AVFormatContext* m_formatContext = nullptr;   // 解封装上下文
    AVCodecContext* m_codecContext = nullptr;     // 解码器上下文
    SwsContext* m_swsContext = nullptr;           // 图像转换上下文
    AVPacket* m_packet = nullptr;                 // 数据包
    AVFrame* m_frame = nullptr;                   // 解码后的视频帧（原始格式）
    AVFrame* m_frame1 = nullptr;                  // 解码后的视频帧（转换为YUV420P格式）
    int m_videoIndex = 0;                         // 视频流索引
    qint64 m_totalTime = 0;                       // 视频总时长
    qint64 m_totalFrames = 0;                     // 视频总帧数
    qint64 m_obtainFrames = 0;                    // 视频当前获取到的帧数
    qreal m_frameRate = 0;                        // 视频帧率
    char* m_error = nullptr;                      // 保存异常信息
};