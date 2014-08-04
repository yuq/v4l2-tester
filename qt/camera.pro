# QT Modules
QT += qml quick

# Target
TARGET = camera
TEMPLATE = app

# Extra Configs
CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

# Sources and Headers
INCLUDEPATH += /home/yuq/projects/a10/linux-a10/include

SOURCES += main.cpp \
    sunxi_tvd_camera.cpp \
    camera_texture.cpp \
    yuv2rgb_material.cpp \
    camera_player.cpp \
    imagestream.cpp

HEADERS += \
    sunxi_tvd_camera.h \
    camera_texture.h \
    yuv2rgb_material.h \
    camera_player.h \
    imagestream.h

# Qml Files
OTHER_FILES += main.qml

# Resources
RESOURCES += qml.qrc
QMAKE_RESOURCE_FLAGS += -no-compress

target.path = /opt/HuaTai
INSTALLS += target

# Objects Pathes
DESTDIR = bin
UI_DIR = build
MOC_DIR = build
RCC_DIR = build
OBJECTS_DIR = build
