#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include "backend.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    // Use OpenGL for QtGraphicalEffects (DropShadow etc.) on Jetson Nano
    QQuickWindow::setSceneGraphBackend("opengl");

    Backend backend;

    QQmlApplicationEngine engine;
    // Expose backend to all QML files as "backend"
    engine.rootContext()->setContextProperty("backend", &backend);

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
