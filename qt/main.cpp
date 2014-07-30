#include <QDebug>
#include <QQuickView>
#include <QQmlEngine>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "camera_player.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
	qmlRegisterType<CameraPlayer>("CameraPlayer", 1, 0, "CameraPlayer");

    QQuickView viewer;
    viewer.setSource(QUrl("qrc:///main.qml"));
    viewer.show();

    return app.exec();
}
