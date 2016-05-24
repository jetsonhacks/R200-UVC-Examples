include(include.pri)

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11
PKGCONFIG += glfw3 gl libusb-1.0
CONFIG += link_pkgconfig
INCLUDEPATH += ../include
LIBS += -pthread

SOURCES += ../examples/cpp-tutorial-1-depth.cpp

LIBS += -L$$DESTDIR -lrealsense

PRE_TARGETDEPS += $$DESTDIR/librealsense.a

