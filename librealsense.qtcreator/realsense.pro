
include(include.pri)

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

INCLUDEPATH += ../include

CONFIG += link_pkgconfig
PKGCONFIG += libusb-1.0
LIBS += -pthread
# Jetson does not have -mssse3
# QMAKE_CXXFLAGS += -std=c++11 -fPIC -pedantic -mssse3
QMAKE_CXXFLAGS += -std=c++11 -fPIC -pedantic
QMAKE_CXXFLAGS += -Wno-missing-field-initializers -Wno-switch -Wno-multichar
QMAKE_CXXFLAGS += -DRS_USE_LIBUVC_BACKEND

HEADERS += ../include/librealsense/* ../src/*.h
SOURCES += ../src/*.cpp ../src/verify.c

OBJECTS += $$DESTDIR/libuvc/obj/*.o

unix:!macx: PRE_TARGETDEPS += $$DESTDIR/libuvc.a
