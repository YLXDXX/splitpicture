// drawingcanvas.cpp
// 处理图片显示、矩形绘制、鼠标交互、缩放平移等
#include "drawingcanvas.h"
#include "imageprocessor.h"

#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include <QClipboard>
#include <QMimeData>
#include <QKeyEvent>
#include <QApplication>

#include <opencv2/imgproc.hpp>   // 提供 cv::rectangle 和 cv::FILLED

// 构造函数：初始化成员变量和动作
DrawingCanvas::DrawingCanvas(QWidget *parent)
: QWidget(parent)
, m_autoAddWhitePadding(-1)
, m_selectedRectIndex(-1)
, m_isDrawing(false)
, m_isPanning(false)
, m_isRecResizing(false)
, m_isMoveToRecEdge(false)
, m_panOffset(0, 0)
, m_zoomFactor(1.0)
, m_resizeEdge(None)
, m_clickRectNumber(-1)
, m_backgroundColor(30, 30, 40)
, m_isClickImageRegion(false)
, m_programEdit("comicenhancerpro")
, m_programScale("upscayl")
, m_fillWhiteEnabled(false)   // 默认关闭
{
    setMouseTracking(true);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFocusPolicy(Qt::StrongFocus);   // 允许通过点击或 Tab 获得焦点

    // 创建动作
    m_deleteAction = new QAction("删除矩形", this);
    connect(m_deleteAction, &QAction::triggered, this, &DrawingCanvas::deleteSelectedRectangle); // 修正：连接到删除矩形

    m_deleteAllAction = new QAction("删除所有矩形", this);
    connect(m_deleteAllAction, &QAction::triggered, this, &DrawingCanvas::removeAllRectangles);

    m_loadImageAction = new QAction("加载背景图片", this);
    connect(m_loadImageAction, &QAction::triggered, this, &DrawingCanvas::loadBackgroundImage);

    m_resetZoomAction = new QAction("重置缩放", this);
    connect(m_resetZoomAction, &QAction::triggered, [this]() {
        m_zoomFactor = 1.0;
        m_panOffset = QPoint(0, 0);
        update();
    });

    m_splitPictureAction = new QAction("切割图片", this);
    connect(m_splitPictureAction, &QAction::triggered, this, &DrawingCanvas::splitImageByRects);

    m_removeWhiteAction = new QAction("去除白边", this);
    connect(m_removeWhiteAction, &QAction::triggered, this, &DrawingCanvas::removeWhiteBorder);

    m_removeAllWhiteAction = new QAction("去除所有白边", this);
    connect(m_removeAllWhiteAction, &QAction::triggered, this, &DrawingCanvas::removeAllWhiteBorder);

    m_addWhiteBorderOrigPictureAction = new QAction("原图加白边", this);
    connect(m_addWhiteBorderOrigPictureAction, &QAction::triggered, [this]() {
        addWhiteBorderToOriginal(DEFAULT_BORDER_PADDING);
    });

    // 文件监控定时器
    m_fileWatcherTimer = new QTimer(this);
    m_fileWatcherTimer->setInterval(150); // 150ms
    connect(m_fileWatcherTimer, &QTimer::timeout,
            this, &DrawingCanvas::checkFileModified);

    // 填白开关（始终显示，与是否在图片区域内无关）
    m_toggleFillWhiteAction = new QAction("填白", this);
    m_toggleFillWhiteAction->setCheckable(true);
    m_toggleFillWhiteAction->setChecked(false);
    connect(m_toggleFillWhiteAction, &QAction::triggered,
            this, &DrawingCanvas::toggleFillWhite);

    // 复制到剪贴板
    m_copyAction = new QAction("复制矩形内容", this);
    connect(m_copyAction, &QAction::triggered, this, &DrawingCanvas::copyRectToClipboard);

    // 填充并删除 / 填充外部并保留
    m_fillAndDelAction = new QAction("内部填白删除矩形", this);
    connect(m_fillAndDelAction, &QAction::triggered, this, &DrawingCanvas::fillRectAndDelete);

    m_fillOutsideAction = new QAction("外部填白保留矩形", this);
    connect(m_fillOutsideAction, &QAction::triggered, this, &DrawingCanvas::fillOutsideAndKeep);

    // 编辑图片
    m_processEditAction = new QAction("编辑", this);
    connect(m_processEditAction, &QAction::triggered, this, [this]() {
        if (m_fileName.isEmpty()) return;
        QString program = m_programEdit;
        QStringList args = { m_fileName };
        if (!QProcess::startDetached(program, args)) {
            QMessageBox::warning(this, "启动失败",
                                 "无法启动编辑程序: " + program + "\n请检查路径是否正确。");
        }
    });

    // 图片超分（二级子菜单）
    m_action2xSuperResolution = new QAction("2x", this);
    connect(m_action2xSuperResolution, &QAction::triggered, this, [this]() {
        if (m_fileName.isEmpty()) return;
        QString program = m_programScale;
        QStringList args = {"-i", m_fileName, "-o", m_fileName, "-s", "2", "-n", "digital-art-4x" };
        if (!QProcess::startDetached(program, args)) {
            QMessageBox::warning(this, "启动失败",
                                 "无法启动超分程序: " + program + "\n请检查路径是否正确。");
        }
    });

    m_action3xSuperResolution = new QAction("3x", this);
    connect(m_action3xSuperResolution, &QAction::triggered, this, [this]() {
        if (m_fileName.isEmpty()) return;
        QString program = m_programScale;
        QStringList args = {"-i", m_fileName, "-o", m_fileName, "-s", "3", "-n", "digital-art-4x" };
        if (!QProcess::startDetached(program, args)) {
            QMessageBox::warning(this, "启动失败",
                                 "无法启动超分程序: " + program + "\n请检查路径是否正确。");
        }
    });

    m_action4xSuperResolution = new QAction("4x", this);
    connect(m_action4xSuperResolution, &QAction::triggered, this, [this]() {
        if (m_fileName.isEmpty()) return;
        QString program = m_programScale;
        QStringList args = {"-i", m_fileName, "-o", m_fileName, "-s", "4", "-n", "digital-art-4x" };
        if (!QProcess::startDetached(program, args)) {
            QMessageBox::warning(this, "启动失败",
                                 "无法启动超分程序: " + program + "\n请检查路径是否正确。");
        }
    });

}

