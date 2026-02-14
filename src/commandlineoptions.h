// commandlineoptions.h
// 解析命令行参数，存储解析结果，供其他模块使用
#ifndef COMMANDLINEOPTIONS_H
#define COMMANDLINEOPTIONS_H

#include <QString>

class CommandLineOptions
{
public:
    CommandLineOptions();

    bool parse(int argc, char *argv[]);

    QString inputFilePath() const;
    QString outputFilePrefix() const;
    QString outputDirectory() const;
    int addWhiteBorderPadding() const;
    bool hasInputFile() const;

    QString programEdit() const;
    QString programScale() const;

private:
    QString m_inputFilePath;
    QString m_outputFilePrefix;
    QString m_outputDirectory;
    int m_addWhiteBorderPadding;
    bool m_hasInputFile;
    QString m_programEdit;
    QString m_programScale;
};

#endif // COMMANDLINEOPTIONS_H
