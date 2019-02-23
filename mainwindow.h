#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <QTcpSocket>

#include "ifoxtrotctl.h"
#include "ifoxtrotmodel.h"

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
    void sockError(QAbstractSocket::SocketError socketError);

    void on_listViewItems_clicked(const QModelIndex &index);

    void on_pushButtonRelay_clicked();

    void on_pushButtonLight_clicked();

    void on_pushButtonSc1_clicked();

    void on_pushButtonSc2_clicked();

    void on_pushButtonSc3_clicked();

    void on_pushButtonSc4_clicked();

    void on_pushButtonSc5_clicked();

    void on_pushButtonSc6_clicked();

    void on_pushButtonSc7_clicked();

    void on_pushButtonSc8_clicked();

private:
    enum ConState {
        Disconnected,
        Connecting,
        Connected
    };
    enum ConState state;
    QTcpSocket socket;
    Ui::MainWindow *ui;
    iFoxtrotModel *model;
};

#endif // MAINWINDOW_H
