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
#include <QCursor>
#include <QToolBar>
#include <QToolButton>
#include <QStyle>
#include <QtConcurrentRun>


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
    connect(splitPictureAction, &QAction::triggered, this,
            [this](){DrawingCanvas::splitImageByRects(displayImage,fileName,outFileDirectory,outputFilePrefix);});
    

    removeWhiteAction = new QAction("去除白边", this);
    connect(removeWhiteAction, &QAction::triggered, this,
            [this](){DrawingCanvas::removeWhiteBorder(displayImage);});

    removeAllWhiteAction = new QAction("去除所有白边", this);
    connect(removeAllWhiteAction, &QAction::triggered, this,
            [this](){DrawingCanvas::removeAllWhiteBorder(displayImage);});

    addWhiteBorderOrigPictureAction = new QAction("原图加白边", this);
    connect(addWhiteBorderOrigPictureAction, &QAction::triggered, this,&DrawingCanvas::addWhiteBorderOrigPicture);

    // 初始背景颜色
    backgroundColor = QColor(30, 30, 40);

    // 设置性能优化
    setAttribute(Qt::WA_OpaquePaintEvent);
}

//
////
//窗口中出现行为设置
////
//

//窗口显示界面绘制，update() 会刷新此界面
void DrawingCanvas::paintEvent(QPaintEvent *event)
{
    QElapsedTimer timer;
    timer.start();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setFont(QFont("Arial", 10, QFont::Bold));

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

    //选中后矩形大小的缩放
    if( isMoveToRecEdge && isRecResizing && selectedRectIndex == clickRectNumber )
    {
        resizeOriginalRect = rectangles[selectedRectIndex]; // 保存原始矩形
        QPointF imgCurrent = windowToImage(currentPoint);
        // 调整矩形大小
        QPointF delta;
        QRectF newRect = resizeOriginalRect;
        switch (m_resizeEdge) {
            case Left:
                delta = imgCurrent- resizeOriginalRect.bottomLeft();
                newRect.setLeft(newRect.left() + delta.x());
                break;
            case Right:
                delta = imgCurrent- resizeOriginalRect.bottomRight();
                newRect.setRight(newRect.right() + delta.x());
                break;
            case Top:
                delta = imgCurrent - resizeOriginalRect.topRight();
                newRect.setTop(newRect.top() + delta.y());
                break;
            case Bottom:
                delta = imgCurrent- resizeOriginalRect.bottomRight();
                newRect.setBottom(newRect.bottom() + delta.y());
                break;
            case TopLeft:
                delta = imgCurrent - resizeOriginalRect.topLeft();
                newRect.setTopLeft(newRect.topLeft() + delta);
                break;
            case TopRight:
                delta = imgCurrent- resizeOriginalRect.topRight();
                newRect.setTopRight(newRect.topRight() + delta);
                break;
            case BottomLeft:
                delta = imgCurrent- resizeOriginalRect.bottomLeft();
                newRect.setBottomLeft(newRect.bottomLeft() + delta);
                break;
            case BottomRight:
                delta = imgCurrent- resizeOriginalRect.bottomRight();
                newRect.setBottomRight(newRect.bottomRight() + delta);
                break;
            default:
                newRect = resizeOriginalRect;
        }
        //防止矩形无效
        if (newRect.width() > minimumRecLength && newRect.height() > minimumRecLength)
        {
            rectangles[selectedRectIndex] = newRect.normalized();
        }
    }

    // 绘制状态信息
    if (backgroundImage.isNull()) //添加图片后就不再显示
    {
        painter.setPen(Qt::white);
        painter.drawText(10, 20, "左键点击并拖动: 绘制新矩形");
        painter.drawText(10, 40, "右键点击矩形: 选择/删除矩形");
        painter.drawText(10, 60, "拖动已选矩形: 移动矩形位置");
        painter.drawText(10, 80, "滚轮: 缩放视图 | 中键: 平移视图");
        painter.drawText(10, 100, QString("矩形数量: %1 | 绘制时间: %2ms")
                         .arg(rectangles.size())
                         .arg(timer.elapsed()) );
    }else
    {
        painter.drawText(10, 20, QString("矩形数量: %1 | 绘制时间: %2ms")
                         .arg(rectangles.size())
                         .arg(timer.elapsed()) );
        painter.drawText(10, 40, QString("Output: %1").arg(outFileDirectory) );
        painter.drawText(10, 60, QString("Prefix: %1").arg(outputFilePrefix) );
    }


    //绘制当前选中信息，选中矩形后的提示信息
    if (selectedRectIndex >= 0 && selectedRectIndex < rectangles.size()) {
        painter.setPen(Qt::green);
        QPointF center = rectangles[selectedRectIndex].center();
        QPoint winCenter = imageToWindow(center);
        //painter.drawText(winCenter + QPoint(-50, -10), QString("选中 #%1").arg(selectedRectIndex + 1));

        painter.save();// 保存当前画笔设置
        painter.setPen(QColor(255, 165, 0)); // 仅设置文本颜色 橙色
        painter.drawText(winCenter + QPoint(-18, 20),QString("(%1,%2)")
                         .arg(static_cast<int>(std::round(rectangles[selectedRectIndex].width()))) // 转换为整数像素坐标（对齐到最近整数）
                         .arg(static_cast<int>(std::round(rectangles[selectedRectIndex].height()))));
        painter.restore();// 恢复之前的画笔设置
    }
}



