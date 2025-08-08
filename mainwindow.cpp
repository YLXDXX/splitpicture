#include "mainwindow.h"

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

#include <QScreen>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
        // 获取主屏幕尺寸
        QScreen *primaryScreen = QGuiApplication::primaryScreen();
        QRect screenGeometry = primaryScreen->availableGeometry();

        // 设置为屏幕大小的80%
        int width = screenGeometry.width() * 0.8;
        int height = screenGeometry.height() * 0.8;
        resize(width, height);

        // 居中窗口
        move(screenGeometry.center() - rect().center());
}

MainWindow::~MainWindow()
{
}



DrawingCanvas::DrawingCanvas(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true); // 启用鼠标跟踪
    setAutoFillBackground(false);

    // 创建菜单动作
    deleteAction = new QAction("删除矩形", this);
    connect(deleteAction, &QAction::triggered, this, &DrawingCanvas::deleteSelectedRectangle);
    
    deleteAllAction = new QAction("删除所有矩形", this);
    connect(deleteAllAction, &QAction::triggered, this, &DrawingCanvas::deleteAllRectangle);
    
    loadImageAction = new QAction("加载背景图片", this);
    connect(loadImageAction, &QAction::triggered, this, &DrawingCanvas::loadBackgroundImage);

    resetZoomAction = new QAction("重置缩放", this);
    connect(resetZoomAction, &QAction::triggered, this, [this]() {
        zoomFactor = 1.0;
        panOffset = QPoint(0, 0);
        update();
    });

    splitPictureAction = new QAction("切割图片", this);
    connect(splitPictureAction, &QAction::triggered, this, &DrawingCanvas::splitImageByRects);
    
    // 初始背景颜色
    backgroundColor = QColor(30, 30, 40);

    // 设置性能优化
    setAttribute(Qt::WA_OpaquePaintEvent);
}

//删除矩形框
void DrawingCanvas::deleteSelectedRectangle()
{
    if (selectedRectIndex >= 0 && selectedRectIndex < rectangles.size()) {
        rectangles.remove(selectedRectIndex);
        selectedRectIndex = -1;
        update();
    }
}

//删除所有矩形框
void DrawingCanvas::deleteAllRectangle()
{
    // 清空向量
    rectangles.clear();
    selectedRectIndex = -1;
    update();
}

//加载图片并显示
void DrawingCanvas::loadBackgroundImage() {
    fileName = QFileDialog::getOpenFileName(this,
                                                    "选择背景图片",
                                                    "",
                                                    "图片文件 (*.png *.jpg *.jpeg *.bmp)");

    if (!fileName.isEmpty()) {
        QPixmap newImage;
        if (newImage.load(fileName)) {
            backgroundImage = newImage;
            // 重置缩放和平移
            zoomFactor = 1.0;
            panOffset = QPoint(0, 0);
            update();
        } else {
            fileName="";
            QMessageBox::warning(this, "加载错误", "无法加载图片: " + fileName);
        }
    }
}

