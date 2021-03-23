#include "mainwindow.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTranslator>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QTranslator translator;
	auto paths = QStandardPaths::standardLocations(QStandardPaths::DataLocation);
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

	MainWindow w;
	w.show();

	return a.exec();
}
