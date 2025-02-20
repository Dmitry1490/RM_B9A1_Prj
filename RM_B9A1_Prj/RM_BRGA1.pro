QT       += core gui
QT      += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dc_power.cpp \
    main.cpp \
    mainwindow.cpp \
    mil_std_1553.cpp \
    port.cpp \
    wdmtmkv2.cpp

HEADERS += \
    dc_power.h \
    globaldef.h \
    mainwindow.h \
    mil_std_1553.h \
    port.h \
    wdmtmkv2.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
