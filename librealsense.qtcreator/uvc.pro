include(LRS.pri)

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

# INCLUDEPATH += ../third_party/libuvc/include/
SOURCES += \
    ../src/libuvc/ctrl.c \
    ../src/libuvc/dev.c \
    ../src/libuvc/diag.c \
    ../src/libuvc/frame.c \
    ../src/libuvc/init.c \
    ../src/libuvc/stream.c

HEADERS += \
    ../src/libuvc/libuvc.h \
    ../src/libuvc/libuvc_config.h \
    ../src/libuvc/libuvc_internal.h \
    ../src/libuvc/utlist.h

CONFIG += link_pkgconfig
PKGCONFIG += libusb-1.0
LIBS += -pthread -ljpeg
# QMAKE_CFLAGS += -fPIC -DUVC_DEBUGGING
QMAKE_CFLAGS += -fPIC -DUVC_DEBUGGING
