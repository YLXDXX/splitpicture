#include "mainwindow.h"
#include <QApplication>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.setWindowTitle("图像切割");

    DrawingCanvas *canvas = new DrawingCanvas();
    w.setCentralWidget(canvas);

    w.show();
    return a.exec();
}
