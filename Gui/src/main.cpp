#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <google/protobuf/stubs/common.h>
#include "app_config.h"
#include "hub_publisher.h"
#include <QMetaType>
#include "backend.h"
#include "video_item.h"

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    // Register types for cross-thread signal delivery  // ADD
    qRegisterMetaType<uint32_t>("uint32_t");            // ADD
    qRegisterMetaType<QByteArray>("QByteArray");        // ADD
    qRegisterMetaType<QList<GuiDetection>>("QList<GuiDetection>");  // ADD
    std::string configPath = "/etc/aeroboard/gui_config.json";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i+1 < argc) configPath = argv[++i];
    }

    GuiConfig cfg;
    cfg.load(configPath);
    cfg.dump();

    QGuiApplication app(argc, argv);
    qmlRegisterType<VideoItem>("Aeroboard", 1, 0, "VideoItem"); 
    qRegisterMetaType<QList<GuiDetection>>("QList<GuiDetection>");
    HubPublisher publisher(
        QString::fromStdString(cfg.pub_host), cfg.pub_port,
        QString::fromStdString(cfg.pub_host), cfg.sub_port);

    Backend backend(publisher, cfg);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    if (engine.rootObjects().isEmpty()) return -1;

    int ret = app.exec();
    google::protobuf::ShutdownProtobufLibrary();
    return ret;
}
