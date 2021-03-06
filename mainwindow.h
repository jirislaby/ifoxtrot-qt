#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDataWidgetMapper>
#include <QMainWindow>
#include <QSortFilterProxyModel>

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
    void conStatusUpdate(const QString &status);
    void disconnected();
    void sockError(QAbstractSocket::SocketError socketError);
    void conError(const QString &reason);
    void rowChanged(const QModelIndex &current, const QModelIndex &previous);

    void on_listViewItems_clicked(const QModelIndex &index);

    void on_pushButtonLight_clicked();

    void buttonSceneClicked();
    void fileTransferTriggered();
    void aboutTriggered();
    void aboutQtTriggered();

    void on_listViewItems_doubleClicked(const QModelIndex &index);

    void on_pushButtonShutUp_clicked();

    void on_pushButtonShutRUp_clicked();

    void on_pushButtonShutRDown_clicked();

    void on_pushButtonShutDown_clicked();


    void on_lineEditFilter_textEdited(const QString &arg1);

    void on_horizontalSliderDimlevel_sliderReleased();

    void on_horizontalSliderDimlevel_actionTriggered(int action);

    void on_doubleSpinBoxDisplayVal_valueChanged(double arg1);

    void on_TPW_SB_delta_valueChanged(double arg1);

private:
    Ui::MainWindow *ui;
    iFoxtrotSession session;
    QSortFilterProxyModel *proxyModel;
    QDataWidgetMapper widgetMapper;
    bool changingVals;

    iFoxtrotCtl *getCurrentCtl() const;
};

#endif // MAINWINDOW_H
