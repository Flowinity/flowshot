#include <QApplication>
#include <QWidget>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QDBusConnection>
#include <QDBusMessage>
#include "app/Application.h"
#include "app/ScreenshotManager.h"
#include "app/pages/settings/configEntry.h"
#include "app/pages/settings/generalconf2.h"
#include "utils/abstractlogger.h"

bool sendFlowshotDbusCommand(QString method, const QVariantList &args = {}) {
#if !(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.isConnected()) return false;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        "com.flowinity.flowshot2", "/", "com.flowinity.flowshot2", method
    );

    for (const QVariant &arg : args) msg << arg;

    QDBusMessage reply = connection.call(msg);
    return reply.type() != QDBusMessage::ErrorMessage;
#else
    return false;
#endif
}

int main(int argc, char **argv) {
    bool platformSet = false;
    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]).startsWith("-platform")) {
            platformSet = true;
            break;
        }
    }

    static const char* platformArgs[] = {"-platform", "xcb"};
    if (!platformSet) {
        char** newArgv = new char*[argc + 2];
        for (int i = 0; i < argc; ++i) newArgv[i] = argv[i];
        newArgv[argc] = const_cast<char*>(platformArgs[0]);
        newArgv[argc+1] = const_cast<char*>(platformArgs[1]);
        argc += 2;
        argv = newArgv;
    }

    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Flowshot2 help");
    parser.addHelpOption();
    parser.addPositionalArgument("up", "up {file} - Upload a file to Flowinity.");
    parser.addPositionalArgument("config", "Open the Flowshot Configuration menu.");
    parser.addPositionalArgument("gui", "Quickly take a screenshot.");
    parser.addPositionalArgument("{file}", "Alias for up {file}. Upload a file to Flowinity.");
    parser.addPositionalArgument("[none]", "Run the Flowshot system tray service.");

    parser.process(app);
    const QStringList args = parser.positionalArguments();

    QApplication::setApplicationName("Flowshot2");
    QApplication::setOrganizationName("Flowinity");
    QApplication::setOrganizationDomain("flowinity.com");

    if (args.isEmpty()) {
        Flowshot::Application flowshotApp;
        flowshotApp.init(false);
        Flowshot::ScreenshotManager manager;
        return app.exec();
    }

    QString command = args.first();

    if (command == "gui") {
        Flowshot::Application flowshotApp;
        if (sendFlowshotDbusCommand("captureScreen", {"1"})) return 0;
        flowshotApp.init(true);
        flowshotApp.takeScreenshot();
        QObject::connect(&flowshotApp, &Flowshot::Application::dialogClosed, &QCoreApplication::quit);
        return app.exec();
    } else if (command == "config") {
        ConfigEntry* settingsWindow = new ConfigEntry();
        settingsWindow->show();
        settingsWindow->raise();
        settingsWindow->activateWindow();
        return app.exec();
    }

    QString filePath;
    if (command == "up" && args.size() > 1) {
        filePath = args.at(1);
    } else {
        filePath = command;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.isAbsolute()) filePath = QFileInfo(QDir::current(), filePath).absoluteFilePath();

    if (!QFile::exists(filePath)) {
        AbstractLogger::error() << "File does not exist:" << filePath;
        return 1;
    }

    if (sendFlowshotDbusCommand("uploadFile", {filePath})) return 0;

    Flowshot::Application flowshotApp;
    flowshotApp.init(true);
    flowshotApp.takeScreenshot();
    QObject::connect(&flowshotApp, &Flowshot::Application::dialogClosed, &QCoreApplication::quit);
    return app.exec();
}
