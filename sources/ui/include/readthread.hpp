#pragma once

#include <QThread>
#include <QTime>
#include <qobject.h>
#include <qobjectdefs.h>

class VideoDecode;
class VideoSave;
class PlayImage;
struct AVFrame;

class ReadThread : public QThread {
    Q_OBJECT
public:
    enum PlayState {
        play,
        end
    };

public:
    explicit ReadThread(QObject *parent = nullptr);
    ~ReadThread() override;
    void open(QString const &url = QString());
    void close();
    QString const &url() const;
    void saveVideo(QString const &filename);
    void stop();

protected:
    void run() override;

signals:
    void repaint(AVFrame *frame);
    void playState(PlayState state);

private:
    VideoDecode *m_videoDecode = nullptr;
    VideoSave *m_videoSave = nullptr;
    QString m_url;
    bool m_play = false;
};
