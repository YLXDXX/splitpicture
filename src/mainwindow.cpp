// mainwindow.cpp
// 仅作为顶层窗口，持有 DrawingCanvas，接收命令行参数并初始化
#include "mainwindow.h"
#include "drawingcanvas.h"
#include <QScreen>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, m_canvas(new DrawingCanvas(this))
{
    // 设置窗口大小（屏幕的80%）
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    if (primaryScreen) {
        QRect screenGeometry = primaryScreen->availableGeometry();
        int width = screenGeometry.width() * 0.8;
        int height = screenGeometry.height() * 0.8;
        resize(width, height);
        move(screenGeometry.center() - rect().center());
    }

    setCentralWidget(m_canvas);
    setWindowTitle("SplitPicture");
}

MainWindow::~MainWindow()
{
}

DrawingCanvas* MainWindow::canvas() const
{
    return m_canvas;
}
