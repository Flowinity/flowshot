//
// Created by troplo on 10/24/25.
//

#ifndef APPLICATION_H
#define APPLICATION_H
#include <QApplication>
#include <qtmetamacros.h>
#include "ScreenshotManager.h"

namespace Flowshot {
    class ScreenshotManager;

    class Application : public QObject {
    Q_OBJECT
private:
    ScreenshotManager *m_screenshotManager = new ScreenshotManager(this);
public:
    Application(QObject* parent = nullptr) : QObject(parent) {}

    void init(bool noTray);
    void takeScreenshot() const;

    signals:
        void ready();
        void dialogClosed();
};

} // Flowshot

#endif //APPLICATION_H
