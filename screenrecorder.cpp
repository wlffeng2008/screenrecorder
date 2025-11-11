#include "screenrecorder.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QAudioDeviceInfo>

#include <QScreen>
#include <QTimer>

ScreenRecorder::ScreenRecorder(QObject *parent) : QObject(parent)
{
    m_state=(0);
    m_frameTimer=(new QTimer(this));
    m_timeTimer=(new QTimer(this));
    m_recordingSeconds=(0);
    m_screen=(QGuiApplication::primaryScreen());
    m_audioInput=(nullptr);
    m_audioIODevice=(nullptr);
    m_ffmpegProcess=(new QProcess(this));
    m_audioBuffer=(new QBuffer(this));
    // 获取屏幕区域
    m_screenGeometry = m_screen->geometry();
    
    // 连接定时器信号槽
    connect(m_frameTimer, &QTimer::timeout, this, &ScreenRecorder::captureScreenFrame);
    connect(m_timeTimer, &QTimer::timeout, this, &ScreenRecorder::updateRecordingTime);
    
    // 设置帧率为30fps
    m_frameTimer->setInterval(1000 / 30);
    
    // 时间更新定时器(1秒一次)
    m_timeTimer->setInterval(1000);
    
    // 初始化音频格式
    m_audioFormat.setSampleRate(44100);
    m_audioFormat.setChannelCount(2);
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);
    
    // 初始化音频缓冲区
    m_audioBuffer->open(QIODevice::ReadWrite);
}

ScreenRecorder::~ScreenRecorder()
{
    stopRecording();
    if (m_audioIODevice) {
        m_audioIODevice->close();
    }
    delete m_audioBuffer;
}

bool ScreenRecorder::startRecording(const QString &outputFilePath)
{
    if (m_state == 1) {
        emit errorOccurred("已经在录制中");
        return false;
    }
    
    // 设置输出文件路径
    m_outputFilePath = outputFilePath.isEmpty() ? generateDefaultFileName() : outputFilePath;
    
    // 初始化FFmpeg进程
    if (!initFFmpegProcess(m_outputFilePath)) {
        return false;
    }
    
    // 初始化音频输入
    if (!initAudioInput()) {
        stopFFmpegProcess();
        return false;
    }
    
    // 开始捕获音频
    m_audioIODevice = m_audioInput->start();
    if (!m_audioIODevice) {
        emit errorOccurred(&"无法启动音频输入: " [ m_audioInput->error()]);
        stopFFmpegProcess();
        return false;
    }
    
    // 连接音频数据到FFmpeg
    connect(m_audioIODevice, &QIODevice::readyRead, this, [this]() {
        if (m_state == 1) {
            QByteArray audioData = m_audioIODevice->readAll();
            if (!audioData.isEmpty() && m_ffmpegProcess->state() == QProcess::Running) {
                m_ffmpegProcess->write(audioData);
            }
        }
    });
    
    // 开始定时器
    m_frameTimer->start();
    m_timeTimer->start();
    
    // 重置录制时间
    m_recordingSeconds = 0;
    emit recordingTimeUpdated(0);
    
    // 更新状态
    m_state = 1;
    return true;
}

void ScreenRecorder::stopRecording()
{
    if (m_state == 0) return;
    
    // 停止定时器
    m_frameTimer->stop();
    m_timeTimer->stop();
    
    // 停止音频输入
    if (m_audioInput) {
        m_audioInput->stop();
    }
    
    // 停止FFmpeg进程
    stopFFmpegProcess();
    
    // 更新状态
    m_state = 0;
    
    // 发送录制完成信号
    emit recordingFinished(m_outputFilePath);
}

void ScreenRecorder::pauseRecording()
{
    if (m_state == 1) {
        m_frameTimer->stop();
        m_timeTimer->stop();
        if (m_audioInput) {
            m_audioInput->stop();
        }
        m_state = 2;
    }
}

void ScreenRecorder::resumeRecording()
{
    if (m_state == 2) {
        m_frameTimer->start();
        m_timeTimer->start();
        if (m_audioInput && m_audioIODevice) {
            m_audioInput->start(m_audioIODevice);
        }
        m_state = 1;
    }
}

