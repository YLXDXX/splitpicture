// imageprocessor.h
// 封装所有 OpenCV 相关操作（格式转换、白边检测、添加边框、保存等）
#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QPixmap>
#include <QRectF>
#include <opencv2/core.hpp>

class ImageProcessor
{
public:
    // 格式转换
    static QPixmap cvMatToQPixmap(const cv::Mat& mat);
    static cv::Mat qPixmapToCvMat(const QPixmap& pixmap);

    // 图片加载与保存
    static cv::Mat loadImage(const QString& filePath);
    static bool saveImage(const cv::Mat& image, const QString& filePath);

    // 图像处理
    static cv::Mat addWhiteBorder(const cv::Mat& src, int padding);
    static QRectF getContentRect(const cv::Mat& img, int threshold = 200, int padding = 5);

    // 裁剪 ROI
    static cv::Mat extractROI(const cv::Mat& src, const QRectF& rect);

private:
    ImageProcessor() = delete;
};

#endif // IMAGEPROCESSOR_H
