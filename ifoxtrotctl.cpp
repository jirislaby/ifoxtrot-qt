#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QTextCodec>

#include "ifoxtrotctl.h"
#include "ifoxtrotsession.h"
#include "ui_mainwindow.h"

iFoxtrotCtl *iFoxtrotCtl::getOne(iFoxtrotSession *session,
                                 const QString &foxType, const QString &foxName)
{
    if (foxType == "DISPLAY")
        return new iFoxtrotDisplay(session, foxName);
    if (foxType == "LIGHT")
        return new iFoxtrotLight(session, foxName);
    if (foxType == "PIRSENSOR")
        return new iFoxtrotPIRSENSOR(session, foxName);
    if (foxType == "RELAY")
        return new iFoxtrotRelay(session, foxName);
    if (foxType == "SCENE")
        return new iFoxtrotScene(session, foxName);
    if (foxType == "SHUTTER")
        return new iFoxtrotShutter(session, foxName);
    if (foxType == "TPW")
        return new iFoxtrotTPW(session, foxName);

    return nullptr;
}

QString iFoxtrotCtl::getFoxString(const QString &val)
{
	QString ret(val);

	if (!val.startsWith('"') || !val.endsWith('"'))
		return QString();

	ret.remove(0, 1);
	ret.chop(1);

	return ret;
}

bool iFoxtrotCtl::setProp(const QString &prop, const QString &val)
{
    if (prop == "ENABLE")
        return true;

    if (prop == "NAME") {
	    name = getFoxString(val);
	    if (name == "") {
		    qWarning() << "wrong name for" << foxName << ":" << val;
		    return false;
	    }

        return true;
    }

    qWarning() << "unknown property" << prop << "for" << getFoxType();

    return false;
}

QByteArray iFoxtrotCtl::GTSAP(const QString &prefix, const QString &prop,
                              const QString &set) const
{
    QByteArray ret;

    ret.append(prefix).append(':').append(getFoxName()).append(".GTSAP1_").
            append(getFoxType()).append('_').append(prop);

    if (!set.isEmpty())
        ret.append(',').append(set);

    ret.append('\n');

    return ret;
}

QVariant iFoxtrotCtl::data(int column, int role) const
{
    if (column > 1)
        qDebug() << __PRETTY_FUNCTION__ << column;

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return getFoxType().mid(0, 1) + ' ' + getName();

    if (role == Qt::BackgroundRole)
        return QBrush(getColor());

    return QVariant();
}

void iFoxtrotCtl::changed(const QString &prop)
{
    Q_UNUSED(prop);
    session->getModel()->changed(this);
}

void iFoxtrotOnOff::switchState()
{
    QByteArray req = GTSAP("SET", "ONOFF", onOff ? "0" : "1");
    qDebug() << "REQ" << req;
    session->write(req);
}

bool iFoxtrotOnOff::setProp(const QString &prop, const QString &val)
{
    if (prop == "ONOFF") {
        onOff = (val == "1");
        changed(prop);
        return true;
    }

    return iFoxtrotCtl::setProp(prop, val);
}

QVariant iFoxtrotOnOff::data(int column, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
        case 1:
            return onOff ? "1" : "0";
        }
    }

    if (role == Qt::FontRole && onOff) {
        QFont f;
        f.setWeight(QFont::Bold);
        return f;
    }

    return iFoxtrotCtl::data(column, role);
}

bool iFoxtrotLight::setProp(const QString &prop, const QString &val)
{
    if (prop == "TYPE") {
        dimmable = (val == "1");
        return true;
    }
    if (prop == "DIMTYPE") {
        rgb = (val == "1");
        return true;
    }
    if (prop == "DIMLEVEL") {
        dimlevel = val.toFloat();
        changed(prop);
        return true;
    }

    return iFoxtrotOnOff::setProp(prop, val);
}

void iFoxtrotLight::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    //ui->labelLightStatus->setText(onOff ? "1" : "0");
    ui->horizontalSliderDimlevel->setVisible(dimmable);
    ui->horizontalSliderDimlevel->setValue((int)dimlevel);
    widgetMapper.addMapping(ui->labelLightStatus, 1, "text");
    widgetMapper.addMapping(ui->horizontalSliderDimlevel, 2, "value");
}

QVariant iFoxtrotLight::data(int column, int role) const
{
    if (column > 0)
        qDebug() << __func__ << column << role;

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
        case 2:
            return (int)dimlevel;
        }
    }

    return iFoxtrotOnOff::data(column, role);
}

