#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <google/protobuf/stubs/common.h>
#include "app_config.h"
#include "hub_publisher.h"
#include "backend.h"

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::string configPath = "/media/JetsonNan/Peple_Flow/config/gui_config.json";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i+1 < argc) configPath = argv[++i];
    }

    GuiConfig cfg;
    cfg.load(configPath);
    cfg.dump();

    QGuiApplication app(argc, argv);

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