void DrawingCanvas::setProgramEdit(const QString& path) { m_programEdit = path; }
void DrawingCanvas::setProgramScale(const QString& path) { m_programScale = path; }


void DrawingCanvas::keyPressEvent(QKeyEvent *event) // 键盘事件
{

    if (event->key() == Qt::Key_Escape) {
        QApplication::quit(); // 按 esc 键退出整个应用程序
    }
    else if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_C) { //按 ctrl+c 键复制矩形内容
        if (m_selectedRectIndex >= 0 && m_selectedRectIndex < m_rectangles.size()) {
            DrawingCanvas::copyRectToClipboard();
        }
    }
    else {
        QWidget::keyPressEvent(event); // 其他按键交由父类处理
    }

    if (event->key() == Qt::Key_Escape) {
        QApplication::quit();   // 退出整个应用程序
    } else {
        QWidget::keyPressEvent(event); // 其他按键交由父类处理
    }
}

void DrawingCanvas::toggleFillWhite(bool checked)
{
    m_fillWhiteEnabled = checked;
}


void DrawingCanvas::fillRectWithWhite(const QRectF& rect) // 填充矩形内部为白色
{
    if (m_displayImage.empty())
        return;

    // 将 QRectF 转换为整数像素坐标，并限制在图像范围内
    int x = static_cast<int>(std::round(rect.x()));
    int y = static_cast<int>(std::round(rect.y()));
    int w = static_cast<int>(std::round(rect.width()));
    int h = static_cast<int>(std::round(rect.height()));

    // 边界裁剪
    x = std::max(0, x);
    y = std::max(0, y);
    w = std::min(m_displayImage.cols - x, w);
    h = std::min(m_displayImage.rows - y, h);

    if (w <= 0 || h <= 0)
        return;

    // 在 OpenCV 矩阵中填充白色矩形
    cv::rectangle(m_displayImage,
                  cv::Rect(x, y, w, h),
                  cv::Scalar(255, 255, 255),
                  cv::FILLED);

    // 更新显示的 QPixmap
    m_backgroundImage = ImageProcessor::cvMatToQPixmap(m_displayImage);

    // 触发重绘
    update();
}


void DrawingCanvas::fillOutsideRectWithWhite(const QRectF& rect) // 填充矩形外部为白色，内部保留原样
{
    if (m_displayImage.empty())
        return;

    // 1. 创建一个全白图像（与原图尺寸相同）
    cv::Mat whiteBg(m_displayImage.size(), m_displayImage.type(), cv::Scalar(255, 255, 255));

    // 2. 将原图中的矩形区域复制到白色背景的相同位置
    int x = static_cast<int>(std::round(rect.x()));
    int y = static_cast<int>(std::round(rect.y()));
    int w = static_cast<int>(std::round(rect.width()));
    int h = static_cast<int>(std::round(rect.height()));

    // 边界裁剪（确保在图像范围内）
    x = std::max(0, x);
    y = std::max(0, y);
    w = std::min(m_displayImage.cols - x, w);
    h = std::min(m_displayImage.rows - y, h);

    if (w > 0 && h > 0) {
        cv::Rect roi(x, y, w, h);
        m_displayImage(roi).copyTo(whiteBg(roi));
    }

    // 3. 更新显示图像
    m_displayImage = whiteBg.clone();
    m_backgroundImage = ImageProcessor::cvMatToQPixmap(m_displayImage);
    update();
}

void DrawingCanvas::fillRectAndDelete()
{
    if (m_selectedRectIndex < 0 || m_selectedRectIndex >= m_rectangles.size())
        return;

    // 1. 填充选中矩形内部为白色
    fillRectWithWhite(m_rectangles[m_selectedRectIndex]);

    // 2. 删除该矩形
    removeRectangle(m_selectedRectIndex);
}

