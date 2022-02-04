#include "treemodel.h"

#include <QDebug>

#include "filetransfer.h"

TreeModel::TreeModel(FileTransfer *ft) : ft(ft), root(nullptr)
{
}

TreeModel::~TreeModel()
{
	delete root;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
	/*if (parent.isValid())
		qDebug() << __func__ << parent;
	qDebug() << __func__ << row << column;*/

	if (!hasIndex(row, column, parent))
		return QModelIndex();

	auto parItem = parent.isValid() ? getInternal(parent) : root;

	return createIndex(row, column, parItem->getChild(row));
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	auto parItem = getInternal(index)->getParent();
	if (parItem == root)
		return QModelIndex();

	return createIndex(parItem->row(), 0, parItem);
}

int TreeModel::rowCount(const QModelIndex &index) const
{
	auto item = index.isValid() ? getInternal(index) : root;

	//qDebug() << __func__ << "parent" << par->getName() << "->" << par->childrenCount();

	if (!item->isLoaded()) {
		item->setLoaded();
		auto path = new QString(item->getPath());
		//auto m = const_cast<TreeModel *>(this);
		ft->load(*path, [/*m,*/ item, path](const QStringList &entries) -> void {
			for (auto e : entries) {
				if (!e.startsWith(*path)) {
					qWarning() << "entry does not conform to path";
					continue;
				}
				item->addChild(e.mid(path->count()));
			}
			/*auto idx = m->createIndex(item->row(), 0, item);
			emit m->dataChanged(idx, idx);*/
			delete path;
		});
	}

	return item->childrenCount();
}

int TreeModel::columnCount(const QModelIndex &index) const
{
	Q_UNUSED(index);
	return 1;
}

bool TreeModel::hasChildren(const QModelIndex &index) const
{
	if (index.isValid())
		return getInternal(index)->hasChildren();

	return root ? root->childrenCount() : false;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role != Qt::DisplayRole)
		return QVariant();

	return getInternal(index)->getName();
}

QString TreeModel::getPath(const QModelIndex &index) const
{
	auto item = index.isValid() ? getInternal(index) : root;

	return item->getPath();
}

QString TreeModel::getName(const QModelIndex &index) const
{
	auto item = index.isValid() ? getInternal(index) : root;

	return item->getName();
}

void TreeModel::setRoot(TreeItem *root)
{
	beginResetModel();
	delete this->root;
	root->setLoaded();
	this->root = root;
	endResetModel();
}

TreeItem *TreeModel::getInternal(const QModelIndex &index)
{
	return static_cast<TreeItem *>(index.internalPointer());
}
