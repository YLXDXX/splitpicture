// mainwindow.h
// 仅作为顶层窗口，持有 DrawingCanvas，接收命令行参数并初始化
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class DrawingCanvas;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    DrawingCanvas* canvas() const;

private:
    DrawingCanvas *m_canvas;
};

#endif // MAINWINDOW_H
