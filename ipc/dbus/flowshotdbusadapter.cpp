#include "flowshotdbusadapter.h"
#include "../../app/Application.h"  // or whatever header defines Application
#include <QDateTime>
#include "../../utils/abstractlogger.h"

FlowshotDbusAdapter::FlowshotDbusAdapter(QObject* parent)
    : QDBusAbstractAdaptor(parent)
{}

FlowshotDbusAdapter::~FlowshotDbusAdapter() = default;

void FlowshotDbusAdapter::captureScreen(const QString& captureMode)
{
#ifdef MEASURE_INIT_TIME
    qputenv("FLAMESHOT_INIT_TIME", QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
#endif
    AbstractLogger::info() << "Capture Screen requested";
    bool ok = false;
    int captureModeInt = captureMode.toInt(&ok);
    if (!ok || captureModeInt < 0 || captureModeInt > 3)
        return;

    if (auto app = qobject_cast<Flowshot::Application*>(parent())) {
        app->takeScreenshot();
    }
}

void FlowshotDbusAdapter::uploadFile(const QString& path)
{
#ifdef MEASURE_INIT_TIME
    qputenv("FLAMESHOT_INIT_TIME", QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
#endif
    if (auto app = qobject_cast<Flowshot::Application*>(parent())) {
        app->uploadFile(path);
    }
}

void FlowshotDbusAdapter::checkIfRunning() {
    //
}