void DrawingCanvas::fillOutsideAndKeep()
{
    if (m_selectedRectIndex < 0 || m_selectedRectIndex >= m_rectangles.size())
        return;

    // 填充矩形外部为白色，内部保留原样
    fillOutsideRectWithWhite(m_rectangles[m_selectedRectIndex]);

    // 矩形框保持不变，不需要删除
}


// ---------- 图片操作 ----------
void DrawingCanvas::setBackgroundImage(const QPixmap& pixmap, const cv::Mat& cvImage)
{
    m_backgroundImage = pixmap;
    m_displayImage = cvImage.clone();

    // 自动加白边（如有设置）
    if (m_autoAddWhitePadding > 0) {
        addWhiteBorderToOriginal(m_autoAddWhitePadding);
    }

    // 重置视图和矩形
    m_zoomFactor = 1.0;
    m_panOffset = QPoint(0, 0);
    m_rectangles.clear();
    m_selectedRectIndex = -1;
    m_isDrawing = false;
    m_isPanning = false;
    m_isRecResizing = false;
    m_isMoveToRecEdge = false;

    update();
}

void DrawingCanvas::clearImage()
{
    stopFileWatching();
    m_backgroundImage = QPixmap();
    m_displayImage = cv::Mat();
    m_fileName.clear();
    m_rectangles.clear();
    m_selectedRectIndex = -1;
    update();
}

bool DrawingCanvas::hasImage() const
{
    return !m_backgroundImage.isNull();
}

// ---------- 矩形管理 ----------
void DrawingCanvas::addRectangle(const QRectF& rect)
{
    m_rectangles.append(rect.normalized());
    m_selectedRectIndex = m_rectangles.size() - 1;
    update();
}

void DrawingCanvas::removeRectangle(int index)
{
    if (index >= 0 && index < m_rectangles.size()) {
        m_rectangles.remove(index);
        if (m_selectedRectIndex == index)
            m_selectedRectIndex = -1;
        else if (m_selectedRectIndex > index)
            m_selectedRectIndex--;
        update();
    }
}

void DrawingCanvas::removeAllRectangles()
{
    m_rectangles.clear();
    m_selectedRectIndex = -1;
    update();
}

const QVector<QRectF>& DrawingCanvas::rectangles() const
{
    return m_rectangles;
}

// ---------- 参数设置 ----------
void DrawingCanvas::setOutputFilePrefix(const QString& prefix)
{
    m_outputFilePrefix = prefix;
}

QString DrawingCanvas::outputFilePrefix() const
{
    return m_outputFilePrefix;
}

void DrawingCanvas::setOutputDirectory(const QString& dir)
{
    m_outFileDirectory = dir;
}

QString DrawingCanvas::outputDirectory() const
{
    return m_outFileDirectory;
}

void DrawingCanvas::setAutoAddWhiteBorderPadding(int padding)
{
    m_autoAddWhitePadding = padding;
}

int DrawingCanvas::autoAddWhiteBorderPadding() const
{
    return m_autoAddWhitePadding;
}

// ---------- 公共槽函数 ----------
void DrawingCanvas::loadBackgroundImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择背景图片", "",
                                                    "图片文件 (*.png *.jpg *.jpeg *.bmp)");
    if (fileName.isEmpty())
        return;

    QPixmap pixmap;
    if (!pixmap.load(fileName)) {
        QMessageBox::warning(this, "加载错误", "无法加载图片: " + fileName);
        return;
    }

    cv::Mat cvImage = ImageProcessor::loadImage(fileName);
    if (cvImage.empty()) {
        QMessageBox::warning(this, "加载错误", "OpenCV无法加载图片: " + fileName);
        return;
    }

    m_fileName = fileName;
    setBackgroundImage(pixmap, cvImage);

    startFileWatching();

    // 更新窗口标题
    QFileInfo info(fileName);
    emit titleChangeRequested("SplitPicture & " + info.completeBaseName());
    emit imageLoaded(fileName);
}

void DrawingCanvas::splitImageByRects()
{
    if (m_displayImage.empty() || m_fileName.isEmpty()) {
        QMessageBox::warning(this, "图片未加载", "请选择图片后再操作");
        return;
    }

    QFileInfo fileInfo(m_fileName);
    QString baseName = m_outputFilePrefix.isEmpty() ? fileInfo.completeBaseName() : m_outputFilePrefix;
    QString suffix = fileInfo.suffix();
    QString dirPath = m_outFileDirectory.isEmpty() ? fileInfo.absolutePath() : m_outFileDirectory;

    for (int i = 0; i < m_rectangles.size(); ++i) {
        cv::Mat roi = ImageProcessor::extractROI(m_displayImage, m_rectangles[i]);
        if (roi.empty())
            continue;

        QString outputName;
        if (m_rectangles.size() == 1) {
            outputName = QString("%1/%2.%3").arg(dirPath).arg(baseName).arg(suffix);
        } else {
            outputName = QString("%1/%2%3.%4").arg(dirPath).arg(baseName)
            .arg(QChar('a' + i)).arg(suffix);
        }

        if (!ImageProcessor::saveImage(roi, outputName)) {
            qWarning("Failed to write: %s", qPrintable(outputName));
            QMessageBox::warning(this, "图片保存失败", "无法保存: " + outputName);
        }
    }
}