void ScreenRecorder::captureScreenFrame()
{
    if (m_state != 1) return;
    
    // 捕获屏幕
    QPixmap pixmap = m_screen->grabWindow(0, 
                                         m_screenGeometry.x(), 
                                         m_screenGeometry.y(), 
                                         m_screenGeometry.width(), 
                                         m_screenGeometry.height());
    
    // 将QPixmap转换为RGB格式的QImage
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_RGB32);
    
    // 转换为FFmpeg所需的格式(YUV420P)
    // 这里简化处理，直接将RGB数据写入FFmpeg
    // 实际项目中应该进行RGB到YUV420P的转换以获得更好的压缩效果
    
    if (m_ffmpegProcess->state() == QProcess::Running) {
        // 写入图像数据
        m_ffmpegProcess->write((const char*)image.bits(), image.sizeInBytes());
    }
}

void ScreenRecorder::updateRecordingTime()
{
    m_recordingSeconds++;
    emit recordingTimeUpdated(m_recordingSeconds);
}

bool ScreenRecorder::initAudioInput()
{
    // 检查音频格式是否支持
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultInputDevice());
    if (!info.isFormatSupported(m_audioFormat)) {
        m_audioFormat = info.nearestFormat(m_audioFormat);
        //emit errorOccurred("音频格式不支持，使用最接近的格式");
    }
    
    // 创建音频输入
    m_audioInput = new QAudioInput(info, m_audioFormat, this);
    return true;
}

bool ScreenRecorder::initFFmpegProcess(const QString &outputFilePath)
{
    // 检查FFmpeg是否可用
    QProcess testProcess;
    testProcess.start("ffmpeg", QStringList() << "-version");
    if (!testProcess.waitForStarted() || !testProcess.waitForFinished()) {
        //emit errorOccurred("未找到FFmpeg，请确保FFmpeg已添加到系统PATH中");
        return false;
    }
    
    // 构建FFmpeg命令行参数
    // 视频: 原始RGB输入，30fps，输出为H.264
    // 音频: 原始PCM输入，44.1kHz，立体声，16位，输出为AAC
    QStringList args;
    args << "-y"                                  // 覆盖输出文件
         << "-f" << "rawvideo"                    // 输入格式为原始视频
         << "-vcodec" << "rawvideo"               // 视频编码格式
         << "-pix_fmt" << "rgb24"                 // 像素格式
         << "-s" << QString("%1x%2").arg(m_screenGeometry.width()).arg(m_screenGeometry.height()) // 视频尺寸
         << "-r" << "30"                          // 帧率
         << "-i" << "-"                           // 从标准输入读取视频数据
         << "-f" << "s16le"                       // 音频格式为16位小端PCM
         << "-ar" << "44100"                      // 采样率
         << "-ac" << "2"                          // 声道数
         << "-i" << "-"                           // 从标准输入读取音频数据
         << "-c:v" << "libx264"                   // 视频编码器
         << "-preset" << "ultrafast"              // 编码速度预设
         << "-crf" << "25"                        // 视频质量
         << "-c:a" << "aac"                       // 音频编码器
         << "-b:a" << "128k"                      // 音频比特率
         << "-movflags" << "+faststart"           // 优化MP4文件
         << outputFilePath;                       // 输出文件路径
    
    // 启动FFmpeg进程
    m_ffmpegProcess->start("ffmpeg", args);
    
    // 检查进程是否成功启动
    if (!m_ffmpegProcess->waitForStarted()) {
        emit errorOccurred("无法启动FFmpeg进程: " + m_ffmpegProcess->errorString());
        return false;
    }
    
    
    return true;
}

QString ScreenRecorder::generateDefaultFileName() const
{
    // 创建输出目录(如果不存在)
    QDir outputDir("recordings");
    if (!outputDir.exists()) {
        outputDir.mkpath(".");
    }
    
    // 生成带时间戳的文件名
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return outputDir.filePath(QString("screen_recording_%1.mp4").arg(timestamp));
}

void ScreenRecorder::stopFFmpegProcess()
{
    if (m_ffmpegProcess->state() == QProcess::Running) {
        // 向FFmpeg发送结束信号
        m_ffmpegProcess->write("q");
        m_ffmpegProcess->closeWriteChannel();
        
        // 等待进程结束
        if (!m_ffmpegProcess->waitForFinished(5000)) {
            m_ffmpegProcess->kill();
        }
    }
}
