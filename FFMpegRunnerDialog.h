#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class FFMpegRunnerDialog;
}

class FFMpegRunner;
class FFMpegRunnerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FFMpegRunnerDialog(QWidget *parent = 0);
    ~FFMpegRunnerDialog();
    void init(const QString &filein, int video_duration_secs, int width, int height);

signals:
    void start();
private slots:
    void onProgress(int seconds);
    void onStart();
    void onDone();
    void on_pushButtonCancel_clicked();

private:
    Ui::FFMpegRunnerDialog *ui;
    std::unique_ptr<FFMpegRunner> m_runner;
    QString m_file_in;
    QString m_file_out;
    int m_video_duration_secs;
    int m_width;
    int m_height;
};