void DrawingCanvas::removeWhiteBorder()
{
    if (m_displayImage.empty()) {
        QMessageBox::warning(this, "图片未加载", "请选择图片后再操作");
        return;
    }
    if (m_selectedRectIndex < 0 || m_selectedRectIndex >= m_rectangles.size())
        return;

    const QRectF& rect = m_rectangles[m_selectedRectIndex];
    cv::Mat roi = ImageProcessor::extractROI(m_displayImage, rect);
    if (roi.empty())
        return;

    QRectF content = ImageProcessor::getContentRect(roi, REMOVE_WHITE_THRESHOLD, REMOVE_WHITE_PADDING);
    if (content.width() <= 2 * REMOVE_WHITE_PADDING && content.height() <= 2 * REMOVE_WHITE_PADDING) {
        // 全白，删除矩形
        removeRectangle(m_selectedRectIndex);
    } else {
        // 调整矩形位置和大小
        QRectF newRect(rect.topLeft().x() + content.x(),
                       rect.topLeft().y() + content.y(),
                       content.width(), content.height());
        m_rectangles[m_selectedRectIndex] = newRect.normalized();
        update();
    }
}

void DrawingCanvas::removeAllWhiteBorder()
{
    if (m_displayImage.empty()) {
        QMessageBox::warning(this, "图片未加载", "请选择图片后再操作");
        return;
    }

    for (int i = m_rectangles.size() - 1; i >= 0; --i) {
        const QRectF& rect = m_rectangles[i];
        cv::Mat roi = ImageProcessor::extractROI(m_displayImage, rect);
        if (roi.empty())
            continue;

        QRectF content = ImageProcessor::getContentRect(roi, REMOVE_WHITE_THRESHOLD, REMOVE_WHITE_PADDING);
        if (content.width() <= 2 * REMOVE_WHITE_PADDING && content.height() <= 2 * REMOVE_WHITE_PADDING) {
            removeRectangle(i);
        } else {
            QRectF newRect(rect.topLeft().x() + content.x(),
                           rect.topLeft().y() + content.y(),
                           content.width(), content.height());
            m_rectangles[i] = newRect.normalized();
        }
    }
    update();
}

void DrawingCanvas::addWhiteBorderToOriginal(int padding)
{
    if (m_displayImage.empty())
        return;

    m_displayImage = ImageProcessor::addWhiteBorder(m_displayImage, padding);
    m_backgroundImage = ImageProcessor::cvMatToQPixmap(m_displayImage);
    updateRectanglesAfterBorderAdded(padding);
    update();
}

// ---------- 事件处理 ----------
void DrawingCanvas::paintEvent(QPaintEvent *event)
{
    QElapsedTimer timer;
    timer.start();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setFont(QFont("Arial", 10, QFont::Bold));

    // 背景色
    painter.fillRect(rect(), m_backgroundColor);

    if (!m_backgroundImage.isNull()) {
        QRect displayRect = getImageDisplayRect();
        painter.drawPixmap(displayRect, m_backgroundImage);
        painter.setPen(QPen(QColor(150, 150, 170), 1));
        painter.drawRect(displayRect);

        // 图片信息
        painter.setPen(Qt::white);
        QString info = QString("图片: %1x%2 | 显示: %3x%4 | 缩放: %5%")
        .arg(m_backgroundImage.width())
        .arg(m_backgroundImage.height())
        .arg(displayRect.width())
        .arg(displayRect.height())
        .arg(int(m_zoomFactor * 100));
        painter.drawText(10, height() - 10, info);
    } else {
        // 网格背景
        painter.setPen(QPen(QColor(60, 60, 70), 1));
        for (int x = 0; x < width(); x += 20)
            painter.drawLine(x, 0, x, height());
        for (int y = 0; y < height(); y += 20)
            painter.drawLine(0, y, width(), y);
    }

    // 绘制所有矩形
    for (int i = 0; i < m_rectangles.size(); ++i) {
        const QRectF &rect = m_rectangles[i];
        QColor rectColor = (i == m_selectedRectIndex) ? QColor(255, 215, 0, 120) : QColor(70, 130, 180, 100);
        QColor borderColor = (i == m_selectedRectIndex) ? Qt::yellow : Qt::white;

        painter.setBrush(rectColor);
        painter.setPen(QPen(borderColor, 1));

        QPoint topLeft = imageToWindow(rect.topLeft());
        QPoint bottomRight = imageToWindow(rect.bottomRight());
        painter.drawRect(QRect(topLeft, bottomRight));

        // 编号
        QPoint center = imageToWindow(rect.center());
        painter.setPen(Qt::white);
        painter.drawText(center, QString::number(i + 1));
    }

    // 正在绘制的矩形
    if (m_isDrawing) {
        QPointF imgStart = windowToImage(m_startPoint);
        QPointF imgCurrent = windowToImage(m_currentPoint);

        if (!imgStart.isNull() && !imgCurrent.isNull()) {
            QRectF imgRect(imgStart, imgCurrent);
            painter.setBrush(QBrush(QColor(255, 100, 100, 80)));
            painter.setPen(QPen(Qt::red, 2, Qt::DashLine));

            QPoint winStart = imageToWindow(imgRect.topLeft());
            QPoint winEnd = imageToWindow(imgRect.bottomRight());
            painter.drawRect(QRect(winStart, winEnd));

            painter.setPen(Qt::white);
            QString sizeText = QString("%1 x %2")
            .arg(fabs(imgRect.width()), 0, 'f', 1)
            .arg(fabs(imgRect.height()), 0, 'f', 1);
            painter.drawText(winStart + QPoint(5, -5), sizeText);
        }
    }

    // 矩形缩放时的实时更新
    if (m_isMoveToRecEdge && m_isRecResizing && m_selectedRectIndex == m_clickRectNumber) {
        // 已经在 mouseMoveEvent 中更新矩形，这里无需额外绘制，但需要触发update
    }

    // 状态信息
    painter.setPen(Qt::white);
    if (m_backgroundImage.isNull()) {
        painter.drawText(10, 20, "左键点击并拖动: 绘制新矩形");
        painter.drawText(10, 40, "右键点击矩形: 选择/删除矩形");
        painter.drawText(10, 60, "拖动已选矩形: 移动矩形位置");
        painter.drawText(10, 80, "滚轮: 缩放视图 | 中键: 平移视图");
        painter.drawText(10, 100, QString("矩形数量: %1 | 绘制时间: %2ms")
        .arg(m_rectangles.size()).arg(timer.elapsed()));
    } else {
        painter.drawText(10, 20, QString("矩形数量: %1 | 绘制时间: %2ms")
        .arg(m_rectangles.size()).arg(timer.elapsed()));
        painter.drawText(10, 40, QString("Output: %1").arg(m_outFileDirectory));
        painter.drawText(10, 60, QString("Prefix: %1").arg(m_outputFilePrefix));
    }

    // 选中矩形的尺寸提示
    if (m_selectedRectIndex >= 0 && m_selectedRectIndex < m_rectangles.size()) {
        painter.setPen(QColor(255, 165, 0));
        QPointF center = m_rectangles[m_selectedRectIndex].center();
        QPoint winCenter = imageToWindow(center);
        painter.drawText(winCenter + QPoint(-18, 20),
                         QString("(%1,%2)")
                         .arg(static_cast<int>(std::round(m_rectangles[m_selectedRectIndex].width())))
                         .arg(static_cast<int>(std::round(m_rectangles[m_selectedRectIndex].height()))));
    }
}

