#-------------------------------------------------
#
# Project created by QtCreator 2009-07-24T15:31:41
#
#-------------------------------------------------

TARGET = tsDemuxGUI
TEMPLATE = app
VPATH += ../ ../ps3muxer
INCLUDEPATH += ../ ../ps3muxer


SOURCES += main.cpp\
        mainwindow.cpp\
        mpls.cpp\
        execwindow.cpp

HEADERS  += mainwindow.h\
        mpls.h\
        execwindow.h

FORMS    += mainwindow.ui\
        execwindow.ui

TRANSLATIONS += tsDemuxGUI_ru.ts tsDemuxGUI_it.ts

system(lupdate tsDemuxGUI.pro)
system(lrelease tsDemuxGUI.pro)

win32 {
    DEFINES += _CRT_SECURE_NO_WARNINGS
}
