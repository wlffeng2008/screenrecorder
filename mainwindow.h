#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "screenrecorder.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_pauseButton_clicked();
    void updateRecordingTime(int seconds);
    void onRecordingFinished(const QString &filePath);
    void showError(const QString &errorMsg);

private:
    Ui::MainWindow *ui;
    ScreenRecorder *m_recorder;
    QString formatTime(int seconds) const;
};
#endif // MAINWINDOW_H
