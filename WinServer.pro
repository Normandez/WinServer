CONFIG -= qt

TEMPLATE = app
TARGET = WinServer

INCLUDE = include
SRC = src

INCLUDEPATH += $${INCLUDE}

SOURCES += $${SRC}/main.cpp

HEADERS += $${INCLUDE}/SInputParams.h

HEADERS += $${INCLUDE}/CLogger.h
SOURCES += $${SRC}/CLogger.cpp

HEADERS += $${INCLUDE}/CApplication.h
SOURCES += $${SRC}/CApplication.cpp

HEADERS += $${INCLUDE}/CHttpParser.h
SOURCES += $${SRC}/CHttpParser.cpp