void DrawingCanvas::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
    m_currentPoint = event->pos();

    if (event->button() == Qt::LeftButton) {
        // --- 第一步：处理边缘缩放（必须优先）---
        if (m_isMoveToRecEdge && m_selectedRectIndex >= 0 &&
            m_rectangles[m_selectedRectIndex].contains(windowToImage(event->pos()))) {

            m_isRecResizing = true;
        m_clickRectNumber = m_selectedRectIndex;
        // ★★★ 重要：保存当前矩形的原始几何数据，供鼠标移动时计算新矩形 ★★★
        m_resizeOriginalRect = m_rectangles[m_selectedRectIndex];
            } else {
                m_isRecResizing = false;
            }

            // --- 第二步：仅当非缩放状态时，才进行“选中矩形”检测 ---
            if (!m_isRecResizing) {
                m_selectedRectIndex = -1;
                for (int i = 0; i < m_rectangles.size(); ++i) {
                    QPointF imgPos = windowToImage(event->pos());
                    if (!imgPos.isNull() && m_rectangles[i].contains(imgPos)) {
                        m_selectedRectIndex = i;
                        m_dragOffset = m_rectangles[i].topLeft() - imgPos;
                        update();
                        break;
                    }
                }
            }

            // --- 第三步：如果既未缩放也未选中矩形，开始绘制新矩形 ---
            if (m_selectedRectIndex == -1 && !m_isRecResizing) {
                m_isDrawing = true;
                m_startPoint = event->pos();
            }
    }
    else if (event->button() == Qt::RightButton) {
        // 判断点击是否在图片显示区域内（用于右键菜单）
        m_isClickImageRegion = getImageDisplayRect().contains(event->pos());

        // 右键选中矩形
        int newSelection = -1;
        for (int i = 0; i < m_rectangles.size(); ++i) {
            QPointF imgPos = windowToImage(event->pos());
            if (!imgPos.isNull() && m_rectangles[i].contains(imgPos)) {
                newSelection = i;
                break;
            }
        }

        QMenu contextMenu(this);
        if (newSelection >= 0) {
            m_selectedRectIndex = newSelection;

            contextMenu.addAction(m_removeWhiteAction);
            contextMenu.addAction(m_copyAction);
            contextMenu.addAction(m_fillAndDelAction);
            contextMenu.addAction(m_fillOutsideAction);

            contextMenu.addAction(m_deleteAction);
            contextMenu.addAction("取消选择", [this]() {
                m_selectedRectIndex = -1;
                update();
            });
        } else {

            if (!m_isClickImageRegion) {
                contextMenu.addAction(m_removeAllWhiteAction);
                contextMenu.addAction(m_resetZoomAction);
                contextMenu.addAction(m_addWhiteBorderOrigPictureAction);
                contextMenu.addAction(m_loadImageAction);
            } else {
                contextMenu.addAction(m_removeAllWhiteAction);
                contextMenu.addAction(m_deleteAllAction);
                contextMenu.addAction(m_splitPictureAction);
                contextMenu.addAction(m_resetZoomAction);
            }

            // ---------- 新增：调用外部程序 ----------
            if (hasImage()) {
                contextMenu.addSeparator();


                contextMenu.addAction(m_processEditAction);

                QMenu *subMenuSuperResolution = contextMenu.addMenu("超分");
                subMenuSuperResolution->addAction(m_action2xSuperResolution);
                subMenuSuperResolution->addAction(m_action3xSuperResolution);
                subMenuSuperResolution->addAction(m_action4xSuperResolution);
            }

            contextMenu.addSeparator();
            contextMenu.addAction(m_toggleFillWhiteAction);

            m_selectedRectIndex = -1;
        }
        contextMenu.exec(event->globalPosition().toPoint());
        update();
    }
    else if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_panStartPoint = event->pos();
    }
}

