// commandlineoptions.cpp
// 解析命令行参数，存储解析结果，供其他模块使用
#include "commandlineoptions.h"
#include <QCommandLineParser>
#include <QCoreApplication>

CommandLineOptions::CommandLineOptions()
: m_addWhiteBorderPadding(-1)
, m_hasInputFile(false)
, m_programEdit("comicenhancerpro")      // 默认值
, m_programScale("upscayl")      // 默认值
{
}

bool CommandLineOptions::parse(int argc, char *argv[])
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Cut an image into parts and remove the white edges");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inputFilePath("i", "Input file path", "input");
    parser.addOption(inputFilePath);

    QCommandLineOption outputFilePrefix("p", "Output file name prefix", "prefix");
    parser.addOption(outputFilePrefix);

    QCommandLineOption outputDirectory("d", "Output file save directory", "directory");
    parser.addOption(outputDirectory);

    QCommandLineOption addWhiteBorder("a", "Add White Border", "add");
    parser.addOption(addWhiteBorder);

    QCommandLineOption programEdit("E", "Path to external edit program (default: comicenhancerpro)", "edit");
    parser.addOption(programEdit);

    QCommandLineOption programScale("S", "Path to external scale program (default: upscayl)", "scale");
    parser.addOption(programScale);

    parser.process(*QCoreApplication::instance());

    m_inputFilePath = parser.value(inputFilePath);
    m_outputFilePrefix = parser.value(outputFilePrefix);
    m_outputDirectory = parser.value(outputDirectory);


    // 读取选项值：仅当存在时才覆盖，否则保留构造函数中的默认值
    if (parser.isSet(addWhiteBorder))
        m_addWhiteBorderPadding = parser.value(addWhiteBorder).toInt();

    if (parser.isSet(inputFilePath))
        m_hasInputFile = parser.isSet(inputFilePath);

    if (parser.isSet(programEdit))
        m_programEdit = parser.value(programEdit);

    if (parser.isSet(programScale))
        m_programScale = parser.value(programScale);

    return true;
}

QString CommandLineOptions::inputFilePath() const { return m_inputFilePath; }
QString CommandLineOptions::outputFilePrefix() const { return m_outputFilePrefix; }
QString CommandLineOptions::outputDirectory() const { return m_outputDirectory; }
int CommandLineOptions::addWhiteBorderPadding() const { return m_addWhiteBorderPadding; }
bool CommandLineOptions::hasInputFile() const { return m_hasInputFile; }
QString CommandLineOptions::programEdit() const{return m_programEdit;}
QString CommandLineOptions::programScale() const{return m_programScale;}
