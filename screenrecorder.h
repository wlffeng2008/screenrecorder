#ifndef SCREENRECORDER_H
#define SCREENRECORDER_H

#include <QObject>
#include <QTimer>
#include <QScreen>
#include <QAudioInput>
#include <QAudioFormat>
#include <QFile>
#include <QDateTime>
#include <QProcess>
#include <QBuffer>


class ScreenRecorder : public QObject
{
    Q_OBJECT
public:
    // 录屏状态枚举
    // enum RecorderState
    // {
    //     Stopped,
    //     Recording,
    //     Paused
    // };
    // Q_ENUM(RecorderState)

    explicit ScreenRecorder(QObject *parent = nullptr);
    ~ScreenRecorder();

    // 开始录制
    bool startRecording(const QString &outputFilePath = "");
    
    // 停止录制
    void stopRecording();
    
    // 暂停录制
    void pauseRecording();
    
    // 恢复录制
    void resumeRecording();
    
    // 获取当前状态
    int getState() const { return m_state; }

signals:
    // 录制时间更新
    void recordingTimeUpdated(int seconds);
    
    // 录制完成
    void recordingFinished(const QString &filePath);
    
    // 错误信息
    void errorOccurred(const QString &errorMsg);

private slots:
    // 捕获屏幕帧
    void captureScreenFrame();
    
    // 更新录制时间
    void updateRecordingTime();

private:
    // 初始化音频输入
    bool initAudioInput();
    
    // 初始化FFmpeg进程
    bool initFFmpegProcess(const QString &outputFilePath);
    
    // 生成默认文件名
    QString generateDefaultFileName() const;
    
    // 停止FFmpeg进程
    void stopFFmpegProcess();

    int m_state;          // 当前状态
    QTimer *m_frameTimer;           // 帧捕获定时器
    QTimer *m_timeTimer;            // 时间更新定时器
    int m_recordingSeconds;         // 录制秒数
    
    QScreen *m_screen;              // 屏幕对象
    QRect m_screenGeometry;         // 屏幕区域
    
    // 音频相关
    QAudioInput *m_audioInput;
    QIODevice *m_audioIODevice;
    QAudioFormat m_audioFormat;
    
    // FFmpeg进程
    QProcess *m_ffmpegProcess;
    QString m_outputFilePath;       // 输出文件路径
    
    // 音频数据缓冲区
    QBuffer *m_audioBuffer;
};

#endif // SCREENRECORDER_H
