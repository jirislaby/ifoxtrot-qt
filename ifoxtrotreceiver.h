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
	virtual qint64 handleData(QByteArray &data, bool *keep) = 0;

	const QByteArray &getPrefix() const { return prefix; }
	const QByteArray &getWrite() const { return write; }

protected:
	iFoxtrotSession *session;
	const QByteArray prefix;
	const QByteArray write;
};

class iFoxtrotReceiverLine : public iFoxtrotReceiver {
	Q_OBJECT
public:
	explicit iFoxtrotReceiverLine(iFoxtrotSession *session,
	                              const QByteArray &prefix,
	                              const QByteArray &write);

	virtual qint64 handleData(QByteArray &data, bool *keep) override;
	virtual void pushLine(const QString &line) = 0;
};

class iFoxtrotReceiverDIFF : public iFoxtrotReceiverLine {
	Q_OBJECT
public:
	iFoxtrotReceiverDIFF(iFoxtrotSession *session);

	virtual void pushLine(const QString &line) override;
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

private:
	QString file;
	CallbackFn callbackFn;
	QByteArray fileBuffer;
	qint64 continuation;
};

#endif // IFOXTROTRECEIVER_H
