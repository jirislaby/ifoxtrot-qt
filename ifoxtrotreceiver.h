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
	                          const QByteArray &prefix,
	                          const QByteArray &write);

	virtual bool handleError(QByteArray &data) { Q_UNUSED(data); return true; }
	virtual qint64 handleData(QByteArray &data, bool *keep);

	const QByteArray &getPrefix() const { return prefix; }

	const QByteArray &getWrite() const { return write; }

signals:
	void hasData(const QString &line);

public slots:

protected:
	iFoxtrotSession *session;
	const QByteArray prefix;
	const QByteArray write;
};

class iFoxtrotReceiverFile : public iFoxtrotReceiver
{
	Q_OBJECT
public:
	typedef std::function<void(iFoxtrotReceiverFile *,
			const QByteArray &)> CallbackFn;

	iFoxtrotReceiverFile(iFoxtrotSession *session,
	                     const QByteArray &write,
	                     const QString &file,
	                     const CallbackFn &fun);

	virtual bool handleError(QByteArray &data) override;
	virtual qint64 handleData(QByteArray &data, bool *keep) override;
signals:
private:
	QString file;
	CallbackFn callbackFn;
	QByteArray fileBuffer;
	qint64 continuation;
};

#endif // IFOXTROTRECEIVER_H
