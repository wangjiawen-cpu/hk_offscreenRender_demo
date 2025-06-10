QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

PROJECT_DIR = $$PWD

INCLUDE_DIR = $$PROJECT_DIR/include
LIBRARY_DIR = $$PROJECT_DIR/library

#### INCLUDE DIR
HIK_INC = $$INCLUDE_DIR/hikvision

#### LIBDIR
HIK_LIB = $$LIBRARY_DIR/hikvision


SOURCES += \
    checkNetwork.cpp \
    hikvisionipccamera.cpp \
    hikvisionsdk.cpp \
    main.cpp \
    videoplayopengl.cpp \
    videorenderthread.cpp \
    videowidget.cpp \
    widget.cpp

HEADERS += \
    VideoRenderThread.h \
    checkNetwork.h \
    hikvisionipccamera.h \
    hikvisionsdk.h \
    include/IpcSdkDef.h \
    include/basic.h \
    videoplayopengl.h \
    videorenderthread.h \
    videowidget.h \
    widget.h

FORMS += \
    widget.ui

INCLUDEPATH += \
        $$INCLUDE_DIR \
        $$HIK_INC

LIBS += -L$$HIK_LIB -lhcnetsdk -lPlayCtrl -lAudioRender -lSuperRender -lGL

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    shaders/shader.frag \
    shaders/shader.vert

RESOURCES += \
    qrs.qrc
