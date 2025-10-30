#include <QApplication>
#include <QWidget>

#include "app/Application.h"
#include "app/ScreenshotManager.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>

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

    for (const QVariant &arg : args) {
        msg << arg;
    }

    QDBusMessage reply = connection.call(msg);
    return reply.type() != QDBusMessage::ErrorMessage;
#else
    return false;
#endif
}

int main(int argc, char **argv) {
    // We need this hack because Wayland doesn't support "tool" overlay windows.
    // We don't want the environment variable just in case it affects Spectacle which we want in Wayland mode.
    bool platformSet = false;
    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]).startsWith("-platform")) {
            platformSet = true;
            break;
        }
    }

    static const char* platformArgs[] = {"-platform", "xcb"};
    if (!platformSet) {
        // Append -platform xcb to argv
        char** newArgv = new char*[argc + 2];
        for (int i = 0; i < argc; ++i) newArgv[i] = argv[i];
        newArgv[argc] = const_cast<char*>(platformArgs[0]);
        newArgv[argc+1] = const_cast<char*>(platformArgs[1]);
        argc += 2;
        argv = newArgv;
    }

    QApplication app(argc, argv);

    // Command-line parser
    QCommandLineParser parser;
    parser.addPositionalArgument("command", "gui or config");
    parser.process(app);
    QString command;
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) command = args.first();
    AbstractLogger::info() << command;

    Flowshot::Application flowshotApp;
    QApplication::setApplicationName("Flowshot2");
    QApplication::setOrganizationName("Flowinity");
    QApplication::setOrganizationDomain("flowinity.com");

    if (command == "gui") {
        if (sendFlowshotDbusCommand("captureScreen", {"1"})) {
            return 0;
        }
        flowshotApp.init(true);
        flowshotApp.takeScreenshot();

        QObject::connect(&flowshotApp, &Flowshot::Application::dialogClosed, &QCoreApplication::quit);

        return app.exec();
    }
    else if (command == "config") {
        ConfigEntry* settingsWindow = new ConfigEntry();
        settingsWindow->show();
        settingsWindow->raise();
        settingsWindow->activateWindow();
        return app.exec();
    } else if (!command.isNull()) {
        QString filePath = command;

        // Resolve to absolute path if it's a relative path
        QFileInfo fileInfo(filePath);
        if (!fileInfo.isAbsolute()) {
            filePath = QFileInfo(QDir::current(), filePath).absoluteFilePath();
        }

        // Ensure the file exists
        if (!QFile::exists(filePath)) {
            AbstractLogger::error() << "File does not exist:" << filePath;
            return 1;
        }

        // Attempt D-Bus upload
        if (sendFlowshotDbusCommand("uploadFile", {filePath})) {
            return 0;
        }

        // If D-Bus upload failed, proceed with screenshot flow
        flowshotApp.init(true);
        flowshotApp.takeScreenshot();

        QObject::connect(&flowshotApp, &Flowshot::Application::dialogClosed, &QCoreApplication::quit);
        return app.exec();
    }


    // Default GUI behavior
    flowshotApp.init(false);
    Flowshot::ScreenshotManager manager;
    return app.exec();
}