bool iFoxtrotRelay::setProp(const QString &prop, const QString &val)
{
    if (prop == "SYMBOL") {
        return true;
    }
    if (prop == "TYPE") {
        return true;
    }

    return iFoxtrotOnOff::setProp(prop, val);
}

void iFoxtrotRelay::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    widgetMapper.addMapping(ui->labelRelayStatus, 1, "text");
}

bool iFoxtrotDisplay::setProp(const QString &prop, const QString &val)
{
    if (prop == "EDIT") {
        editable = (val == "1");
        return true;
    }
    if (prop == "TYPE") {
        real = (val == "1");
        return true;
    }
    if (prop == "SYMBOL") {
        return true;
    }
    if (prop == "VALUE") {
        value = val.toDouble();
        changed(prop);
        return true;
    }
    if (prop == "UNIT") {
        unit = val;
        unit.remove(0, 1);
        unit.chop(1);
        return true;
    }
    if (prop == "PRECISION") {
        precision = val.toInt();
        return true;
    }
    if (prop == "URL") {
        return true;
    }
    if (prop == "INCVALUE") {
        return true;
    }
    if (prop == "DECVALUE") {
        return true;
    }
    if (prop == "VALUESET") {
        return true;
    }

    if (iFoxtrotCtl::setProp(prop, val))
        return true;

    return false;
}

void iFoxtrotDisplay::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    ui->doubleSpinBoxDisplayVal->setReadOnly(!editable);
    ui->doubleSpinBoxDisplayVal->setDecimals(precision);
    ui->doubleSpinBoxDisplayVal->setSuffix(" " + unit);
    widgetMapper.addMapping(ui->doubleSpinBoxDisplayVal, 1, "value");
}

QVariant iFoxtrotDisplay::data(int column, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
        case 1:
            return value;
        }
    }

    return iFoxtrotCtl::data(column, role);
}

bool iFoxtrotPIRSENSOR::setProp(const QString &prop, const QString &val)
{
    if (prop == "VALUE") {
	    bool ok;
        value = val.toInt(&ok);
        if (!ok)
            qWarning() << "invalid PIR value" << val;
        changed(prop);
        return true;
    }

    return iFoxtrotCtl::setProp(prop, val);
}

void iFoxtrotPIRSENSOR::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    widgetMapper.addMapping(ui->labelPIRSENSORValue, 1, "text");
}

QVariant iFoxtrotPIRSENSOR::data(int column, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
        case 1:
            return value;
        }
    }

    return iFoxtrotCtl::data(column, role);
}

bool iFoxtrotShutter::setProp(const QString &prop, const QString &val)
{
    if (prop == "UP") {
        status = val.toInt() ? MovingUp : Steady;
        changed(prop);
        return true;
    }
    if (prop == "DOWN") {
        status = val.toInt() ? MovingDown : Steady;
        changed(prop);
        return true;
    }
    if (prop == "RUN") {
        running = !!val.toInt();
        changed(prop);
        return true;
    }
    if (prop == "UPPOS") {
        upPos = !!val.toInt();
        changed(prop);
        return true;
    }
    if (prop == "DOWNPOS") {
        downPos = !!val.toInt();
        changed(prop);
        return true;
    }
    if (prop == "UP_CONTROL" ||
            prop == "DOWN_CONTROL" ||
            prop == "ROTUP_CONTROL" ||
            prop == "ROTDOWN_CONTROL")
        return true;

    return iFoxtrotCtl::setProp(prop, val);
}

void iFoxtrotShutter::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    widgetMapper.addMapping(ui->labelShutterStatus, 1, "text");
    widgetMapper.addMapping(ui->labelShutterPos, 2, "text");
    widgetMapper.addMapping(ui->labelShutterRunning, 3, "text");
}

QVariant iFoxtrotShutter::data(int column, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
        case 0: {
			QString suffix;

			if (upPos)
				suffix = " ↑";
			else if (downPos)
				suffix = " ↓";
			else if (status == MovingUp)
				suffix = " ⇡";
			else if (status == MovingDown)
				suffix = " ⇣";

			return iFoxtrotCtl::data(column, role).toString() + suffix;
		}
        case 1:
            return stringStatus();
        case 2:
            return stringPosition();
        case 3:
            return running ? "Yes" : "No";
        }
    }

    if (role == Qt::FontRole && running) {
        QFont f;
        f.setWeight(QFont::Bold);
        return f;
    }

    return iFoxtrotCtl::data(column, role);
}

void iFoxtrotShutter::up()
{
    QByteArray req = GTSAP("SET", "UP_CONTROL", "1");
    qDebug() << req;
    session->write(req);
}

