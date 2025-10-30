#include "Application.h"
#include "ScreenshotManager.h"
#include <QSystemTrayIcon>
#include <QMenu>

#include "../ipc/dbus/flowshotdbusadapter.h"
#include "../utils/clipboard.h"
#include "../utils/ConfigHandler.h"
#include "../utils/abstractlogger.h"
#include "pages/settings/configEntry.h"
#include "pages/settings/generalconf2.h"

namespace Flowshot
{
    void Application::init(bool noTray)
    {
        ConfigHandler::getInstance()->checkImport();
        Clipboard::start();
        if (!noTray)
        {
            QSystemTrayIcon* tray = new QSystemTrayIcon(QIcon(":/icons/flowshot2.png"), this);
            QMenu* menu = new QMenu();
            menu->addAction("Take Screenshot", [this]()
            {
                takeScreenshot();
            });
            ConfigEntry* configEntry = new ConfigEntry();
            menu->addAction("Settings", [configEntry]()
            {
                configEntry->show();
                configEntry->raise();
                configEntry->activateWindow();
            });

            menu->addAction("Quit", this, &QCoreApplication::quit);

            tray->setContextMenu(menu);
            tray->show();
            connect(tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason)
            {
                if (reason == QSystemTrayIcon::Trigger)
                {
                    takeScreenshot();
                }
            });


#if !(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
            new FlowshotDbusAdapter(this);
            QDBusConnection connection = QDBusConnection::sessionBus();
            if (!connection.registerService("com.flowinity.flowshot2"))
            {
                AbstractLogger::error() << "Error registering DBus service.";
            }
            if (!connection.registerObject("/", this))
            {
                AbstractLogger::error() << "Error registering DBus object.";
            }
#endif
        }


        connect(m_screenshotManager, &Flowshot::ScreenshotManager::dialogClosed,
                this, [this]() {
                    emit dialogClosed();
                });

        emit ready();
    }

    void Application::takeScreenshot() const
    {
        m_screenshotManager->takeScreenshot(ScreenshotUtility::SPECTACLE);
    }

    void Application::uploadFile(QString path) const
    {

    }
}
