//创建应用、解析命令行、设置画布、显示窗口

#include "mainwindow.h"
#include "drawingcanvas.h"
#include "commandlineoptions.h"
#include "imageprocessor.h"

#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("splitpicture");
    app.setApplicationVersion("0.1");

    // 解析命令行
    CommandLineOptions options;
    options.parse(argc, argv);

    MainWindow w;
    DrawingCanvas* canvas = w.canvas();

    // 连接标题更新信号
    QObject::connect(canvas, &DrawingCanvas::titleChangeRequested,
                     &w, &QMainWindow::setWindowTitle);

    // 如果命令行提供了输入文件，直接加载
    if (options.hasInputFile()) {
        QString filePath = options.inputFilePath();
        QPixmap pixmap;
        if (pixmap.load(filePath)) {
            cv::Mat cvImage = ImageProcessor::loadImage(filePath);
            if (!cvImage.empty()) {
                canvas->setOutputFilePrefix(options.outputFilePrefix());
                canvas->setOutputDirectory(options.outputDirectory());
                canvas->setAutoAddWhiteBorderPadding(options.addWhiteBorderPadding());

                canvas->setProgramEdit(options.programEdit());
                canvas->setProgramScale(options.programScale());
                canvas->setFileName(filePath);

                canvas->setBackgroundImage(pixmap, cvImage);
                canvas->startFileWatching();

                QFileInfo info(filePath);
                canvas->triggerTitleChange("SplitPicture & " + info.completeBaseName());
            } else {
                QMessageBox::warning(nullptr, "加载错误", "OpenCV无法加载图片: " + filePath);
            }
        } else {
            QMessageBox::warning(nullptr, "加载错误", "无法加载图片: " + filePath);
        }
    }

    w.show();
    return app.exec();
}
