#ifndef FILESENDER_H
#define FILESENDER_H

#include <QObject>
#include <QThread>
#include <QFile>
#include <QTimer>
#include <QUdpSocket>
#include <QTimer>

class FileSender : public QObject
{
    Q_OBJECT
public:
    explicit FileSender(QObject *parent = nullptr);
    ~FileSender();

    void start(const QString& path, int kbps,
               QString destIP, int destPort,
               QString localIP="", int localPort=0);
    void setSpeed(int kbps);
    void stop();

    qint64 currentProgress() {
        return m_sendBytes;
    }

signals:
    void started();
    void finished();

public slots:
    void onThreadStarted();
    void onThreadFinished();
    void onTimeout();

private:
    QThread m_thread;
    QTimer* m_timer = nullptr;
    QUdpSocket* m_udp = nullptr;

    QString m_path;
    int     m_speed = 1; // KB/s
    QString m_destIP;
    qint16  m_destPort;
    QString m_localIP;
    qint16  m_localPort;

    QHostAddress m_destAddr;

    QFile   m_file;
    qint64  m_sendBytes = 0;
    qint64  m_lastTime = 0;
};

#endif // FILESENDER_H
