#include <QApplication>
#include "widget.hpp"
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    // 检查并选择一个摄像头
    Widget w;
    w.show();

    return app.exec();
}
