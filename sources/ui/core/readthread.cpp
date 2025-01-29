#include "readthread.hpp"
#include "libavutil/frame.h"
#include "videodecode.hpp"
#include "videosave.hpp"
#include <QVariant>

// #include <playimage.h>
#include <qdebug.h>
#include <QDebug>
#include <QEventLoop>
#include <qglobal.h>
#include <qimage.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <QTimer>


ReadThread::ReadThread(QObject *parent) : QThread(parent) {
    m_videoDecode = new VideoDecode();
    m_videoSave = new VideoSave();
    m_audioSave = new AudioSave();

    qRegisterMetaType<PlayState>("PlayState");
}

ReadThread::~ReadThread() {
    if (m_videoSave) {
        delete m_videoSave;
    }
    if (m_videoDecode) {
        delete m_videoDecode;
    }
    if(m_audioSave) {
        delete m_audioSave;
    }
}

void ReadThread::open(QString const &url) {
    if (!this->isRunning()) {
        m_url = url;
        emit this->start();
    }
}

void ReadThread::close() {
    m_play = false;
}

QString const &ReadThread::url() const {
    return m_url;
}

void ReadThread::saveVideo(QString const &filename) {
    m_videoSave->open(m_videoDecode->getVideoStream(), filename);
}

void ReadThread::saveAudio(QString const &filename) {
    if (!m_audioSave->open(filename)) {
        return;
    }
    m_audioSave->startRecord();
}

void ReadThread::stop() {
    m_videoSave->close();
    m_audioSave->stopRecord();
}

void sleepMsec(int msec) {
    if (msec <= 0) {
        return;
    }
    QEventLoop loop; // 定义一个新的事件循环
    QTimer::singleShot(
        msec, &loop,
        SLOT(quit())); // 创建单次定时器，槽函数为事件循环的退出函数
    loop.exec(); // 事件循环开始执行，程序会卡在这里，直到定时时间到，本循环被退出
}

void ReadThread::run() {
    bool ret = m_videoDecode->open(m_url);
    if (ret) {
        m_play = true;
        emit PlayState(play);
    } else {
        qWarning() << "open failed";
    }

    while (m_play) {
        AVFrame *frame = m_videoDecode->read();
        if (frame) {
            m_videoSave->write(frame);
            emit repaint(frame);
        }
        
        char* data = m_audioSave->ReadData();
        if (data) {
            m_audioSave->writeAudioData();
        } else {
            msleep(10);
        }   
    }
    qDebug() << "close";
    m_videoDecode->close();
    emit PlayState(end);
}
