#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <treeitem.h>

#include <QAbstractItemModel>
#include <QModelIndex>

class FileTransfer;

class TreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	TreeModel(FileTransfer *ft);
	~TreeModel();

	virtual QModelIndex index(int row, int column,
				  const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &index) const;

	//virtual QModelIndex sibling(int row, int column, const QModelIndex &idx) const;
	virtual int rowCount(const QModelIndex &index = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &index = QModelIndex()) const;
	virtual bool hasChildren(const QModelIndex &index = QModelIndex()) const;

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

	QString getName(const QModelIndex &index) const;
	QString getPath(const QModelIndex &index) const;

	void setRoot(TreeItem *root);

private:
	static TreeItem *getInternal(const QModelIndex &index);

	FileTransfer *ft;
	TreeItem *root;
};

#endif // TREEMODEL_H
