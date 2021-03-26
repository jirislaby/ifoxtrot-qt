#include "treeitem.h"

#include <QDebug>
#include <QString>

TreeItem::TreeItem(const QString &name, TreeItem *parent) :
	 parent(parent), loaded(false)
{
	dir = name.endsWith('/');
	this->name = name.chopped(dir);
}

TreeItem::~TreeItem()
{
	qDeleteAll(children);
}

bool TreeItem::hasChildren() const
{
	return dir;
}

QString TreeItem::getPath() const
{
	QString ret(getName());

	if (dir)
		ret.append('/');

	for (auto parent = getParent(); parent; parent = parent->getParent())
		ret.prepend('/').prepend(parent->getName());

	return ret;
}

int TreeItem::row() const
{
    if (parent)
	return parent->children.indexOf(const_cast<TreeItem *>(this));

    return 0;
}

TreeItem *TreeItem::addChild(const QString &name)
{
	auto item = new TreeItem(name, this);

	addChild(item);

	return item;
}
