#include <QBrush>
#include <QColor>
#include <QCollator>
#include <QDebug>
#include <QFont>

#include "ifoxtrotmodel.h"

int iFoxtrotModel::rowCount(const QModelIndex &parent) const
{
    return list.size();
}

QVariant iFoxtrotModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= list.count())
        return QVariant();

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
    this->list = list;
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
