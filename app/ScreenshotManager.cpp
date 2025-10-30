//
// Created by troplo on 10/24/25.
//

#include "ScreenshotManager.h"

#include <QNetworkAccessManager>
#include <QTimer>

#include "Application.h"
#include "../utils/clipboard.h"
#include "../utils/abstractlogger.h"

namespace Flowshot
{
    void ScreenshotManager::takeScreenshot(ScreenshotUtility util)
    {
        if (m_isTakingScreenshot) return;
        m_isTakingScreenshot = true;

        QString program;
        switch (util)
        {
        case ScreenshotUtility::SPECTACLE: program = "spectacle";
            break;
        case ScreenshotUtility::FLAMESHOT: program = "flameshot";
            break;
        }

        QString filePath = randomFilePath();

        QProcess* process = new QProcess(this);

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, filePath, process](int, QProcess::ExitStatus)
                {
                    m_isTakingScreenshot = false;
                    process->deleteLater();
                    uploadFile(filePath, true);
                });

        process->start(program, QStringList() << "-ncrb" << "-o" << filePath);
    }

    void ScreenshotManager::uploadFile(const QString& filePath, bool fromScreenshotUtility = false)
    {
        if (QFile::exists(filePath))
        {
            // QPixmap pixmap(filePath);
            // if (!pixmap.isNull())
            {
                ImgUploaderManager* uploaderManager = new ImgUploaderManager(m_NetworkAM);
                ImgUploaderBase* widget = uploaderManager->uploader(filePath, fromScreenshotUtility);

                m_openWindowCount++;

                QObject::connect(
                    widget, &QObject::destroyed, [this]() { m_openWindowCount--; });

                if (ConfigHandler().uploadWindowEnabled())
                {
                    widget->show();
                }

                // NOTE: lambda can't capture 'this' because it might be destroyed later
                widget->showPreUploadDialog(m_openWindowCount);
                QObject::connect(
                    widget, &ImgUploaderBase::uploadOk, [=, this](const QUrl& url)
                    {
                        AbstractLogger::info() << "URL" << url.toString();
                        if (ConfigHandler().copyURLAfterUpload())
                        {
                            // I dunno why this works, because shouldn't it be on the main thread already
                            QObject* receiver = qApp;
                            QMetaObject::invokeMethod(receiver, [url]() {
                                if (ConfigHandler().copyURLAfterUpload()) {
                                    Clipboard::copyToClipboard(url.toString(), url.toString());
                                }
                            }, Qt::QueuedConnection);
                            widget->showPostUploadDialog(m_openWindowCount);

                            // Disconnect all signals after upload completes
                            disconnect(widget, &ImgUploaderBase::uploadProgress, nullptr, nullptr);
                            disconnect(widget, &ImgUploaderBase::uploadError, nullptr, nullptr);
                        }
                    });
                QObject::connect(
                    widget, &ImgUploaderBase::uploadProgress, [=](int progress)
                    {
                        widget->updateProgress(progress);
                    });

                QObject::connect(
                    widget, &ImgUploaderBase::uploadError, [=](QNetworkReply* error)
                    {
                        widget->showErrorUploadDialog(error);
                    });

                QObject::connect(
                    widget, &ImgUploaderBase::dialogClosed, [this, filePath, fromScreenshotUtility](bool success)
                    {
                        if (!filePath.data() || filePath == "") return;
                        // Do not delete files that aren't in the /tmp folder from the screenshot utility
                        if (QFile::exists(filePath) && fromScreenshotUtility)
                        {
                            if (QFile::remove(filePath))
                            {
                                AbstractLogger::info() << "File deleted at: " << filePath;
                            }
                            else
                            {
                                AbstractLogger::warning() << "File failed to delete: " << filePath;
                            }
                        }
                        else
                        {
                            AbstractLogger::warning() << "File doesn't exist: " << filePath;
                        }

                        emit dialogClosed();
                    });


                emit screenshotUploaded(filePath);
            }
            // {
            // AbstractLogger::warning() << "Failed to load screenshot pixmap:" << filePath;
            // }
        }
        else
        {
            AbstractLogger::warning() << "Screenshot file does not exist:" << filePath;
        }
    }
} // Flowshot
