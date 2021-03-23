#-------------------------------------------------
#
# Project created by QtCreator 2019-02-07T13:48:08
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ifoxtrot-qt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
	filetransfer.cpp \
	ifoxtrotreceiver.cpp \
	main.cpp \
	mainwindow.cpp \
	ifoxtrotctl.cpp \
	ifoxtrotmodel.cpp \
	ifoxtrotsession.cpp

HEADERS += \
	filetransfer.h \
	ifoxtrotreceiver.h \
	mainwindow.h \
	ifoxtrotctl.h \
	ifoxtrotmodel.h \
	ifoxtrotsession.h

FORMS += \
        filetransfer.ui \
        mainwindow.ui

TRANSLATIONS += \
	trans/ifoxtrot-qt_cs.ts

trans.files += trans/ifoxtrot-qt_cs.qm

QMAKE_CXXFLAGS += -Wno-unused-parameter

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin

unix:!android {
	isEmpty(PREFIX): PREFIX = /usr
	BINDIR = $$PREFIX/bin
	DATADIR = $$PREFIX/share
	TRANS_DIR = "$${DATADIR}/$$TARGET/trans"

	target.path = $$BINDIR

	desktop.path = "$${DATADIR}/applications"
	desktop.files += ifoxtrot-qt.desktop

	trans.path = "$$TRANS_DIR"

	INSTALLS += desktop
}

win32 {
	isEmpty(bindir): bindir = bin
	isEmpty(datadir): datadir = share

	DATADIR = $$datadir
	TRANS_DIR = "$${DATADIR}/$$TARGET/trans"

	target.path = $$bindir
	trans.path = "$$TRANS_DIR"
}

!isEmpty(DATADIR): DEFINES += DATADIR=\\\"$$DATADIR\\\"

!isEmpty(target.path): INSTALLS += target
!isEmpty(trans.path): INSTALLS += trans
