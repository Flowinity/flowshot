//
// Created by troplo on 10/24/25.
//

#ifndef SCREENSHOTMANAGER_H
#define SCREENSHOTMANAGER_H
#include <qobject.h>
#include <QProcess>
#include "../utils/rng.h"
#include <QFile>
#include <QNetworkAccessManager>

#include "../uploader/imguploadermanager.h"
#include <qpixmap.h>

namespace Flowshot {
    enum class ScreenshotUtility {
        SPECTACLE,
        FLAMESHOT,
        LAST_VALUE
    };

    constexpr int ScreenshotUtilityMax = static_cast<int>(ScreenshotUtility::LAST_VALUE) - 1;

    class ScreenshotManager : public QObject {
        Q_OBJECT
    private:
        int m_openWindowCount = 0;
        bool m_isTakingScreenshot = false;
        QNetworkAccessManager* m_NetworkAM;

    public:
        explicit ScreenshotManager(QObject* parent = nullptr) : QObject(parent)
        {
            m_NetworkAM = new QNetworkAccessManager(this);
        }

        void takeScreenshot(ScreenshotUtility util);

    signals:
        void screenshotTaken(const QString &filePath);
        void screenshotUploaded(const QString &filePath);
        void dialogClosed();
    };


} // Flowshot

#endif //SCREENSHOTMANAGER_H
