// drawingcanvas.h
// 处理图片显示、矩形绘制、鼠标交互、缩放平移等
#ifndef DRAWINGCANVAS_H
#define DRAWINGCANVAS_H

#include <QWidget>
#include <QPixmap>
#include <QVector>
#include <QRectF>
#include <QAction>
#include <QColor>
#include <QTimer>
#include <QDateTime>
#include <QProcess>

#include <opencv2/core.hpp>

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QWheelEvent;
class QPaintEvent;
QT_END_NAMESPACE

class DrawingCanvas : public QWidget
{
    Q_OBJECT

public:
    enum ResizeEdge { None, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };

    explicit DrawingCanvas(QWidget *parent = nullptr);

    // 图片操作
    void setBackgroundImage(const QPixmap& pixmap, const cv::Mat& cvImage);
    void clearImage();
    bool hasImage() const;

    // 矩形管理
    void addRectangle(const QRectF& rect);
    void removeRectangle(int index);
    void removeAllRectangles();
    const QVector<QRectF>& rectangles() const;

    // 参数设置
    void setOutputFilePrefix(const QString& prefix);
    void setOutputDirectory(const QString& dir);
    QString outputFilePrefix() const;
    QString outputDirectory() const;

    // 命令行加载时自动加白边
    void setAutoAddWhiteBorderPadding(int padding);
    int autoAddWhiteBorderPadding() const;

    void setProgramEdit(const QString& path);
    void setProgramScale(const QString& path);
    void setFileName(const QString& name) { m_fileName = name; }
    QString getFileName() const { return m_fileName; }

public slots:
    void loadBackgroundImage();           // 从文件对话框加载
    void splitImageByRects();            // 切割图片
    void removeWhiteBorder();            // 去除选中矩形白边
    void removeAllWhiteBorder();         // 去除所有矩形白边
    void addWhiteBorderToOriginal(int padding = 50); // 为原图加白边
    void deleteSelectedRectangle();
    void triggerTitleChange(const QString& title);
    void startFileWatching();      // 启动文件监控
    void stopFileWatching();       // 停止文件监控

private slots:
    void checkFileModified();      // 定时检查文件修改
    void toggleFillWhite(bool checked);   // 切换填白模式
    void fillRectAndDelete();      // 填充矩形内部 + 删除矩形
    void fillOutsideAndKeep();    // 填充矩形外部 + 保留矩形
    void copyRectToClipboard();

signals:
    void titleChangeRequested(const QString& newTitle);
    void imageLoaded(const QString& fileName);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    // 坐标转换
    QRect getImageDisplayRect() const;
    QPointF windowToImage(const QPoint& windowPos) const;
    QPoint imageToWindow(const QPointF& imagePos) const;

    // 交互辅助
    ResizeEdge getResizeEdge(const QPointF& pos) const;
    void setCursorForEdge(ResizeEdge edge);
    void updateRectanglesAfterBorderAdded(int padding);

    // 成员变量
    QPixmap m_backgroundImage;      // 显示的QPixmap
    cv::Mat m_displayImage;         // 原始OpenCV图像（用于处理）
    QString m_fileName;            // 当前图片路径
    QString m_outputFilePrefix;
    QString m_outFileDirectory;
    int m_autoAddWhitePadding;     // 自动加白边参数（-1表示不自动加）

    // 矩形数据
    QVector<QRectF> m_rectangles;  // 图片坐标系中的矩形
    int m_selectedRectIndex;

    // 交互状态
    bool m_isDrawing;
    bool m_isPanning;
    bool m_isRecResizing;
    bool m_isMoveToRecEdge;
    QPoint m_startPoint;
    QPoint m_currentPoint;
    QPoint m_panStartPoint;
    QPoint m_lastMousePos;
    QPointF m_dragOffset;
    QPoint m_panOffset;
    double m_zoomFactor;
    ResizeEdge m_resizeEdge;
    int m_clickRectNumber;         // 缩放时记录选中的矩形索引
    QRectF m_resizeOriginalRect;

    // 常量配置
    static constexpr int EDGE_MARGIN = 5;
    static constexpr int MINIMUM_RECT_LENGTH = 10;
    static constexpr int REMOVE_WHITE_PADDING = 5;
    static constexpr int REMOVE_WHITE_THRESHOLD = 200;
    static constexpr int DEFAULT_BORDER_PADDING = 50;

    // 动作对象
    QAction* m_deleteAction;
    QAction* m_deleteAllAction;
    QAction* m_loadImageAction;
    QAction* m_resetZoomAction;
    QAction* m_splitPictureAction;
    QAction* m_removeWhiteAction;
    QAction* m_removeAllWhiteAction;
    QAction* m_addWhiteBorderOrigPictureAction;
    QAction *m_copyAction;
    QAction *m_fillAndDelAction;
    QAction *m_fillOutsideAction;
    QAction *m_processEditAction;
    QAction *m_action2xSuperResolution;
    QAction *m_action3xSuperResolution;
    QAction *m_action4xSuperResolution;

    QColor m_backgroundColor;
    bool m_isClickImageRegion;      // 右键菜单判断

    QTimer *m_fileWatcherTimer; // 文件监控
    QDateTime m_fileLastModified;
    void reloadImageFromFile(); // 重新加载当前图片

    QString m_programEdit;
    QString m_programScale;

    // 填白模式状态与动作
    bool m_fillWhiteEnabled;
    QAction* m_toggleFillWhiteAction;

    // 填充矩形为白色
    void fillRectWithWhite(const QRectF& rect);
    void fillOutsideRectWithWhite(const QRectF& rect);   // 将矩形外部填充白色
};

#endif // DRAWINGCANVAS_H
