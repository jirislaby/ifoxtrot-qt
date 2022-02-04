cmake_minimum_required(VERSION 3.12.0)

project(ifoxtrot-qt VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (UNIX)
	add_compile_options(-Wall -Wno-unused-parameter)
endif()
if (USE_WEXTRA)
	add_compile_options(-Wextra -pedantic -Wno-unused-parameter)
endif()
if (USE_WERROR)
	add_compile_options(-Werror)
endif()

set(QT_COMPONENTS Core Gui LinguistTools Network Widgets)
find_package(Qt6 QUIET COMPONENTS ${QT_COMPONENTS})
if (NOT Qt6_FOUND)
	find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})
endif()

qt_add_translation(QM_FILES
	trans/ifoxtrot-qt_cs.ts)

add_compile_definitions(QT_NO_FOREACH QT_DEPRECATED_WARNINGS)
add_compile_definitions(DATADIR="${CMAKE_INSTALL_FULL_DATADIR}")

add_executable(ifoxtrot-qt
	commandline.cpp
	filetransfer.cpp
	filetransfer.ui
	ifoxtrotreceiver.cpp
	main.cpp
	mainwindow.cpp
	mainwindow.ui
	ifoxtrotctl.cpp
	ifoxtrotmodel.cpp
	ifoxtrotsession.cpp
	treeitem.cpp
	treemodel.cpp
	commandline.h
	filetransfer.h
	ifoxtrotreceiver.h
	mainwindow.h
	ifoxtrotctl.h
	ifoxtrotmodel.h
	ifoxtrotsession.h
	treeitem.h
	treemodel.h
	${QM_FILES}
)

target_link_libraries(ifoxtrot-qt Qt::Core Qt::Gui Qt::Network Qt::Widgets)

install(TARGETS ifoxtrot-qt
	RUNTIME DESTINATION bin
)

install(FILES ${QM_FILES}
	DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/trans/
)

if (UNIX)
	install(FILES ifoxtrot-qt.desktop
		DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
	)
endif()
