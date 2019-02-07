#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <QTcpSocket>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_butConnect_clicked();
    void connected();
    void disconnected();
    void readyRead();

private:
    enum ConState {
        Disconnected,
        Connecting,
        Connected
    };
    enum ConState state;
    QTcpSocket socket;
    Ui::MainWindow *ui;
    QStringListModel *model;
};

#endif // MAINWINDOW_H
