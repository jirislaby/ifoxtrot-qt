#ifndef IFOXTROTMODEL_H
#define IFOXTROTMODEL_H

#include <QAbstractListModel>
#include <QObject>

class iFoxtrotCtl;

class iFoxtrotModel : public QAbstractListModel
{
    Q_OBJECT
public:
    iFoxtrotModel(QObject *parent = nullptr) : QAbstractListModel(parent), list() { }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setList(QList<iFoxtrotCtl *> list);

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    iFoxtrotCtl *at(int index) const { return list.at(index); }
private:
    QList<iFoxtrotCtl *> list;
};

#if 0
public:

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    QStringList stringList() const;
    void setStringList(const QStringList &strings);

    Qt::DropActions supportedDropActions() const override;

private:
    QStringList lst;
};
#endif

#endif // IFOXTROTMODEL_H
