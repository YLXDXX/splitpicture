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


enum ResizeEdge { None, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };
class DrawingCanvas : public QWidget
{
    Q_OBJECT
public:
    DrawingCanvas(QWidget *parent = nullptr); //构建函数，初始化
    QPointF windowToImage(const QPoint &windowPos); // 将窗口坐标转换为图片坐标
    QPoint imageToWindow(const QPointF &imagePos); // 将图片坐标转换为窗口坐标
    ResizeEdge getResizeEdge(const QPointF &pos); // 检测鼠标在所选矩形的哪个边上
    QRect getImageDisplayRect(); // 计算保持宽高比的图片显示区域
    void setCursorForEdge(ResizeEdge edge); // 根据边鼠标位置设置鼠标光标
    void deleteSelectedRectangle(); //删除矩形框
    void deleteAllRectangle(); //删除所有矩形框
    void loadBackgroundImage(); //加载图片并显示
    void splitImageByRects(const cv::Mat &Image,const QString imagePath); //利用画出的矩形切割图片
    QRectF imgContentRect(const cv::Mat &img); //返回图片去除白边后的内容区域
    void removeWhiteBorder(const cv::Mat &Image); //去除选中矩形的白边
    void removeAllWhiteBorder(const cv::Mat &Image); //去除所有创建矩形所的白边
protected:
    void paintEvent(QPaintEvent *event) override ; //窗口中显示界面绘制
    void mousePressEvent(QMouseEvent *event) override; //鼠标按键按下行为
    void mouseMoveEvent(QMouseEvent *event) override; //鼠标移动行为
    void mouseReleaseEvent(QMouseEvent *event) override; //鼠标按键释放行为
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
    QAction *removeWhiteAction;
    QAction *removeAllWhiteAction;
    QPixmap backgroundImage;
    QColor backgroundColor;
    QString fileName="";
    double zoomFactor = 1.0;
    ResizeEdge m_resizeEdge = None;
    const int edgeMargin = 5; // 边框检测灵敏度
    bool isRecResizing = false;
    bool isMoveToRecEdge=false;
    QRectF resizeOriginalRect;
    const int minimumRecLength=10; //矩形长宽的最小长度
    const int removeWhitePadding=5; //截取白边时预留空白
    const int removeWhiteThreshold=200; //白色区域截取灵敏度控制 0-255，存白色为 255
    cv::Mat displayImage;
};



#endif // MAINWINDOW_H
