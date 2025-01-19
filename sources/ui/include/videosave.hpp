#pragma once
#include <QMutex>
#include <QString>

struct AVCodecParameters;
struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVPacket;
struct AVOutputFormat;

class VideoSave {
public:
    VideoSave();
    ~VideoSave();

    bool open(AVStream *inStream, QString const &filename);
    void write(AVFrame *frame);
    void close();

private:
    void showError(int err);

private:
    AVFormatContext *m_formatContext = nullptr;
    AVCodecContext *m_codecContext = nullptr;
    AVStream *m_videostream = nullptr;
    AVPacket *m_packet = nullptr;
    int m_index = 0;
    bool m_writeHeader = false;
    QMutex m_mutex;
};
