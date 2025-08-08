#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QDebug>
#include <QVector>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QTransform>
#include <QElapsedTimer>
#include <QList>
#include <QRectF>
#include <opencv4/opencv2/opencv.hpp>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

private:
    Ui::MainWindow *ui;
    int cal_times(int i);
};



class DrawingCanvas : public QWidget
{
    Q_OBJECT
public:
    DrawingCanvas(QWidget *parent = nullptr); //构建函数，初始化
    void deleteSelectedRectangle(); //删除矩形框
    void deleteAllRectangle(); //删除所有矩形框
    void loadBackgroundImage(); //加载图片并显示
    void splitImageByRects(void); //利用画出的矩形切割图片
    QRect getImageDisplayRect(); // 计算保持宽高比的图片显示区域
    QPointF windowToImage(const QPoint &windowPos); // 将窗口坐标转换为图片坐标
    QPoint imageToWindow(const QPointF &imagePos); // 将图片坐标转换为窗口坐标
protected:
    void paintEvent(QPaintEvent *event) override ; //窗口中显示界面绘制
    void mousePressEvent(QMouseEvent *event) override; //鼠标行为：选中矩形、绘制新矩形、移动图片、右键菜单
    void mouseMoveEvent(QMouseEvent *event) override; //鼠标行为：拖动矩形、移动图片
    void mouseReleaseEvent(QMouseEvent *event) override; //鼠标行为：左键释放后绘制新矩形
    void wheelEvent(QWheelEvent *event) override; //滚轮行为：放大缩小视图
    void resizeEvent(QResizeEvent *event) override; //窗口大小改变：居中显示
private:
    bool isDrawing = false;
    bool isPanning = false;
    QPoint startPoint;
    QPoint currentPoint;
    QPoint panStartPoint;
    QPoint lastMousePos;
    QVector<QRectF> rectangles; // 存储图片坐标系的矩形
    int selectedRectIndex = -1;
    QPointF dragOffset;
    QPoint panOffset;
    QAction *deleteAction;
    QAction *deleteAllAction;
    QAction *loadImageAction;
    QAction *resetZoomAction;
    QAction *splitPictureAction;
    QPixmap backgroundImage;
    QColor backgroundColor;
    QString fileName="";
    double zoomFactor = 1.0;
};



#endif // MAINWINDOW_H