void DrawingCanvas::mouseMoveEvent(QMouseEvent *event)
{
    QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();

    // 检测鼠标是否在选中矩形的边缘
    if (m_selectedRectIndex != -1 && !m_isRecResizing &&
        m_rectangles[m_selectedRectIndex].contains(windowToImage(event->pos()))) {
        m_resizeEdge = getResizeEdge(windowToImage(event->pos()));
    setCursorForEdge(m_resizeEdge);
    if (m_resizeEdge != None) {
        m_clickRectNumber = m_selectedRectIndex;
        m_isMoveToRecEdge = true;
    } else {
        m_isMoveToRecEdge = false;
    }
        } else {
            setCursor(Qt::ArrowCursor);
        }

        if (m_isDrawing || (m_isMoveToRecEdge && m_isRecResizing)) {
            m_currentPoint = event->pos();
            // 缩放矩形实时更新
            if (m_isRecResizing && m_selectedRectIndex == m_clickRectNumber) {
                QPointF imgCurrent = windowToImage(event->pos());
                QRectF newRect = m_resizeOriginalRect;
                QPointF delta;
                switch (m_resizeEdge) {
                    case Left:
                        delta = imgCurrent - m_resizeOriginalRect.bottomLeft();
                        newRect.setLeft(newRect.left() + delta.x());
                        break;
                    case Right:
                        delta = imgCurrent - m_resizeOriginalRect.bottomRight();
                        newRect.setRight(newRect.right() + delta.x());
                        break;
                    case Top:
                        delta = imgCurrent - m_resizeOriginalRect.topRight();
                        newRect.setTop(newRect.top() + delta.y());
                        break;
                    case Bottom:
                        delta = imgCurrent - m_resizeOriginalRect.bottomRight();
                        newRect.setBottom(newRect.bottom() + delta.y());
                        break;
                    case TopLeft:
                        delta = imgCurrent - m_resizeOriginalRect.topLeft();
                        newRect.setTopLeft(newRect.topLeft() + delta);
                        break;
                    case TopRight:
                        delta = imgCurrent - m_resizeOriginalRect.topRight();
                        newRect.setTopRight(newRect.topRight() + delta);
                        break;
                    case BottomLeft:
                        delta = imgCurrent - m_resizeOriginalRect.bottomLeft();
                        newRect.setBottomLeft(newRect.bottomLeft() + delta);
                        break;
                    case BottomRight:
                        delta = imgCurrent - m_resizeOriginalRect.bottomRight();
                        newRect.setBottomRight(newRect.bottomRight() + delta);
                        break;
                    default:
                        break;
                }
                if (newRect.width() > MINIMUM_RECT_LENGTH && newRect.height() > MINIMUM_RECT_LENGTH) {
                    m_rectangles[m_selectedRectIndex] = newRect.normalized();
                }
            }
            update();
        }
        else if (m_selectedRectIndex >= 0 && (event->buttons() & Qt::LeftButton) && !m_isRecResizing) {
            // 移动矩形
            QPointF imgPos = windowToImage(event->pos());
            if (!imgPos.isNull()) {
                QPointF newPos = imgPos + m_dragOffset;
                QRectF movedRect = m_rectangles[m_selectedRectIndex];
                movedRect.moveTo(newPos);
                // 限制在图片范围内
                if (hasImage()) {
                    if (movedRect.left() < 0)
                        newPos.setX(0);
                    if (movedRect.top() < 0)
                        newPos.setY(0);
                    if (movedRect.right() > m_backgroundImage.width())
                        newPos.setX(m_backgroundImage.width() - movedRect.width());
                    if (movedRect.bottom() > m_backgroundImage.height())
                        newPos.setY(m_backgroundImage.height() - movedRect.height());
                }
                m_rectangles[m_selectedRectIndex].moveTo(newPos);
                update();
            }
        }
        else if (m_isPanning && (event->buttons() & Qt::MiddleButton)) {
            m_panOffset += delta;
            update();
        }
}

void DrawingCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isRecResizing = false;
        m_isMoveToRecEdge = false;
        if (m_isDrawing) {
            m_isDrawing = false;
            QPointF imgStart = windowToImage(m_startPoint);
            QPointF imgEnd = windowToImage(event->pos());
            if (!imgStart.isNull() && !imgEnd.isNull()) {
                QRectF newRect(imgStart, imgEnd);
                newRect = newRect.normalized();
                if (newRect.width() >= MINIMUM_RECT_LENGTH && newRect.height() >= MINIMUM_RECT_LENGTH) {

                    // ★★★ 填白模式只填充，不添加矩形框；否则正常添加 ★★★
                    if (m_fillWhiteEnabled) {
                        fillRectWithWhite(newRect);
                    } else {
                        addRectangle(newRect);
                    }

                }
            }
            update();
        }
    }
    else if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
    }
}

void DrawingCanvas::wheelEvent(QWheelEvent *event)
{
    QPointF center = windowToImage(event->position().toPoint());
    if (center.isNull())
        return;

    double zoomAmount = 1.1;
    if (event->angleDelta().y() < 0)
        zoomAmount = 1.0 / zoomAmount;

    QPointF imgPosBeforeZoom = center;
    double newZoomFactor = m_zoomFactor * zoomAmount;
    newZoomFactor = qBound(0.1, newZoomFactor, 10.0);

    QPointF imgPosAfterZoom = imgPosBeforeZoom;
    QPoint mousePos = event->position().toPoint();
    QPointF imgPosAfterZoomWindow = imageToWindow(imgPosAfterZoom);
    m_panOffset += (mousePos - imgPosAfterZoomWindow.toPoint());
    m_zoomFactor = newZoomFactor;
    update();
}

void DrawingCanvas::resizeEvent(QResizeEvent *event)
{
    QSize oldSize = event->oldSize();
    QSize newSize = size();
    if (oldSize.isValid()) {
        m_panOffset += QPoint((newSize.width() - oldSize.width()) / 2,
                              (newSize.height() - oldSize.height()) / 2);
    }
    QWidget::resizeEvent(event);
}

// ---------- 坐标转换 ----------
QRect DrawingCanvas::getImageDisplayRect() const
{
    if (m_backgroundImage.isNull())
        return QRect(0, 0, width(), height());

    QSize imageSize = m_backgroundImage.size();
    QSize scaledSize = imageSize.scaled(width() * m_zoomFactor, height() * m_zoomFactor,
                                        Qt::KeepAspectRatio);
    int x = (width() - scaledSize.width()) / 2 + m_panOffset.x();
    int y = (height() - scaledSize.height()) / 2 + m_panOffset.y();
    return QRect(x, y, scaledSize.width(), scaledSize.height());
}

QPointF DrawingCanvas::windowToImage(const QPoint& windowPos) const
{
    if (m_backgroundImage.isNull())
        return QPointF(windowPos);

    QRect displayRect = getImageDisplayRect();
    QPoint clampedPos = windowPos;

    // 将窗口坐标限制在显示区域内
    if (!displayRect.contains(windowPos)) {
        if (windowPos.x() > displayRect.x() + displayRect.width())
            clampedPos.setX(displayRect.x() + displayRect.width());
        else if (windowPos.x() < displayRect.x())
            clampedPos.setX(displayRect.x());

        if (windowPos.y() > displayRect.y() + displayRect.height())
            clampedPos.setY(displayRect.y() + displayRect.height());
        else if (windowPos.y() < displayRect.y())
            clampedPos.setY(displayRect.y());
    }

    double scaleX = static_cast<double>(m_backgroundImage.width()) / displayRect.width();
    double scaleY = static_cast<double>(m_backgroundImage.height()) / displayRect.height();

    return QPointF((clampedPos.x() - displayRect.x()) * scaleX,
                   (clampedPos.y() - displayRect.y()) * scaleY);
}

QPoint DrawingCanvas::imageToWindow(const QPointF& imagePos) const
{
    if (m_backgroundImage.isNull())
        return imagePos.toPoint();

    QRect displayRect = getImageDisplayRect();
    double scaleX = static_cast<double>(displayRect.width()) / m_backgroundImage.width();
    double scaleY = static_cast<double>(displayRect.height()) / m_backgroundImage.height();

    return QPoint(displayRect.x() + static_cast<int>(imagePos.x() * scaleX),
                  displayRect.y() + static_cast<int>(imagePos.y() * scaleY));
}

