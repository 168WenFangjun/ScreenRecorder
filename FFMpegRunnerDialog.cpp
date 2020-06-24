#include "FFMpegRunnerDialog.h"

#include <QFile>

#include "FFMpegRunner.h"

#include "ui_FFMpegRunnerDialog.h"

FFMpegRunnerDialog::FFMpegRunnerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FFMpegRunnerDialog)
    , m_runner(std::make_unique<FFMpegRunner>())
{
    ui->setupUi(this);
    ui->progressBar->setRange(0, 100);
    connect(m_runner.get(), &FFMpegRunner::signalProgress,
            this, &FFMpegRunnerDialog::onProgress);
    connect(m_runner.get(), &FFMpegRunner::signalDone,
            this, &FFMpegRunnerDialog::onDone);
    connect(this, &FFMpegRunnerDialog::start,
            this, &FFMpegRunnerDialog::onStart);
}

FFMpegRunnerDialog::~FFMpegRunnerDialog()
{
    delete ui;
}

void FFMpegRunnerDialog::init(const QString &filein, int video_duration_secs, int width, int height)
{
    m_file_in = filein;
    m_video_duration_secs = video_duration_secs;
    m_width = width;
    m_height = height;
}

void FFMpegRunnerDialog::onProgress(int seconds)
{
    ui->progressBar->setValue(seconds * 100 / m_video_duration_secs);
    if(seconds == m_video_duration_secs)
    {
        close();
    }
}

void FFMpegRunnerDialog::onStart()
{
    ui->progressBar->setValue(0);
    m_file_out = m_runner->convertToH264(m_file_in, m_width, m_height);
}

void FFMpegRunnerDialog::onDone()
{
    QFile fin(m_file_in);
    QFile fout(m_file_out);
    if(fout.exists() && fin.exists())
    {
        fin.remove();
        fout.rename(m_file_in);
    }
    close();
}

void FFMpegRunnerDialog::on_pushButtonCancel_clicked()
{
    close();
}
