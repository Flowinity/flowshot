//
// Created by troplo on 10/24/25.
//

#include "ScreenshotManager.h"

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

                    if (QFile::exists(filePath))
                    {
                        QPixmap pixmap(filePath);
                        if (!pixmap.isNull())
                        {
                            ImgUploaderManager uploaderManager;
                            ImgUploaderBase* widget = ImgUploaderManager().uploader(pixmap);

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
                                    if (ConfigHandler().copyURLAfterUpload())
                                    {
                                        Clipboard::copyToClipboard(url.toString(), url.toString());
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
                                widget, &ImgUploaderBase::dialogClosed, [this, filePath](bool success)
                                {
                                    if (!filePath.data() || filePath == "") return;
                                    if (QFile::exists(filePath))
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
                        else
                        {
                            AbstractLogger::warning() << "Failed to load screenshot pixmap:" << filePath;
                        }
                    }
                    else
                    {
                        AbstractLogger::warning() << "Screenshot file does not exist:" << filePath;
                    }
                });

        process->start(program, QStringList() << "-ncrb" << "-o" << filePath);
    }
} // Flowshot
