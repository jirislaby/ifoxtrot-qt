#ifndef IFOXTROTMODEL_H
#define IFOXTROTMODEL_H

#include <QAbstractListModel>
#include <QObject>

class iFoxtrotCtl;

class iFoxtrotModel : public QAbstractListModel
{
    Q_OBJECT
public:
    iFoxtrotModel(QObject *parent = nullptr) : QAbstractListModel(parent) { }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setList(QList<iFoxtrotCtl *> list);

    void clear();
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    iFoxtrotCtl *at(int index) const { return list.at(index); }

    void changed(const iFoxtrotCtl *ctl);

private:
    QList<iFoxtrotCtl *> list;
};

#endif // IFOXTROTMODEL_H
