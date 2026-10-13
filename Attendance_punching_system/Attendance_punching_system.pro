QT       += core gui
QT       += sql #添加数据库模块

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

SOURCES += \
    databasecontrol.cpp \
    dialog.cpp \
    main.cpp \
    mainwindow.cpp \
    manager_dialog.cpp \
    registration_dialog.cpp \
    userinfo.cpp

HEADERS += \
    databasecontrol.h \
    dialog.h \
    mainwindow.h \
    manager_dialog.h \
    registration_dialog.h \
    userinfo.h

FORMS += \
    dialog.ui \
    mainwindow.ui \
    manager_dialog.ui \
    registration_dialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


# 指定opencv头文件所在路径
INCLUDEPATH += D:/opencv4.5.5-MinGw7.3.0/include
# 指定opencv库文件路径及库名称
LIBS += -LD:/opencv4.5.5-MinGw7.3.0/x64/mingw/bin -lopencv_world455