void iFoxtrotShutter::down()
{
    QByteArray req = GTSAP("SET", "DOWN_CONTROL", "1");
    qDebug() << req;
    session->write(req);
}

void iFoxtrotShutter::rotUp()
{
    QByteArray req = GTSAP("SET", "ROTUP_CONTROL", "1");
    qDebug() << req;
    session->write(req);
}

void iFoxtrotShutter::rotDown()
{
    QByteArray req = GTSAP("SET", "ROTDOWN_CONTROL", "1");
    qDebug() << req;
    session->write(req);
}

QString iFoxtrotShutter::stringStatus() const
{
    switch (status) {
    case Steady:
        return "Steady";
    case MovingUp:
        return "Moving Up";
    case MovingDown:
        return "Moving Down";
    }
    return QString();
}

QString iFoxtrotShutter::stringPosition() const
{
    if (upPos)
        return "Up";
    if (downPos)
        return "Down";

    return "Unknown";
}

bool iFoxtrotScene::setProp(const QString &prop, const QString &val)
{
    if (prop == "FILE") {
	    filename = getFoxString(val);
	    if (filename == "" || !filename.endsWith('?')) {
		    qWarning() << "wrong file for" << foxName << ":" << val;
		    return false;
	    }
	    filename.chop(1);
        return true;
    }
    if (prop == "NUM") {
        bool ok;
        scenes = val.toInt(&ok);
        if (!ok || (scenes != 4 && scenes != 8))
            qWarning() << "invalid scene NUM" << val;
        return true;
    }
    if (prop.length() == 4 && prop.startsWith("SET") && prop.at(3).isDigit()) {
        return true;
    }

    return iFoxtrotCtl::setProp(prop, val);
}

void iFoxtrotScene::postReceive()
{
	QTextCodec *codec = QTextCodec::codecForName("Windows 1250");
	for (int a = 1; a <= scenes; a++) {
		QString src(filename);
		src.append(a + '0');
		qDebug() << __PRETTY_FUNCTION__ << foxName << src;

		session->receiveFile(src, [this, a, src, codec](const QByteArray &data) {
			QJsonParseError error;
			auto doc = QJsonDocument::fromJson(codec->toUnicode(data).toUtf8(),
			                                   &error);
			if (doc.isNull() || !doc.isObject()) {
				qWarning() << "invalid scene json" << foxName << src <<
				              error.errorString();
				return;
			}
			auto obj = doc.object();
			QJsonObject::iterator idx = obj.find("index");
			QJsonObject::iterator name = obj.find("name");
			if (idx == obj.end() || name == obj.end() ||
			                !idx.value().isDouble() || idx.value().toInt() != a ||
			                !name.value().isString()) {
				qWarning() << "invalid scene json" << foxName << src;
				return;
			}
			qDebug() << name.value().toString();
			sceneNames[a - 1] = name.value().toString();
		});
	}
}

void iFoxtrotScene::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    Q_UNUSED(widgetMapper);

    for (QPushButton *but : ui->page_SCENE->findChildren<QPushButton *>(QString(),
                                    Qt::FindDirectChildrenOnly)) {
        QString name = but->objectName();

        if (!name.startsWith("pushButtonSc"))
            continue;

        int which = name.remove(0, sizeof "pushButtonSc" - 1).toInt();
        but->setEnabled(which <= scenes);
        if (sceneNames[which - 1] != "")
			but->setText(sceneNames[which - 1]);
		else
			but->setText("Scene " + QString::number(which));
    }
}

