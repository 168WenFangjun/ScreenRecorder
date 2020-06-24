#include "FFMpegRunner.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>

FFMpegRunner::FFMpegRunner(QObject *parent)
    : QObject(parent)
    , m_p(std::make_unique<QProcess>())
{
    connect(m_p.get(), &QProcess::readyReadStandardError, this, &FFMpegRunner::onFFmpegError);
    connect(m_p.get(), &QProcess::readyReadStandardOutput, this, &FFMpegRunner::onFFmpegOutput);
    connect(m_p.get(),
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this,
            &FFMpegRunner::onFFmpegDone);

    setLdLibraryPath();
}

FFMpegRunner::~FFMpegRunner()
{
}

void FFMpegRunner::setLdLibraryPath()
{
    m_env = QProcessEnvironment::systemEnvironment();
    m_env.insert("LD_LIBRARY_PATH", getExePath());
    m_p->setProcessEnvironment(m_env);
}

QString FFMpegRunner::getExePath() const
{
//#define MYDEBUG
#ifdef MYDEBUG
#if defined(Win64)
    QString exePath = "c:\\work\\libs\\build\\win64\\gpl";
#elif defined(Darwin)
    QString exePath = "/work/libs/build/linux/bin/gpl";
#elif defined(Linux)
    QString exePath = "/home/dev/oosman/work.git/libs/build/linux/bin/gpl";
#endif
#else//!MYDEBUG
    QString path = QCoreApplication::applicationFilePath();
    QFileInfo info(path);
    QString exePath = info.absolutePath().append("/gpl");
#endif
    exePath = QDir::toNativeSeparators(exePath);
    return exePath;
}

QString FFMpegRunner::getExeFullPath(const QString &exePath) const
{
    QString exeFullPath = QString("%1%2ffmpeg").arg(exePath).arg(QDir::separator());
#if defined(Win32) || defined(Win64)
    exeFullPath.append(".exe");
#endif
    exeFullPath = QDir::toNativeSeparators(exeFullPath);

    return exeFullPath;
}

QString FFMpegRunner::convertToH264(const QString& mpeg4, int width, int height)
{
    qDebug() << "convertToH264" << mpeg4;

    //ffmpeg -i in.mp4  -c:v h264  -c:a copy -f mp4 out.mp4
    auto exePath = getExePath();
    auto exeFullPath = getExeFullPath(exePath);

    QFileInfo info(mpeg4);
    auto path = info.absolutePath();
    auto baseName = info.completeBaseName();
    QString out_file(path.append(QDir::separator()).
                     append(baseName).
                     append(".h264.mp4"));
    QFile f(out_file);
    if(f.exists()) { f.remove(); }
    qDebug() << "out_file" << out_file;

    QStringList args;
    //h264: width and height must be divisible by 2
    QString scale = (width % 2 == 0) ?
                QString("scale=%1:-2").arg(width) :
                (height % 2 == 0) ?
                    QString("scale=-2:%1").arg(height) :
                    QString("scale=%1:-2").arg(width - 1);
    args << "-i" << mpeg4
         << "-c:v" << "h264"
         << "-c:a" << "copy"
         << "-vf" << scale
         << "-f" << "mp4"
         << out_file;

    m_p->start(exeFullPath, args);
    if(!m_p->waitForStarted(-1))
    {
        qDebug() << "failed to start"
                 << exeFullPath
                 << args.join(" ")
                 << m_env.value("LD_LIBRARY_PATH");
        return mpeg4;
    }
    else
    {
        qDebug() << "started"
                 << exeFullPath
                 << args.join(" ")
                 << m_env.value("LD_LIBRARY_PATH");
    }

    qDebug() << "exiting";
    return out_file;
}

void FFMpegRunner::onFFmpegOutput()
{
    QString out = m_p->readAllStandardOutput();
    if(out.count()) logFFmpegOutput(out);
}

void FFMpegRunner::onFFmpegError()
{
    QString err = m_p->readAllStandardError();
    if(err.count()) logFFmpegOutput(err);
}

void FFMpegRunner::logFFmpegOutput(const QString &msg)
{
    qDebug() << msg;

    QRegExp rx("time=(\\d*):(\\d*):(\\d*\\.\\d*)");
    int pos = 0;
    pos = rx.indexIn(msg, pos);
    if(pos == -1)
    {
        return;
    }
    auto tokens = rx.capturedTexts();
    if(tokens.count() != 4)
    {
        return;
    }
    int hours = tokens.at(1).toInt();
    int minutes = tokens.at(2).toInt();
    int seconds = (int)(tokens.at(3).toFloat())
            + minutes * 60
            + hours * 60 * 60;

    emit signalProgress(seconds);
}

void FFMpegRunner::onFFmpegDone(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "done" << exitCode << exitStatus;
    emit signalDone();
}