//利用矩形切割图片
void DrawingCanvas::splitImageByRects(void)
{
    if(fileName=="")
    {
        QMessageBox::warning(this, "图片未加载", "请选择图片后再操作");
        return;
    }
    const QString imagePath=fileName;
    const QList<QRectF> rects=rectangles;
    // 读取原始图像
    cv::Mat src = cv::imread(imagePath.toStdString());
    if(src.empty()) {
        qWarning("Failed to load image: %s", qPrintable(imagePath));
        return;
    }

    // 准备文件名组件
    QFileInfo fileInfo(imagePath);
    QString baseName = fileInfo.completeBaseName();
    QString suffix = fileInfo.suffix();
    QString dirPath = fileInfo.absolutePath();

    // 处理每个矩形区域
    for(int i = 0; i < rects.size(); ++i) {
        const QRectF& qrect = rects[i];

        // 转换为整数像素坐标（对齐到最近整数）
        int x = static_cast<int>(std::round(qrect.x()));
        int y = static_cast<int>(std::round(qrect.y()));
        int width = static_cast<int>(std::round(qrect.width()));
        int height = static_cast<int>(std::round(qrect.height()));

        // 边界检查
        if(x < 0) x = 0;
        if(y < 0) y = 0;
        if(x + width > src.cols) width = src.cols - x;
        if(y + height > src.rows) height = src.rows - y;

        // 验证有效区域
        if(width <= 0 || height <= 0) {
            qWarning("Invalid region at index %d: [%d, %d, %d, %d]",
                     i, x, y, width, height);
            continue;
        }

        // 提取ROI
        cv::Mat roi(src, cv::Rect(x, y, width, height));

        // 构造输出文件名
        QString outputName = QString("%1/%2_%3.%4")
                             .arg(dirPath)
                             .arg(baseName)
                             .arg(i+1, 2, 10, QLatin1Char('0')) // 两位数序号
                             .arg(suffix);

        // 保存分割后的图像
        if(!cv::imwrite(outputName.toStdString(), roi)) {
            qWarning("Failed to write: %s", qPrintable(outputName));
        }
    }
}


// 计算保持宽高比的图片显示区域
QRect DrawingCanvas::getImageDisplayRect()
{
    if (backgroundImage.isNull())
        return QRect(0, 0, width(), height());

    // 计算保持宽高比的目标矩形
    QSize imageSize = backgroundImage.size();
    QSize scaledSize = imageSize.scaled(
        width() * zoomFactor,
        height() * zoomFactor,
        Qt::KeepAspectRatio
    );

    // 计算显示位置（居中） - 修正：移除对缩放因子的除法
    int x = (width() - scaledSize.width()) / 2;  // 移除 /zoomFactor
    int y = (height() - scaledSize.height()) / 2; // 移除 /zoomFactor

    // 应用平移偏移
    x += panOffset.x();
    y += panOffset.y();
    //注意，由此得到的 (x,y) 坐标即图片的原点（左上角）在窗口坐标系的坐标
    return QRect(x, y, scaledSize.width(), scaledSize.height()); // 移除 /zoomFactor
}


// 将窗口坐标转换为图片坐标
QPointF DrawingCanvas::windowToImage(const QPoint &windowPos)
{
    if (backgroundImage.isNull())
        return windowPos;


    QPoint temp_windowPos;
    temp_windowPos = windowPos;

    QRect displayRect = getImageDisplayRect();

    //注意，由此得到的 displayRect 对应的 (x,y) 坐标
    //即图片的原点（左上角）在窗口坐标系的坐标
    if (!displayRect.contains(windowPos))
    {
        //这里需要保证绘制的矩形都在图片内
        //先处理 x 的坐标
        if( windowPos.x() > (displayRect.x()+displayRect.width()) )
        {
            temp_windowPos.rx()=displayRect.x()+displayRect.width();
        }else if( windowPos.x() < displayRect.x() )
        {
            temp_windowPos.rx()=displayRect.x();
        }
        //再处理 y 的坐标
        if( windowPos.y() > (displayRect.y()+displayRect.height()) )
        {
            temp_windowPos.ry()=displayRect.y()+displayRect.height();
        }else if( windowPos.y() < displayRect.y() )
        {
            temp_windowPos.ry()=displayRect.y();
        }
    }

    double scaleX = static_cast<double>(backgroundImage.width()) / displayRect.width();
    double scaleY = static_cast<double>(backgroundImage.height()) / displayRect.height();

    return QPointF(
        (temp_windowPos.x() - displayRect.x()) * scaleX,
                   (temp_windowPos.y() - displayRect.y()) * scaleY
    );
}

