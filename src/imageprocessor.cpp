// imageprocessor.cpp
// 封装所有 OpenCV 相关操作（格式转换、白边检测、添加边框、保存等）
#include "imageprocessor.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <QImage>
#include <QDebug>

QPixmap ImageProcessor::cvMatToQPixmap(const cv::Mat& mat)
{
    if (mat.empty())
        return QPixmap();

    cv::Mat rgbMat;
    switch (mat.channels()) {
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
            return QPixmap();
    }

    QImage image(rgbMat.data, rgbMat.cols, rgbMat.rows,
                 static_cast<int>(rgbMat.step),
                 (rgbMat.channels() == 4) ? QImage::Format_RGBA8888
                 : QImage::Format_RGB888);
    return QPixmap::fromImage(image.copy());
}

cv::Mat ImageProcessor::qPixmapToCvMat(const QPixmap& pixmap)
{
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(image.height(), image.width(), CV_8UC3,
                const_cast<uchar*>(image.bits()),
                static_cast<size_t>(image.bytesPerLine()));
    cv::Mat result = mat.clone();
    cv::cvtColor(result, result, cv::COLOR_RGB2BGR);
    return result;
}

cv::Mat ImageProcessor::loadImage(const QString& filePath)
{
    return cv::imread(filePath.toStdString());
}

bool ImageProcessor::saveImage(const cv::Mat& image, const QString& filePath)
{
    return cv::imwrite(filePath.toStdString(), image);
}

cv::Mat ImageProcessor::addWhiteBorder(const cv::Mat& src, int padding)
{
    cv::Mat dst;
    cv::copyMakeBorder(src, dst, padding, padding, padding, padding,
                       cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
    return dst;
}

QRectF ImageProcessor::getContentRect(const cv::Mat& img, int threshold, int padding)
{
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    cv::threshold(gray, gray, threshold, 255, cv::THRESH_BINARY_INV);

    std::vector<cv::Point> coords;
    cv::findNonZero(gray, coords);

    if (coords.empty()) {
        // 全白图片，返回一个仅包含内边距的小矩形（调用者可以据此删除）
        return QRectF(padding, padding, 2 * padding, 2 * padding);
    }

    cv::Rect boundingRect = cv::boundingRect(coords);
    int x = std::max(0, boundingRect.x - padding);
    int y = std::max(0, boundingRect.y - padding);
    int w = std::min(img.cols - x, boundingRect.width + 2 * padding);
    int h = std::min(img.rows - y, boundingRect.height + 2 * padding);

    return QRectF(x, y, w, h);
}

cv::Mat ImageProcessor::extractROI(const cv::Mat& src, const QRectF& rect)
{
    int x = static_cast<int>(std::round(rect.x()));
    int y = static_cast<int>(std::round(rect.y()));
    int w = static_cast<int>(std::round(rect.width()));
    int h = static_cast<int>(std::round(rect.height()));

    // 边界检查
    x = std::max(0, x);
    y = std::max(0, y);
    w = std::min(src.cols - x, w);
    h = std::min(src.rows - y, h);

    if (w <= 0 || h <= 0)
        return cv::Mat();
    return src(cv::Rect(x, y, w, h)).clone();
}
