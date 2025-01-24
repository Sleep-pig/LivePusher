#pragma once
#include <QAudioInput>
#include <qobject.h>
#include <qobjectdefs.h>

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
public:
    explicit AudioSave(QObject *parent = nullptr);
    ~AudioSave();
    bool open(QString const &filename);
    void startRecord();
    void stopRecord();

private:
    QAudioInput *audioInput = nullptr;
    QIODevice *inputDevice = nullptr;

    AVFormatContext *fmtCtx = nullptr;
    AVCodecContext *codecCtx = nullptr;
    AVCodec const *codec = nullptr;
    SwrContext *swrCtx = nullptr;
    AVFrame *pcmFrame = nullptr;
    AVFrame *swrFrame = nullptr;
    AVPacket *pkt = nullptr;

    bool initFFmpeg(QString const &filename);
    void cleanupFFmpeg();
    void ShowError(int ret);

    bool Swrconvert(QByteArray const &data);
    void writeAudioData(QByteArray const &data);
};
