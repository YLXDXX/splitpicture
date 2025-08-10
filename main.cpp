#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用程序基本信息
    app.setApplicationName("splitpicture");
    app.setApplicationVersion("0.1");


    // 创建命令行解析器
    QCommandLineParser parser;
    parser.setApplicationDescription("Cut an image into parts and remove the white edges");
    parser.addHelpOption();     // 添加 -h/--help 选项
    parser.addVersionOption();  // 添加 -v/--version 选项

    // 添加自定义选项
    QCommandLineOption inputFilePath("i", "Input file path", "input");
    parser.addOption(inputFilePath);

    QCommandLineOption outputFilePrefix("p", "Output file name prefix", "prefix");
    parser.addOption(outputFilePrefix);

    QCommandLineOption inputFileDirectory("d", "Output file save directory", "directory");
    parser.addOption(inputFileDirectory);

    // 解析命令行参数
    parser.process(app);

    MainWindow w;

    w.setWindowTitle("图像切割");

    DrawingCanvas *canvas = new DrawingCanvas();
    w.setCentralWidget(canvas);

    //检查命令行参数是否存在
    QPixmap newImage;
    if( parser.isSet(inputFilePath) )
    {
        canvas->fileName = parser.value(inputFilePath);
        canvas->outputFilePrefix = parser.value(outputFilePrefix);
        canvas->outFileDirectory = parser.value(inputFileDirectory);

        if( newImage.load(canvas->fileName) )
        {
            canvas->backgroundImage = newImage;
            canvas->displayImage = cv::imread(canvas->fileName.toStdString());
            canvas->fileName = parser.value(inputFilePath);
        }else
        {
            QMessageBox::warning(canvas, "加载错误", "无法加载图片: " + canvas->fileName);
            canvas->fileName="";
            canvas->outputFilePrefix="";
            canvas->outFileDirectory="";
        }
    }

    w.show();
    return app.exec();
}
