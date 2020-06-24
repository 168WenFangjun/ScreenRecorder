#pragma once

#include <memory>
#include <QProcess>

class FFMpegRunner : public QObject
{
    Q_OBJECT
public:
    FFMpegRunner(QObject *parent = nullptr);
    ~FFMpegRunner();
    QString convertToH264(const QString &mpeg4, int width, int height);

signals:
    void signalProgress(int seconds);
    void signalDone();
protected:
    void setLdLibraryPath();
    QString getExePath() const;
    QString getExeFullPath(const QString &exePath) const;
    void onFFmpegOutput();
    void onFFmpegError();
    void logFFmpegOutput(const QString &msg);
    void onFFmpegDone(int exitCode, QProcess::ExitStatus exitStatus);

    std::unique_ptr<QProcess> m_p;
    QProcessEnvironment m_env;
};