//鼠标按键按下行为
//选中矩形、绘制新矩形、移动图片、右键菜单、矩形缩放
void DrawingCanvas::mousePressEvent(QMouseEvent *event)
{
    lastMousePos = event->pos();
    currentPoint = event->pos(); //每次点击更新下当前鼠标的位置，有助于解决缩放矩形时鼠标移动出现的抖动

    if (event->button() == Qt::LeftButton) {

        if( isMoveToRecEdge && rectangles[selectedRectIndex].contains(windowToImage(event->pos())) ){ //用于矩形大小的缩放
            isRecResizing=true;
        }else
        {
            isRecResizing=false;
        }

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
        
        if(isRecResizing)
        {
            selectedRectIndex=clickRectNumber; //多个矩形重叠，选中某个矩形后，由此可调整该矩形的大小
        }
        
        // 如果没有选中矩形，开始绘制新矩形
        if (selectedRectIndex == -1) {
            isDrawing = true;
            startPoint = event->pos();
        }

    }else if (event->button() == Qt::RightButton) {
        //如何点击是否在图片区域内，根据点击的区域，显示不同的菜单
        if( getImageDisplayRect().contains(event->pos()) )
        {
            isClickImageRegion=true;
        } else {
            isClickImageRegion=false;
        }

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
            contextMenu.addAction(removeWhiteAction);
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
            contextMenu.addAction(removeAllWhiteAction);
            contextMenu.addAction(splitPictureAction);
            if(!isClickImageRegion)
            {
                contextMenu.addAction(addWhiteBorderOrigPictureAction);
            }
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



//鼠标移动行为
//拖动矩形、移动图片、矩形缩放
void DrawingCanvas::mouseMoveEvent(QMouseEvent *event)
{
    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();

    //若有矩形被选中，检测鼠标是否移动到边框上
    if (selectedRectIndex != -1 && !isRecResizing && rectangles[selectedRectIndex].contains(windowToImage(event->pos())) )
    {

        m_resizeEdge = getResizeEdge(windowToImage(event->pos()));
        setCursorForEdge(m_resizeEdge);//更新鼠标光标形状
        if (m_resizeEdge != None)
        {
            clickRectNumber=selectedRectIndex;
            isMoveToRecEdge=true;
        }else
        {
            isMoveToRecEdge=false;
        }
    }else
    {
        this->setCursor(Qt::ArrowCursor);
    }

    if (isDrawing || (isMoveToRecEdge && isRecResizing) ) {
        //绘制中的矩形
        //矩形的缩放
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


//鼠标按键释放行为
//左键释放后绘制新矩形
void DrawingCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        isRecResizing=false;
        isMoveToRecEdge=false;
        if (isDrawing) {
            // 完成绘制新矩形
            isDrawing = false;

            QPointF imgStart = windowToImage(startPoint);
            QPointF imgEnd = windowToImage(event->pos());


            if (!imgStart.isNull() || !imgEnd.isNull()) {
                QRectF newRect(imgStart, imgEnd);
                newRect = newRect.normalized();

                // 只添加有效矩形（最小尺寸）
                if (newRect.width() >= minimumRecLength && newRect.height() >= minimumRecLength) {
                    rectangles.append(newRect);
                    selectedRectIndex = rectangles.size() - 1;
                }
            }
            update();
        }
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


//
////
//下面是实现各种窗口中出现行为用到的函数
////
//

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
            displayImage = cv::imread(fileName.toStdString());
            // 重置缩放和平移，矩形框，判断变量
            zoomFactor = 1.0;
            panOffset = QPoint(0, 0);
            rectangles.clear();
            selectedRectIndex=-1;
            isDrawing = false;
            isPanning = false;
            isRecResizing = false;
            isMoveToRecEdge=false;
            update();
        } else {
            QMessageBox::warning(this, "加载错误", "无法加载图片: " + fileName);
            fileName="";
        }
    }
}

//利用画出的矩形切割图片，并保存
void DrawingCanvas::splitImageByRects(const cv::Mat &Image, const QString &imagePath,
                                      const QString &Dir, const QString &Prefix)
{
    if( Image.empty() || imagePath.isEmpty() )
    {
        QMessageBox::warning(this, "图片未加载", "请选择图片后再操作");
        return;
    }

    // 准备文件名组件
    QFileInfo fileInfo(imagePath);
     QString baseName;
    if(Prefix.isEmpty())
    {
        baseName = fileInfo.completeBaseName();
    }else
    {
        baseName = Prefix;
    }
    QString suffix = fileInfo.suffix();
    QString dirPath;
    if(Dir.isEmpty())
    {
        dirPath = fileInfo.absolutePath();
    }else
    {
        dirPath = Dir;
    }
    

    // 处理每个矩形区域
    for(int i = 0; i < rectangles.size(); ++i) {
        const QRectF& qrect = rectangles[i];

        // 转换为整数像素坐标（对齐到最近整数）
        int x = static_cast<int>(std::round(qrect.x()));
        int y = static_cast<int>(std::round(qrect.y()));
        int width = static_cast<int>(std::round(qrect.width()));
        int height = static_cast<int>(std::round(qrect.height()));

        // 边界检查
        if(x < 0) x = 0;
        if(y < 0) y = 0;
        if(x + width > Image.cols) width = Image.cols - x;
        if(y + height > Image.rows) height = Image.rows - y;

        // 验证有效区域
        if(width <= 0 || height <= 0) {
            qWarning("Invalid region at index %d: [%d, %d, %d, %d]",
                     i, x, y, width, height);
            continue;
        }

        // 提取ROI
        cv::Mat roi(Image, cv::Rect(x, y, width, height));

        // 构造输出文件名
        QString outputName = QString("%1/%2_%3.%4")
                             .arg(dirPath)
                             .arg(baseName)
                             //.arg(i+1, 2, 10, QLatin1Char('0')) // 两位数序号 01,02,03
                             .arg(QChar('a' + i)) // 字母 a,b,c
                             .arg(suffix);
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


// 检测鼠标在所选矩形的哪个边上
DrawingCanvas::ResizeEdge DrawingCanvas::getResizeEdge(const QPointF &pos)
{
    bool nearLeft = qAbs(pos.x() - rectangles[selectedRectIndex].left()) <= edgeMargin;
    bool nearRight = qAbs(pos.x() - rectangles[selectedRectIndex].right()) <= edgeMargin;
    bool nearTop = qAbs(pos.y() - rectangles[selectedRectIndex].top()) <= edgeMargin;
    bool nearBottom = qAbs(pos.y() - rectangles[selectedRectIndex].bottom()) <= edgeMargin;

    if (nearLeft && nearTop) return TopLeft;
    if (nearRight && nearTop) return TopRight;
    if (nearLeft && nearBottom) return BottomLeft;
    if (nearRight && nearBottom) return BottomRight;
    if (nearLeft) return Left;
    if (nearRight) return Right;
    if (nearTop) return Top;
    if (nearBottom) return Bottom;

    return None;
}


// 根据边框设置鼠标光标
void DrawingCanvas::setCursorForEdge(ResizeEdge edge)
{
    switch (edge) {
        case Left:
        case Right:
            this->setCursor(Qt::SizeHorCursor);
            break;
        case Top:
        case Bottom:
            this->setCursor(Qt::SizeVerCursor);
            break;
        case TopLeft:
        case BottomRight:
            this->setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
        case BottomLeft:
            this->setCursor(Qt::SizeBDiagCursor);
            break;
        default:
            this->setCursor(Qt::ArrowCursor);
    }
}

//去除选中矩形的白边
void DrawingCanvas::removeWhiteBorder(const cv::Mat &Image)
{
    if(Image.empty())
    {
        QMessageBox::warning(this, "图片未加载", "请选择图片后再操作");
        return;
    }

    cv::Mat img;
    img=Image(cv::Rect(rectangles[selectedRectIndex].topLeft().x(), rectangles[selectedRectIndex].topLeft().y(),
            rectangles[selectedRectIndex].width(), rectangles[selectedRectIndex].height()));

    QRectF rect;
    rect=imgContentRect(img); //得到的区域是相对于切割出来的图而言的，还需要转换到大图的坐标上
    //有些矩形所在内容全是白色，删除这样的矩形
    if( rect.width() == 2*removeWhitePadding && rect.height() == 2*removeWhitePadding )
    {
        rectangles.remove(selectedRectIndex); //删除矩形框
        selectedRectIndex = -1;
    }else
    {
        rectangles[selectedRectIndex].setRect(rectangles[selectedRectIndex].topLeft().x()+rect.x(),
                rectangles[selectedRectIndex].topLeft().y()+rect.y(),
                rect.width(),
                rect.height());
    }
    update();
}

//去除所有创建矩形所的白边
void DrawingCanvas::removeAllWhiteBorder(const cv::Mat &Image)
{
    if(Image.empty())
    {
        QMessageBox::warning(this, "图片未加载", "请选择图片后再操作");
        return;
    }
    cv::Mat img;
    QRectF rect;
    for(int i = 0; i < rectangles.size(); ++i)
    {
        img=Image(cv::Rect(rectangles[i].topLeft().x(), rectangles[i].topLeft().y(),
                rectangles[i].width(), rectangles[i].height()));
        rect=imgContentRect(img); //得到的区域是相对于切割出来的图而言的，还需要转换到大图的坐标上
        //有些矩形所在内容全是白色，删除这样的矩形
        if( rect.width() == 2*removeWhitePadding && rect.height() == 2*removeWhitePadding )
        {
            rectangles.remove(i); //删除矩形框
            i--;
        }else
        {
            rectangles[i].setRect(rectangles[i].topLeft().x()+rect.x(),
                    rectangles[i].topLeft().y()+rect.y(),
                    rect.width(),
                    rect.height());
        }
    }
    update();
}

//返回图片去除白边后的内容区域
QRectF DrawingCanvas::imgContentRect(const cv::Mat &img)
{
    // 转换为灰度图
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

    // 二值化处理：将暗色区域变为白色(255)，亮色区域变为黑色(0)
    cv::threshold(gray, gray, removeWhiteThreshold, 255, cv::THRESH_BINARY_INV);

    // 查找非零像素坐标
    std::vector<cv::Point> coords;
    cv::findNonZero(gray, coords);

    // 计算边界矩形
    cv::Rect boundingRect = cv::boundingRect(coords);


    // 添加内边距
    int x = std::max(0, boundingRect.x - removeWhitePadding);
    int y = std::max(0, boundingRect.y - removeWhitePadding);
    int w = std::min(img.cols - x, boundingRect.width + 2 * removeWhitePadding);
    int h = std::min(img.rows - y, boundingRect.height + 2 * removeWhitePadding);

    // 裁剪图像
    //cv::Mat rect = img(cv::Rect(x, y, w, h));// 裁剪图像
    //cv::imwrite("/home/shui/Music/cropwhitefromimage_result.png", rect());// 保存结果

    QRectF rect(x, y, w, h);
    return rect;
}

// Qt 用的图像格式转为 OpenCV 用的图像格式
cv::Mat DrawingCanvas::QPixmapToCvMat(const QPixmap& pixmap)
{
    // 将QPixmap转换为32位RGB格式的QImage
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_RGB888);

    // 创建cv::Mat并复制数据
    cv::Mat mat(image.height(), image.width(), CV_8UC3,
               const_cast<uchar*>(image.bits()),
               static_cast<size_t>(image.bytesPerLine()));

    // 克隆数据以避免Qt释放内存后出现悬空指针
    cv::Mat result = mat.clone();

    // OpenCV使用BGR格式，所以需要转换RGB->BGR
    cv::cvtColor(result, result, cv::COLOR_RGB2BGR);
    return result;
}

// OpenCV 用的图像格式转为 Qt 用的图像格式
QPixmap DrawingCanvas::CvMatToQPixmap(const cv::Mat& mat) {
    // 确保输入Mat有效
    if(mat.empty())
        return QPixmap();

    // 处理颜色空间：BGR->RGB 或 GRAY->RGB
    cv::Mat rgbMat;
    switch(mat.channels()) {
    case 1:
        cv::cvtColor(mat, rgbMat, cv::COLOR_GRAY2RGB);
        break;
    case 3:
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
        break;
    case 4:
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGRA2RGBA);
        break;
    default:
        return QPixmap(); // 不支持的通道数
    }

    // 创建QImage
    QImage image(rgbMat.data,
                rgbMat.cols,
                rgbMat.rows,
                static_cast<int>(rgbMat.step),
                (rgbMat.channels() == 4) ? QImage::Format_RGBA8888
                                         : QImage::Format_RGB888);

    // 必须复制数据，因为QImage不接管Mat的内存
    return QPixmap::fromImage(image.copy());
}

//为显示的原图增加白边
void DrawingCanvas::addWhiteBorderOrigPicture()
{
    //Add a white border around the cropped image
    cv::copyMakeBorder(displayImage, displayImage,
                   addWhiteBorderPaddingOrig, addWhiteBorderPaddingOrig,
                   addWhiteBorderPaddingOrig, addWhiteBorderPaddingOrig,
                   cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
    backgroundImage=CvMatToQPixmap(displayImage);
    //若已有矩形，则还需更新矩形坐标
    if(!rectangles.isEmpty())
    {
        for (QRectF& rect : rectangles) {
                rect.translate(addWhiteBorderPaddingOrig, addWhiteBorderPaddingOrig); //移动 (dx, dy)
            }
    }
}


