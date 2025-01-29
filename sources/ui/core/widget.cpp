#include "widget.hpp"
#include <QCameraInfo>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <qglobal.h>
#include <qpushbutton.h>
#include <QVBoxLayout>

extern "C" { // 用C规则编译指定的代码
#include "libavcodec/avcodec.h"
}

Q_DECLARE_METATYPE(AVFrame) // 注册结构体，否则无法通过信号传递AVFrame

Widget::Widget(QWidget *parent) : QWidget(parent) {
    setWindowTitle(QString("Qt+ffmpeg打开本地摄像头录像Demo "));
    // 使用QOpenGLWindow绘制
    playImage = new PlayImage();

    // 创建布局
    mainLayout = new QVBoxLayout(this);

    // 创建顶部布局（文本和按钮）
    topLayout = new QHBoxLayout();
    playButton = new QPushButton("开始播放", this);
    recordButton = new QPushButton("开始录制", this);
    com_url = new QComboBox(this);
    com_url->setEditable(true);
    com_url->setDuplicatesEnabled(false);
    topLayout->addWidget(com_url);
    topLayout->addWidget(playButton);
    topLayout->addWidget(recordButton);

    // 将所有布局添加到主布局中
    mainLayout->addLayout(topLayout);
#if USE_WINDOW
    mainLayout->addWidget(QWidget::createWindowContainer(
        playImage)); // 这一步加载速度要比OpenGLWidget慢一点
#else
    mainLayout->addWidget(playImage);
#endif

    // 设置窗口属性
    setLayout(mainLayout);

    m_readThread = new ReadThread();
    connect(m_readThread, &ReadThread::repaint, playImage, &PlayImage::repaint,
            Qt::BlockingQueuedConnection);
    connect(m_readThread, &ReadThread::playState, this, &Widget::on_playState);
    connect(playButton, &QPushButton::clicked, this,
            &Widget::on_btn_open_clicked);
    connect(recordButton, &QPushButton::clicked, this,
            &Widget::on_btn_save_clicked);

    // 获取可用摄像头列表
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (auto camera: cameras) {
#if defined(Q_OS_WIN)
        com_url->addItem(
            "video=" +
            camera.description()); // ffmpeg在Window下要使用video=description()
#elif defined(Q_OS_LINUX)
        com_url->addItem(
            camera.deviceName()); // ffmpeg在linux下要使用deviceName()
#elif defined(Q_OS_MAC)
#endif
    }
}

Widget::~Widget() {
    // 释放视频读取线程
    if (m_readThread) {
        // 由于使用了BlockingQueuedConnection，所以在退出时如果信号得不到处理就会卡死，所以需要取消绑定
        disconnect(m_readThread, &ReadThread::repaint, playImage,
                   &PlayImage::repaint);
        m_readThread->close();
        m_readThread->wait();
        delete m_readThread;
    }
}

void Widget::on_btn_open_clicked() {
    if (playButton->text() == "开始播放") {
        playButton->setText("停止播放");
        playImage->start();
        m_readThread->open(com_url->currentText());
    } else {
        playButton->setText("开始播放");
        playImage->clear();
        this->update();
        m_readThread->close();
    }
}

void Widget::on_playState(ReadThread::PlayState state) {
    if (state == ReadThread::play) {
        this->setWindowTitle(QString("正在播放：%1").arg(m_readThread->url()));
        //playButton->setText("停止播放");
    } else {
        //playButton->setText("开始播放");
        this->setWindowTitle(QString("Qt+ffmpeg打开本地摄像头录像Demo "));
    }
}

void Widget::on_btn_save_clicked() {
    if (recordButton->text() == "开始录制") {
        m_readThread->saveVideo(QString("%1.h264").arg(
            QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss")));
        m_readThread->saveAudio(QString("%1.aac").arg(
            QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss")));
        recordButton->setText("停止");
    } else {
        m_readThread->stop();
        recordButton->setText("开始录制");
    }
}
