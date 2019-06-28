#ifndef IFOXTROTRECEIVER_H
#define IFOXTROTRECEIVER_H

#include <functional>

#include <QByteArray>
#include <QObject>
#include <QString>

class iFoxtrotSession;

class iFoxtrotReceiver : public QObject
{
	Q_OBJECT
public:
	explicit iFoxtrotReceiver(iFoxtrotSession *session,
	                          QObject *parent = nullptr);

	virtual bool handleData(QByteArray &data);

signals:
	void hasData(const QString &line);

public slots:

protected:
	iFoxtrotSession *session;
	bool byLines;
};

class iFoxtrotReceiverFile : public iFoxtrotReceiver
{
	Q_OBJECT
public:
	typedef std::function<void(iFoxtrotReceiverFile *,
			const QByteArray &)> CallbackFn;

	iFoxtrotReceiverFile(iFoxtrotSession *session, const QString &file,
	                     const CallbackFn &fun,
	                     QObject *parent = nullptr);

	virtual bool handleData(QByteArray &data) override;
signals:
private:
	QString file;
	CallbackFn callbackFn;
	QByteArray fileBuffer;
};

#endif // IFOXTROTRECEIVER_H
