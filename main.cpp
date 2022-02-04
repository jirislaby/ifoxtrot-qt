#include "commandline.h"
#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QStandardPaths>
#include <QTranslator>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QTranslator translator;
	auto paths = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
	paths.prepend(a.applicationDirPath());
	for (auto p : paths) {
		QDir dir(p);
		if (dir.cd("trans") &&
				translator.load(QLocale(), "ifoxtrot-qt", "_",
						       dir.absolutePath())) {
			a.installTranslator(&translator);
			break;
		}
	}

	QCommandLineParser parser;
	parser.setApplicationDescription(a.translate("main",
						     "Frontend for iFoxtrot"));
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addOptions({
		{ { "q", "quiet" },
		  a.translate("main", "Be quiet") },
		{ { "H", "host" },
		  a.translate("main", "Connect to <host>"),
		  a.translate("main", "host") },
		{ { "p", "port" },
		  a.translate("main", "Connect to <port> (default: 5010)"),
		  a.translate("main", "port"),
		  "5010" },
		{ { "P", "PLC" },
		  a.translate("main", "Connect to <PLC>"),
		  a.translate("main", "PLC") },
		{ { "g", "get" },
		  a.translate("main", "Obtain value of <item>"),
		  a.translate("main", "item") },
		{ { "s", "set" },
		  a.translate("main", "Set <value> to <item>"),
		  a.translate("main", "item=value") },
		});
	parser.process(a);

	CommandLine::handleTUI(a, parser);

	MainWindow w;
	w.show();

	return a.exec();
}
