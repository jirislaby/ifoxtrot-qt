#include <QBrush>
#include <QColor>
#include <QCollator>
#include <QDebug>
#include <QFont>

#include "ifoxtrotctl.h"
#include "ifoxtrotmodel.h"

int iFoxtrotModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return list.size();
}

int iFoxtrotModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    //qDebug() << __PRETTY_FUNCTION__;
    return 2;
}

QVariant iFoxtrotModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= list.count())
        return QVariant();
    if (index.column() > 1)
        qDebug() << __PRETTY_FUNCTION__ << index;

    iFoxtrotCtl *item = list.at(index.row());

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return item->getFoxType().mid(0, 1) + ' ' + item->getName();

    if (role == Qt::FontRole) {
        if (item->isBold()) {
            QFont f;
            f.setWeight(QFont::Bold);
            return f;
        }
        return QVariant();
    }

    if (role == Qt::BackgroundRole)
        return QBrush(item->getColor());

//    qDebug() << __func__ << role << index.row();

    return QVariant();
}

void iFoxtrotModel::setList(QList<iFoxtrotCtl *> list)
{
    beginResetModel();
    this->list = std::move(list);
    endResetModel();
}

void iFoxtrotModel::clear()
{
    beginResetModel();
    list.clear();
    endResetModel();
}

static bool foxSortAsc(const iFoxtrotCtl *a, const iFoxtrotCtl *b)
{
    int typeCmp = a->getFoxType().compare(b->getFoxType());

    if (typeCmp < 0)
        return true;

    if (typeCmp == 0)
        return QCollator().compare(a->getName(), b->getName()) < 0;

    return false;
}

static bool foxSortDesc(const iFoxtrotCtl *a, const iFoxtrotCtl *b)
{
    int typeCmp = a->getFoxType().compare(b->getFoxType());

    if (typeCmp < 0)
        return true;

    if (typeCmp == 0)
        return QCollator().compare(a->getName(), b->getName()) > 0;

    return false;
}

void iFoxtrotModel::sort(int column, Qt::SortOrder order)
{
    if (column == 0)
        std::sort(list.begin(), list.end(),
                  order == Qt::SortOrder::AscendingOrder ? foxSortAsc : foxSortDesc);
}

void iFoxtrotModel::changed(const iFoxtrotCtl *ctl)
{
    int idx = 0;

    // slow as hell -- how to find an index of an item?
    for (const auto c: list) {
        if (ctl == c) {
            const QModelIndex index = createIndex(idx, 0);
            emit dataChanged(index, index);
            return;
        }
        idx++;
    }
}
