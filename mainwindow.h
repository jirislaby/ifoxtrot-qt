#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <QTcpSocket>

#include "ifoxtrotsession.h"
#include "ifoxtrotctl.h"

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
    void on_butDisconnect_clicked();
    void connected();
    void disconnected();
    void sockError(QAbstractSocket::SocketError socketError);
    void rowChanged(const QModelIndex &current, const QModelIndex &previous);

    void on_listViewItems_clicked(const QModelIndex &index);

    void on_pushButtonLight_clicked();

    void buttonSceneClicked();

    void on_listViewItems_doubleClicked(const QModelIndex &index);

    void on_pushButtonShutUp_clicked();

    void on_pushButtonShutRUp_clicked();

    void on_pushButtonShutRDown_clicked();

    void on_pushButtonShutDown_clicked();


private:
    Ui::MainWindow *ui;
    iFoxtrotSession session;
};

#endif // MAINWINDOW_H
