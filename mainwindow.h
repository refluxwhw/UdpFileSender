#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "FileSender.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_choose_file_clicked();
    void on_pushButton_start_clicked();
    void on_pushButton_stop_clicked();
    void on_spinBox_speed_control_editingFinished();

    void onTaskStarted();
    void onTaskFinished();

    void onTimeout();

protected:
    void customEvent(QEvent* ev);

private:
    void setEditable(bool e);


private:
    Ui::MainWindow *ui;
    int m_timerID = 0;

    qint64  m_totalSize;
    qint64  m_lastSend;
    qint64  m_lastTime;

    FileSender m_sender;

    QTimer m_timer;
};

#endif // MAINWINDOW_H