// 将图片坐标转换为窗口坐标
QPoint DrawingCanvas::imageToWindow(const QPointF &imagePos)
{
    if (backgroundImage.isNull())
        return imagePos.toPoint();

    QRect displayRect = getImageDisplayRect();
    double scaleX = static_cast<double>(displayRect.width()) / backgroundImage.width();
    double scaleY = static_cast<double>(displayRect.height()) / backgroundImage.height();

    return QPoint(
        displayRect.x() + imagePos.x() * scaleX,
                  displayRect.y() + imagePos.y() * scaleY
    );
}

//窗口中显示界面绘制
void DrawingCanvas::paintEvent(QPaintEvent *event)
{
    QElapsedTimer timer;
    timer.start();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 绘制背景
    painter.fillRect(rect(), backgroundColor);

    // 绘制背景图片（如果有）
    if (!backgroundImage.isNull()) {
        QRect displayRect = getImageDisplayRect();

        // 绘制图片
        painter.drawPixmap(displayRect, backgroundImage);

        // 绘制图片边框
        painter.setPen(QPen(QColor(150, 150, 170), 1));
        painter.drawRect(displayRect);

        // 绘制图片信息
        painter.setPen(Qt::white);
        QString info = QString("图片: %1x%2 | 显示: %3x%4 | 缩放: %5%")
        .arg(backgroundImage.width())
        .arg(backgroundImage.height())
        .arg(displayRect.width())
        .arg(displayRect.height())
        .arg(int(zoomFactor * 100));
        painter.drawText(10, height() - 10, info);
    } else {
        // 绘制网格背景
        painter.setPen(QPen(QColor(60, 60, 70), 1));
        for (int x = 0; x < width(); x += 20) {
            painter.drawLine(x, 0, x, height());
        }
        for (int y = 0; y < height(); y += 20) {
            painter.drawLine(0, y, width(), y);
        }
    }

    // 绘制所有矩形
    for (int i = 0; i < rectangles.size(); ++i) {
        const QRectF &rect = rectangles[i];
        QColor rectColor = (i == selectedRectIndex) ? QColor(255, 215, 0, 120) : QColor(70, 130, 180, 100);
        QColor borderColor = (i == selectedRectIndex) ? Qt::yellow : Qt::white;

        painter.setBrush(rectColor);
        painter.setPen(QPen(borderColor, 1));

        // 将图片坐标转换为窗口坐标
        QPoint topLeft = imageToWindow(rect.topLeft());
        QPoint bottomRight = imageToWindow(rect.bottomRight());
        painter.drawRect(QRect(topLeft, bottomRight));

        // 在矩形中心绘制编号
        QPoint center = imageToWindow(rect.center());
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(center, QString::number(i + 1));
    }

    // 绘制当前正在绘制的矩形
    if (isDrawing) {
        QPointF imgStart = windowToImage(startPoint);
        QPointF imgCurrent = windowToImage(currentPoint);

        if (!imgStart.isNull() || !imgCurrent.isNull()) {
            QRectF imgRect(imgStart, imgCurrent);

            painter.setBrush(QBrush(QColor(255, 100, 100, 80)));
            painter.setPen(QPen(Qt::red, 2, Qt::DashLine));

            QPoint winStart = imageToWindow(imgRect.topLeft());
            QPoint winEnd = imageToWindow(imgRect.bottomRight());
            painter.drawRect(QRect(winStart, winEnd));

            // 显示绘制尺寸
            painter.setPen(Qt::white);
            QString sizeText = QString("%1 x %2")
            .arg(fabs(imgRect.width()), 0, 'f', 1)
            .arg(fabs(imgRect.height()), 0, 'f', 1);
            painter.drawText(winStart + QPoint(5, -5), sizeText);
        }
    }

    // 绘制状态信息
    painter.setPen(Qt::white);
    painter.drawText(10, 20, "左键点击并拖动: 绘制新矩形");
    painter.drawText(10, 40, "右键点击矩形: 选择/删除矩形");
    painter.drawText(10, 60, "拖动已选矩形: 移动矩形位置");
    painter.drawText(10, 80, "滚轮: 缩放视图 | 中键: 平移视图");
    painter.drawText(10, 100, QString("矩形数量: %1 | 绘制时间: %2ms")
                     .arg(rectangles.size())
                     .arg(timer.elapsed()) );

    //绘制当前选中信息，选中矩形后的提示信息
    if (selectedRectIndex >= 0 && selectedRectIndex < rectangles.size()) {
        painter.setPen(Qt::green);
        QPointF center = rectangles[selectedRectIndex].center();
        QPoint winCenter = imageToWindow(center);
        //painter.drawText(winCenter + QPoint(-50, -10), QString("选中 #%1").arg(selectedRectIndex + 1));
        QPointF local_topLeft=rectangles[selectedRectIndex].topLeft();
        painter.save();// 保存当前画笔设置
        painter.setPen(QColor(255, 165, 0)); // 仅设置文本颜色 橙色

        painter.drawText(winCenter + QPoint(-18, 20),QString("(%1,%2)")
                         .arg(static_cast<int>(std::round(local_topLeft.x()))) // 转换为整数像素坐标（对齐到最近整数）
                         .arg(static_cast<int>(std::round(local_topLeft.y()))));
        painter.restore();// 恢复之前的画笔设置
    }
}

