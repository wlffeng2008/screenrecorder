#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_recorder(new ScreenRecorder(this))
{
    ui->setupUi(this);
    
    // 连接信号槽
    connect(m_recorder, &ScreenRecorder::recordingTimeUpdated, 
            this, &MainWindow::updateRecordingTime);
    connect(m_recorder, &::ScreenRecorder::recordingFinished,
            this, &MainWindow::onRecordingFinished);
    connect(m_recorder, &ScreenRecorder::errorOccurred,
            this, &MainWindow::showError);
    
    // 初始化UI状态
    ui->stopButton->setEnabled(false);
    ui->pauseButton->setEnabled(false);
    ui->timeLabel->setText("00:00:00");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startButton_clicked()
{
    if (m_recorder->getState() == Stopped) {
        // 开始新的录制
        bool success = m_recorder->startRecording();
        if (success) {
            ui->startButton->setEnabled(false);
            ui->stopButton->setEnabled(true);
            ui->pauseButton->setEnabled(true);
            ui->statusLabel->setText("正在录制...");
        }
    } else if (m_recorder->getState() == Paused) {
        // 恢复录制
        m_recorder->resumeRecording();
        ui->startButton->setEnabled(false);
        ui->stopButton->setEnabled(true);
        ui->pauseButton->setText("暂停");
        ui->statusLabel->setText("正在录制...");
    }
}

void MainWindow::on_stopButton_clicked()
{
    m_recorder->stopRecording();
    ui->startButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->pauseButton->setEnabled(false);
    ui->pauseButton->setText("暂停");
    ui->statusLabel->setText("已停止");
}

void MainWindow::on_pauseButton_clicked()
{
    if (m_recorder->getState() == 1) {
        m_recorder->pauseRecording();
        ui->startButton->setEnabled(true);
        ui->pauseButton->setText("继续");
        ui->statusLabel->setText("已暂停");
    } else if (m_recorder->getState() == 2) {
        m_recorder->resumeRecording();
        ui->startButton->setEnabled(false);
        ui->pauseButton->setText("暂停");
        ui->statusLabel->setText("正在录制...");
    }
}

void MainWindow::updateRecordingTime(int seconds)
{
    ui->timeLabel->setText(formatTime(seconds));
}

QString MainWindow::formatTime(int seconds) const
{
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    return QString("%1:%2:%3")
            .arg(h, 2, 10, QLatin1Char('0'))
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0'));
}

void MainWindow::onRecordingFinished(const QString &filePath)
{
    QMessageBox::information(this, "录制完成", 
                           QString("视频已保存至:\n%1").arg(filePath));
}

void MainWindow::showError(const QString &errorMsg)
{
    QMessageBox::critical(this, "错误", errorMsg);
}
