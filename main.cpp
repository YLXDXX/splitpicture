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

    QCommandLineOption openAddWhiteBorder("a", "Add White Border", "add");
    parser.addOption(openAddWhiteBorder);

    // 解析命令行参数
    parser.process(app);

    MainWindow w;

    w.setWindowTitle("SplitPicture");

    DrawingCanvas *canvas = new DrawingCanvas();
    w.setCentralWidget(canvas);

    // 连接信号到槽（可以连接不同类里的信号）
    QObject::connect(canvas, &DrawingCanvas::titleChangeRequested,
            &w, &MainWindow::setWindowTitle);

    //检查命令行参数是否存在
    QPixmap newImage;
    if( parser.isSet(inputFilePath) )
    {
        canvas->fileName = parser.value(inputFilePath);
        canvas->outputFilePrefix = parser.value(outputFilePrefix);
        canvas->outFileDirectory = parser.value(inputFileDirectory);
        canvas->openAddWhiteBorderPaddingOrig = parser.value(openAddWhiteBorder).toInt();

        if( newImage.load(canvas->fileName) )
        {
            canvas->backgroundImage = newImage;
            canvas->displayImage = cv::imread(canvas->fileName.toStdString());
            canvas->fileName = parser.value(inputFilePath);

            if(canvas->openAddWhiteBorderPaddingOrig != -1)
            {
                //加载图片的同时添加白边框
                canvas->addWhiteBorderOrigPicture(canvas->openAddWhiteBorderPaddingOrig);
            }
            // 在窗口标题栏里加入处理图片的名称
            QFileInfo fileInfo(canvas->fileName);
            canvas->triggerTitleChange("SplitPicture & "+fileInfo.completeBaseName());
        }else
        {
            QMessageBox::warning(canvas, "加载错误", "无法加载图片: " + canvas->fileName);
            canvas->fileName="";
            canvas->outputFilePrefix="";
            canvas->outFileDirectory="";
            canvas->openAddWhiteBorderPaddingOrig = -1;
        }
    }

    w.show();
    return app.exec();
}