//鼠标行为：选中矩形、绘制新矩形、移动图片、右键菜单
void DrawingCanvas::mousePressEvent(QMouseEvent *event)
{
    lastMousePos = event->pos();

    if (event->button() == Qt::LeftButton) {
        // 检查是否点击在已有矩形上
        selectedRectIndex = -1;
        for (int i = 0; i < rectangles.size(); ++i) {
            QPointF imgPos = windowToImage(event->pos());
            if (!imgPos.isNull() && rectangles[i].contains(imgPos)) {
                selectedRectIndex = i;
                dragOffset = rectangles[i].topLeft() - imgPos;
                update(); //更新界面「高亮先中的矩形，由 paintEvent 负责」
                break;
            }
        }

        // 如果没有选中矩形，开始绘制新矩形
        if (selectedRectIndex == -1) {
            isDrawing = true;
            startPoint = currentPoint = event->pos();
        }
    }else if (event->button() == Qt::RightButton) {
        // 右键选择矩形
        int newSelection = -1;
        for (int i = 0; i < rectangles.size(); ++i) {
            QPointF imgPos = windowToImage(event->pos());
            if (!imgPos.isNull() && rectangles[i].contains(imgPos)) {
                newSelection = i;
                break;
            }
        }

        // 如果点击的是已选中的矩形，弹出菜单
        if (newSelection >= 0) {
            selectedRectIndex = newSelection;

            // 创建上下文菜单
            QMenu contextMenu(this);
            contextMenu.addAction(deleteAction);
            contextMenu.addAction(loadImageAction);
            contextMenu.addAction(resetZoomAction);
            contextMenu.addAction("取消选择", [this]() {
                selectedRectIndex = -1;
                update();
            });

            // 显示菜单
            contextMenu.exec(event->globalPosition().toPoint());
        } else {
            // 在空白处点击右键显示菜单
            QMenu contextMenu(this);
            contextMenu.addAction(loadImageAction);
            contextMenu.addAction(resetZoomAction);
            contextMenu.addAction(deleteAllAction);
            contextMenu.addAction(splitPictureAction);
            contextMenu.exec(event->globalPosition().toPoint());
            selectedRectIndex = -1;
        }

        update();
    }
    else if (event->button() == Qt::MiddleButton) {
        // 中键拖动视图
        isPanning = true;
        panStartPoint = event->pos();
    }
}

