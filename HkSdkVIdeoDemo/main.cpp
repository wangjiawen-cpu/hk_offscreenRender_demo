#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    qRegisterMetaType<GLuint>("GLuint"); // 注册 GLuint 类型
    qRegisterMetaType<GLTexture>("GLTexture");  // 先注册嵌套类型
    qRegisterMetaType<YUVBuffer>("YUVBuffer"); // 再注册复合类型

    QApplication a(argc, argv);
    Widget w;
    w.init();
    w.show();
    return a.exec();
}
