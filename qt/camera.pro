TEMPLATE = app

QT += qml quick

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

RESOURCES += qml.qrc