//鼠标行为：拖动矩形、移动图片
void DrawingCanvas::mouseMoveEvent(QMouseEvent *event)
{
    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();

    if (isDrawing) {
        // 绘制中的矩形
        currentPoint = event->pos();
        update();
    }
    else if (selectedRectIndex >= 0 && (event->buttons() & Qt::LeftButton)) {
        // 移动选中的矩形
        QPointF imgPos = windowToImage(event->pos());
        if (!imgPos.isNull()) {
            QPointF newPos = imgPos + dragOffset;
            QRectF t_a=rectangles[selectedRectIndex];
            if(!backgroundImage.isNull()) //当没有图片加载时，不作处理
            {
                t_a.moveTo(newPos);
                //这里需要保证移动矩形的位置都在图片的范围内
                //先考虑 x 坐标，移动到的点 newPos 即矩形的左上角点坐标
                if(t_a.bottomRight().x() > backgroundImage.size().width())
                {
                    newPos.rx()=t_a.topLeft().x()-t_a.bottomRight().x()+backgroundImage.size().width();

                }else if (t_a.topLeft().x() < 0)
                {
                    newPos.rx()=0;
                }
                //再考虑 y 坐标，移动到的点 newPos 即矩形的左上角点坐标
                if(t_a.bottomRight().y() > backgroundImage.size().height())
                {
                    newPos.ry()=t_a.topLeft().y()-t_a.bottomRight().y()+backgroundImage.size().height();
                }else if (t_a.topLeft().y() < 0)
                {
                    newPos.ry()=0;
                }
            }
            rectangles[selectedRectIndex].moveTo(newPos);
            update();
        }
    }
    else if (isPanning && (event->buttons() & Qt::MiddleButton)) {
        // 平移背景
        panOffset += delta;
        update();
    }
}

//鼠标行为：左键释放后绘制新矩形
void DrawingCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDrawing) {
        // 完成绘制新矩形
        isDrawing = false;

        QPointF imgStart = windowToImage(startPoint);
        QPointF imgEnd = windowToImage(event->pos());
        qDebug() << imageToWindow(imgStart);


        if (!imgStart.isNull() || !imgEnd.isNull()) {
            QRectF newRect(imgStart, imgEnd);
            newRect = newRect.normalized();

            // 只添加有效矩形（最小尺寸）
            if (newRect.width() >= 10 && newRect.height() >= 10) {
                rectangles.append(newRect);
                selectedRectIndex = rectangles.size() - 1;
            }

            qDebug() << newRect.topLeft() << "\t" << newRect.bottomRight();
        }
        update();
    }
    else if (event->button() == Qt::MiddleButton) {
        isPanning = false;
    }
}

//滚轮行为：放大缩小视图
void DrawingCanvas::wheelEvent(QWheelEvent *event)
{
    // 计算缩放中心点
    QPointF center = windowToImage(event->position().toPoint());
    if (center.isNull()) return;

    // 计算缩放因子
    double zoomAmount = 1.1;
    if (event->angleDelta().y() < 0) {
        zoomAmount = 1.0 / zoomAmount;
    }

    // 保存当前鼠标在图片中的位置
    QPointF imgPosBeforeZoom = center;

    // 更新缩放因子
    double newZoomFactor = zoomFactor * zoomAmount;
    newZoomFactor = qBound(0.1, newZoomFactor, 10.0); // 限制缩放范围

    // 计算缩放后鼠标在图片中的位置应该保持不变
    QPointF imgPosAfterZoom = imgPosBeforeZoom;

    // 调整平移偏移以保持缩放中心
    QPoint mousePos = event->position().toPoint();
    QPointF imgPosAfterZoomWindow = imageToWindow(imgPosAfterZoom);

    panOffset += (mousePos - imgPosAfterZoomWindow.toPoint());

    zoomFactor = newZoomFactor;
    update();
}

//窗口大小改变：居中显示
void DrawingCanvas::resizeEvent(QResizeEvent *event)
{
    // 窗口大小改变时保持图片居中
    QSize oldSize = event->oldSize();
    QSize newSize = size();

    if (oldSize.isValid()) {
        panOffset += QPoint((newSize.width() - oldSize.width()) / 2,
                            (newSize.height() - oldSize.height()) / 2);
    }

    QWidget::resizeEvent(event);
}
