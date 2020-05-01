#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDateTime>
#include <QMessageBox>
#include <QEvent>
#include <QHostAddress>

#define u8(x) QStringLiteral(x)
#define EV_TYPE_LOG ((QEvent::Type)(QEvent::User+1000))

MainWindow* g_win = nullptr;

class LogEvent : public QEvent {
public:
    LogEvent(const QString& msg) :QEvent(EV_TYPE_LOG), msg(msg) {}
    QString msg;
};

void logHandler(QtMsgType, const QMessageLogContext &, const QString &msg) {
    QCoreApplication::postEvent(g_win, new LogEvent(msg));
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->pushButton_stop->hide();
    ui->progressBar->setRange(0, 10000);

    ui->textEdit->document()->setMaximumBlockCount(100);

    connect(&m_sender, &FileSender::started, this, &MainWindow::onTaskStarted);
    connect(&m_sender, &FileSender::finished, this, &MainWindow::onTaskFinished);
    connect(&m_timer, &QTimer::timeout, this, &MainWindow::onTimeout);

    g_win = this;
    qInstallMessageHandler(logHandler);
}

MainWindow::~MainWindow()
{
    qInstallMessageHandler(0);
    delete ui;
}

void MainWindow::on_pushButton_choose_file_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, u8("选择文件"));
    if (path.isEmpty()) {
        return;
    }
    ui->lineEdit_file_path->setText(path);
}

void MainWindow::on_pushButton_start_clicked()
{
    QString path = ui->lineEdit_file_path->text();
    QFileInfo info(path);
    if (!info.exists()) {
        QMessageBox::critical(this, u8("错误"), u8("文件不存在，无法发送"));
        return;
    }

    QHostAddress addr;
    QString localIP = ui->lineEdit_local_ip->text();
    if (!localIP.isEmpty()) {
        if (!addr.setAddress(localIP)) {
            QMessageBox::critical(this, u8("错误"), u8("本机IP地址不合法"));
            return;
        }
    }
    QString destIP = ui->lineEdit_dest_ip->text();
    if (destIP.isEmpty()) {
        QMessageBox::critical(this, u8("错误"), u8("目的IP地址不能为空"));
        return;
    }
    if (!addr.setAddress(destIP)) {
        QMessageBox::critical(this, u8("错误"), u8("目的IP地址不合法"));
        return;
    }

    m_totalSize = info.size();
    m_lastSend = 0;
    m_lastTime = QDateTime::currentMSecsSinceEpoch();

    m_sender.start(info.absoluteFilePath(), ui->spinBox_speed_control->value(),
                   destIP, ui->spinBox_dest_port->value(),
                   localIP, ui->spinBox_local_port->value());
}

void MainWindow::on_pushButton_stop_clicked()
{
    m_sender.stop();
}

void MainWindow::on_spinBox_speed_control_editingFinished()
{
    m_sender.setSpeed(ui->spinBox_speed_control->value());
}

void MainWindow::onTaskStarted()
{
    ui->pushButton_start->setVisible(false);
    ui->pushButton_stop->setVisible(true);
    setEditable(false);
    m_timer.start(200);

    ui->progressBar->setValue(0);
    ui->label_total->setText(QString::number(m_totalSize));
}

void MainWindow::onTaskFinished()
{
    onTimeout();

    m_timer.stop();
    setEditable(true);
    ui->pushButton_start->setVisible(true);
    ui->pushButton_stop->setVisible(false);
}

void MainWindow::onTimeout()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 ms = now - m_lastTime;
    qint64 send = m_sender.currentProgress();
    qreal  speed = (send - m_lastSend) * 1000 / 1024.0 / ms; // KB/s
    int    progress = send * 10000 / m_totalSize;

    ui->doubleSpinBox_real_speed->setValue(speed);
    ui->progressBar->setValue(progress);
    ui->label_send->setText(QString::number(send));

    m_lastSend = send;
    m_lastTime = now;
}

void MainWindow::customEvent(QEvent *ev)
{
    if (ev->type() == EV_TYPE_LOG) {
        ev->accept();
        LogEvent* e = (LogEvent*)ev;
        ui->textEdit->append(e->msg);
    }
}

void MainWindow::setEditable(bool e)
{
    ui->lineEdit_local_ip->setEnabled(e);
    ui->spinBox_local_port->setEnabled(e);
    ui->lineEdit_dest_ip->setEnabled(e);
    ui->spinBox_dest_port->setEnabled(e);
    ui->lineEdit_file_path->setEnabled(e);
    ui->pushButton_choose_file->setEnabled(e);
}
