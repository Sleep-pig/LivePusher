#pragma once
#include "playimage.hpp"
#include "readthread.hpp"
#include <QComboBox>
#include <QPushButton>
#include <QWidget>
#include <QVBoxLayout>
class Widget : public QWidget {
    Q_OBJECT
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:

    void on_btn_open_clicked();
    void on_playState(ReadThread::PlayState state);
    void on_btn_save_clicked();

private:
    PlayImage *playImage = nullptr;
    ReadThread *m_readThread = nullptr;
    QPushButton *playButton = nullptr;
    QPushButton *recordButton = nullptr;
    QComboBox *com_url = nullptr;
    QVBoxLayout *mainLayout =nullptr;
    QHBoxLayout *topLayout = nullptr;
};