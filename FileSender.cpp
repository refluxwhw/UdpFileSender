#include "FileSender.h"
#include <QDateTime>
#include <QDebug>

FileSender::FileSender(QObject *parent) : QObject(parent)
{
    this->moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, this, &FileSender::onThreadStarted);
    connect(&m_thread, &QThread::finished, this, &FileSender::onThreadFinished);
}

FileSender::~FileSender()
{
    stop();
}

void FileSender::start(const QString &path, int kbps, QString destIP, int destPort,
                       QString localIP, int localPort)
{
    stop();

    m_path = path;
    m_speed = kbps;
    m_destIP = destIP;
    m_destPort = destPort;
    m_localIP = localIP;
    m_localPort = localPort;

    m_thread.start();
}

void FileSender::setSpeed(int kbps)
{
    m_speed = kbps;
}

void FileSender::stop()
{
    if (m_thread.isRunning()) {
        m_thread.quit();
        m_thread.wait();
    }
}

void FileSender::onThreadStarted()
{
    m_file.setFileName(m_path);
    if (!m_file.open(QIODevice::ReadOnly)) {
        qCritical() << QString("open file [%1] failed: %2")
                       .arg(m_path).arg(m_file.errorString());
        return;
    }

    m_udp = new QUdpSocket();
    QHostAddress localAddr;
    if (m_localIP.isEmpty()) {
        localAddr = QHostAddress::Any;
    } else {
        localAddr.setAddress(m_localIP);
    }
    if (!m_udp->bind(localAddr, m_localPort)) {
        qCritical() << QString("udp bind failed: %1").arg(m_udp->errorString());
        delete m_udp;
        m_udp = nullptr;
        m_file.close();
        return;
    }

    m_destAddr.setAddress(m_destIP);

    m_timer = new QTimer();
    connect(m_timer, &QTimer::timeout,
            this, &FileSender::onTimeout);
    m_timer->start(10);
    m_lastTime = QDateTime::currentMSecsSinceEpoch();
    m_sendBytes = 0;

    emit started();

    qInfo() << QString("%2:%3 ==> %4:%5 [%1]")
               .arg(m_path)
               .arg(m_udp->localAddress().toString()).arg(m_udp->localPort())
               .arg(m_destIP).arg(m_destPort);
}

void FileSender::onThreadFinished()
{
    if (nullptr != m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;

        delete m_udp;
        m_udp = nullptr;

        m_file.close();

        emit finished();
    }
}

void FileSender::onTimeout()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 ms = now - m_lastTime;
    m_lastTime = now;

    qint64 len = m_speed * 1024 * ms / 1000;
    QByteArray data = m_file.read(len);
    if (data.isEmpty()) {
        if (m_file.atEnd()) {
            qInfo() << QString("file send finished");
        } else {
            qCritical() << QString("read file failed: %1").arg(m_file.errorString());
        }
        m_thread.quit();
        return;
    }

    qint64 send = 0;
    len = data.size();
    while (len > 0) {
        qint64 t = qMin((qint64)1024, len);
        m_udp->writeDatagram(data.data() + send, t, m_destAddr, m_destPort);
        len -= t;
        send += t;

        m_sendBytes += t;
    }
}
