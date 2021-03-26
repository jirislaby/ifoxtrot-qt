#ifndef TREEITEM_H
#define TREEITEM_H

#include <functional>

#include <QString>
#include <QStringList>
#include <QVector>

class TreeItem
{
public:
	TreeItem(const QString &name, TreeItem *parent = nullptr);
	~TreeItem();

	TreeItem *getParent() const { return parent; }
	TreeItem *getChild(int ch) const { return children[ch]; }
	int childrenCount() { return children.count(); }
	bool hasChildren() const;
	QString getName() const { return name; }
	bool isLoaded() const { return loaded; }
	void setLoaded() { loaded = true; }
	QString getPath() const;

	int row() const;

	void addChild(TreeItem *item) { children.append(item); }
	TreeItem *addChild(const QString &name);
private:
	QVector<TreeItem *> children;
	TreeItem *parent;

	QString name;
	bool dir;
	bool loaded;
};

#endif // TREEITEM_H