// ---------- 交互辅助 ----------
DrawingCanvas::ResizeEdge DrawingCanvas::getResizeEdge(const QPointF& pos) const
{
    if (m_selectedRectIndex < 0 || m_selectedRectIndex >= m_rectangles.size())
        return None;

    const QRectF& rect = m_rectangles[m_selectedRectIndex];
    bool nearLeft = qAbs(pos.x() - rect.left()) <= EDGE_MARGIN;
    bool nearRight = qAbs(pos.x() - rect.right()) <= EDGE_MARGIN;
    bool nearTop = qAbs(pos.y() - rect.top()) <= EDGE_MARGIN;
    bool nearBottom = qAbs(pos.y() - rect.bottom()) <= EDGE_MARGIN;

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

void DrawingCanvas::setCursorForEdge(ResizeEdge edge)
{
    switch (edge) {
        case Left: case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case Top: case Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case TopLeft: case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight: case BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
    }
}

void DrawingCanvas::updateRectanglesAfterBorderAdded(int padding)
{
    for (QRectF& rect : m_rectangles) {
        rect.translate(padding, padding);
    }
}

void DrawingCanvas::deleteSelectedRectangle()
{
    if (m_selectedRectIndex >= 0 && m_selectedRectIndex < m_rectangles.size()) {
        m_rectangles.remove(m_selectedRectIndex);
        m_selectedRectIndex = -1;
        update();
    }
}

void DrawingCanvas::triggerTitleChange(const QString& title)
{
    emit titleChangeRequested(title);
}




// 文件监控核心函数
void DrawingCanvas::startFileWatching()
{
    if (!m_fileName.isEmpty()) {
        QFileInfo info(m_fileName);
        if (info.exists()) {
            m_fileLastModified = info.lastModified();
            m_fileWatcherTimer->start();
        }
    }
}

void DrawingCanvas::stopFileWatching()
{
    m_fileWatcherTimer->stop();
}

void DrawingCanvas::checkFileModified()
{
    if (m_fileName.isEmpty() || !m_fileWatcherTimer->isActive())
        return;

    QFileInfo info(m_fileName);
    if (!info.exists())
        return;

    QDateTime lastModified = info.lastModified();
    if (lastModified != m_fileLastModified) {
        reloadImageFromFile();
    }
}

void DrawingCanvas::reloadImageFromFile()
{
    if (m_fileName.isEmpty())
        return;


    QFileInfo checkFile(m_fileName);
    if (!checkFile.exists() || !checkFile.isReadable()) {
        QMessageBox::warning(this, "重新加载失败",
                             "文件不存在或不可读: " + m_fileName);
        return;
    }


    QPixmap pixmap;
    QThread::msleep(150); // 等待 150ms 后再进行读取，原文件的修改需要时间完全写入
    if (!pixmap.load(m_fileName)) {
        QMessageBox::warning(this, "重新加载失败",
                             "Qt 无法加载图片 (格式不支持或文件损坏): " + m_fileName);
        return;
    }

    cv::Mat cvImage = ImageProcessor::loadImage(m_fileName);
    if (cvImage.empty()) {
        QMessageBox::warning(this, "重新加载失败",
                             "OpenCV 无法加载图片 (格式不支持或依赖缺失): " + m_fileName);
        return;
    }


    // 成功加载
    stopFileWatching();  // 停止监控，避免 setBackgroundImage 中可能的重入
    setBackgroundImage(pixmap, cvImage); // 重新设置图像（会自动重置缩放、平移、清除矩形）
    startFileWatching(); // 重新启动监控（setBackgroundImage 不会启动监控，由我们手动启动）
    emit titleChangeRequested("SplitPicture & " + checkFile.completeBaseName());
}


void DrawingCanvas::copyRectToClipboard()
{
    if (m_selectedRectIndex < 0 || m_selectedRectIndex >= m_rectangles.size())
        return;

    if (m_displayImage.empty()) {
        QMessageBox::warning(this, "错误", "没有可用的图像数据。");
        return;
    }

    const QRectF& rect = m_rectangles[m_selectedRectIndex];
    cv::Mat roi = ImageProcessor::extractROI(m_displayImage, rect);
    if (roi.empty()) {
        QMessageBox::warning(this, "错误", "无法提取矩形区域。");
        return;
    }

    QPixmap pixmap = ImageProcessor::cvMatToQPixmap(roi);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "错误", "转换图像失败。");
        return;
    }


    // ---------- 优先尝试使用 wl-copy (Wayland 原生剪贴板工具) ----------
    QString wlCopy = QStandardPaths::findExecutable("wl-copy");
    if (!wlCopy.isEmpty()) {
        QTemporaryFile tempFile;
        if (tempFile.open()) {
            if (pixmap.save(&tempFile, "PNG")) {
                tempFile.close(); // 确保数据写入磁盘
                QProcess process;
                process.setStandardInputFile(tempFile.fileName()); // 将进程的标准输入重定向到临时文件：即进程会读取该文件的内容作为其标准输入
                process.start(wlCopy, QStringList());
                if (process.waitForFinished(500) && process.exitCode() == 0) // 等待进程结束，超时 500 毫秒（0.5 秒）。如果进程正常结束且退出码为 0，则认为成功。
                {
                    return; // 成功复制，直接返回
                }
            }
        }
        // wl-copy 失败，继续尝试 Qt 剪贴板
    }



    // 将图像编码为 PNG 字节数组
    QByteArray byteArray;                 // <--- 必须声明
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    if (!pixmap.save(&buffer, "PNG")) {
        QMessageBox::warning(this, "错误", "无法编码图像数据。");
        return;
    }
    buffer.close();

    // 使用 QMimeData 设置图像数据，确保系统剪贴板持有数据
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("image/png", byteArray);
    mimeData->setImageData(pixmap.toImage()); // 转换为 QImage 并设置

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setMimeData(mimeData); // 剪贴板接管所有权
}

