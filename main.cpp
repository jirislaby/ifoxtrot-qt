#include "mainwindow.h"
#include <QApplication>
#include <QStandardPaths>
#include <QTranslator>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QTranslator translator;
	auto paths = QStandardPaths::standardLocations(QStandardPaths::DataLocation);
	paths.prepend(a.applicationDirPath());
	for (auto p : paths) {
		if (translator.load(QLocale(), "ifoxtrot-qt", "_",
				    p + "/trans")) {
			a.installTranslator(&translator);
			break;
		}
	}

	MainWindow w;
	w.show();

	return a.exec();
}