bool iFoxtrotTPW::setProp(const QString &prop, const QString &val)
{
    bool ok;

    if (prop == "DECDELTA") {
        return true;
    }
    if (prop == "INCDELTA") {
        return true;
    }
    if (prop == "MANUALSET") {
        return true;
    }
    if (prop == "MODESET") {
        return true;
    }
    if (prop == "NEXTMODE") {
        return true;
    }
    if (prop == "NEXTPROG") {
        return true;
    }
    if (prop == "TYPE") {
        type = val.toInt(&ok);
        if (!ok || type < 1 || type > 3)
            qWarning() << "invalid TPW type" << val;
        changed(prop);
        return true;
    }
    if (prop == "FILE") {
	    file = getFoxString(val);
	    if (file == "") {
		    qWarning() << "wrong file for" << foxName << ":" << val;
		    return false;
	    }
	    return true;
    }
    if (prop == "CRC") {
        crc = val.toUInt(&ok);
        if (!ok)
            qWarning() << "invalid TPW crc" << val;
        return true;
    }
    if (prop == "UPDATE") {
        return true;
    }
    if (prop == "MANUAL") {
        manual = (val == "1");
        changed(prop);
        return true;
    }
    if (prop == "HOLIDAY") {
        holiday = (val == "1");
        changed(prop);
        return true;
    }
    if (prop == "HEAT") {
        heat = (val == "1");
        changed(prop);
        return true;
    }
    if (prop == "COOL") {
        cool = (val == "1");
        changed(prop);
        return true;
    }
    if (prop == "ROOMTEMP") {
        roomTemp = val.toDouble(&ok);
        if (!ok)
            qWarning() << "invalid TPW room temp" << val;
        changed(prop);
        return true;
    }
    if (prop == "HEATTEMP") {
        heatTemp = val.toDouble(&ok);
        if (!ok)
            qWarning() << "invalid TPW heat temp" << val;
        changed(prop);
        return true;
    }
    if (prop == "COOLTEMP") {
        coolTemp = val.toDouble(&ok);
        if (!ok)
            qWarning() << "invalid TPW cool temp" << val;
        changed(prop);
        return true;
    }
    if (prop == "MODE") {
        mode = val.toInt(&ok);
        if (!ok)
            qWarning() << "invalid TPW mode" << val;
        changed(prop);
        return true;
    }
    if (prop == "DELTA") {
        delta = val.toDouble(&ok);
        if (!ok)
            qWarning() << "invalid TPW delta" << val;
        changed(prop);
        return true;
    }

    return iFoxtrotCtl::setProp(prop, val);
}

void iFoxtrotTPW::postReceive()
{
	QTextCodec *codec = QTextCodec::codecForName("Windows 1250");

	session->receiveFile(file, [this, codec](const QByteArray &data) {
		QJsonParseError error;
		auto doc = QJsonDocument::fromJson(codec->toUnicode(data).toUtf8(),
		                                   &error);
		if (doc.isNull() || !doc.isObject()) {
			qWarning() << "invalid TPW json" << foxName << file <<
			              error.errorString();
			return;
		}
		auto obj = doc.object();
		QJsonObject::iterator name = obj.find("Name");
		QJsonObject::iterator TPW = obj.find("timeProgWeekData");
		if (name == obj.end() || TPW == obj.end() ||
		                !name.value().isString() ||
		                !TPW.value().isObject()) {
			qWarning() << "invalid scene json" << foxName << file;
			return;
		}
		/*auto TPWo = TPW.value().toObject();
		qDebug() << TPWo;*/
		this->name = name.value().toString();
	});
}

void iFoxtrotTPW::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    widgetMapper.addMapping(ui->TPW_RB_heat, 1, "checked");
    widgetMapper.addMapping(ui->TPW_RB_cool, 2, "checked");
    widgetMapper.addMapping(ui->TPW_RB_both, 3, "checked");
    widgetMapper.addMapping(ui->TPW_CB_manual, 4, "checked");
    widgetMapper.addMapping(ui->TPW_CB_holiday, 5, "checked");
    widgetMapper.addMapping(ui->TPW_CB_heat, 6, "checked");
    widgetMapper.addMapping(ui->TPW_CB_cool, 7, "checked");
    widgetMapper.addMapping(ui->TPW_SB_room, 8, "value");
    widgetMapper.addMapping(ui->TPW_SB_heat, 9, "value");
    widgetMapper.addMapping(ui->TPW_SB_cool, 10, "value");
    widgetMapper.addMapping(ui->TPW_RB_prot, 11, "checked");
    widgetMapper.addMapping(ui->TPW_RB_eco, 12, "checked");
    widgetMapper.addMapping(ui->TPW_RB_low, 13, "checked");
    widgetMapper.addMapping(ui->TPW_RB_comf, 14, "checked");
    widgetMapper.addMapping(ui->TPW_SB_delta, 15, "value");
    widgetMapper.addMapping(ui->TPW_name, 16, "text");
}

QVariant iFoxtrotTPW::data(int column, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
        case 1:
            return type == 1;
        case 2:
            return type == 2;
        case 3:
            return type == 3;
        case 4:
            return manual;
        case 5:
            return holiday;
        case 6:
            return heat;
        case 7:
            return cool;
        case 8:
            return roomTemp;
        case 9:
            return heatTemp;
        case 10:
            return coolTemp;
        case 11:
            return mode == 0;
        case 12:
            return mode == 1;
        case 13:
            return mode == 2;
        case 14:
            return mode == 3;
        case 15:
            return delta;
        case 16:
            return name;
        }
    }

    return iFoxtrotCtl::data(column, role);
}
