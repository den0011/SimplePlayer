#include "videoplayer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Устанавливаем стиль, похожий на MPC-HC
    a.setStyle("Fusion");

    VideoPlayer w;
    w.setWindowTitle("Qt Video Player (MPC-HC style)");
    w.resize(800, 600);
    w.show();

    return a.exec();
}
