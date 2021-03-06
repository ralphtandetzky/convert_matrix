QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QMAKE_CXXFLAGS += -std=c++11 -pedantic

TEMPLATE = app
CONFIG += c++11 link_prl

DEPENDPATH += . ../cpp_utils/ ../qt_utils/
INCLUDEPATH += ..

HEADERS  += \
	gui_main_window.h \

SOURCES += main.cpp\
	gui_main_window.cpp \

FORMS    += \
	gui_main_window.ui

LIBS += \
	-L../qt_utils -lqt_utils \
#	-L/usr/lib/ -lopencv_core -lopencv_imgproc -lopencv_highgui \
	-L../cpp_utils -lcpp_utils \

