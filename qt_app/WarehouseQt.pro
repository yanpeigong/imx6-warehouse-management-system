# ============================================================
# 仓库管理系统 QT工程文件
# 编译方法：
#   1. source /opt/fsl-imx-wayland/4.9.88-2.0.0/environment-setup-cortexa9hf-neon-poky-linux-gnueabi
#   2. qmake WarehouseQt.pro
#   3. make
# ============================================================

QT       += core gui serialport network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
TARGET   = WarehouseQt
TEMPLATE = app

# 直接链接 sqlite3，不再依赖 Qt SQL 模块和 libqsqlite.so plugin
LIBS    += -lsqlite3

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    logindialog.cpp \
    serialreader.cpp \
    dbmanager.cpp \
    httpserver.cpp \
    stockindialog.cpp \
    stockoutdialog.cpp \
    inventorydialog.cpp \
    logdialog.cpp

HEADERS += \
    mainwindow.h \
    logindialog.h \
    serialreader.h \
    dbmanager.h \
    httpserver.h \
    stockindialog.h \
    stockoutdialog.h \
    inventorydialog.h \
    logdialog.h

# 中文支持
DEFINES += QT_DEPRECATED_WARNINGS

# 在目标板上的安装路径（可选）
target.path = /opt/warehouse
INSTALLS += target
