// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2023 Troplo & Contributors

#include "privateuploader.h"
#include "privateuploaderupload.h"
#include "../../utils/ConfigHandler.h"
#include "../imguploaderbase.h"
#include <QBuffer>
#include <QDesktopServices>
#include <QEventLoop>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QShortcut>
#include <QUrlQuery>
#include <iostream>
#include <utility>
#include <QImageReader>

#include "PrivateUploaderUploadHandler.h"
#include "PrivateUploaderUploadV2.h"
#include "../../config/experiments.h"
#include "../../utils/abstractlogger.h"

class PrivateUploaderUploadV2;
using namespace Flowshot;

    PrivateUploader::PrivateUploader(const QPixmap& capture, QWidget* parent, bool fromScreenshotUtility)
      : ImgUploaderBase(capture, parent)
    {
        m_NetworkAM = new QNetworkAccessManager(this);
        m_fromScreenshotUtility = fromScreenshotUtility;
    }

    PrivateUploader::PrivateUploader(const QString& filePath, QWidget* parent, bool fromScreenshotUtility)
  : ImgUploaderBase(filePath, parent)
    {
        m_NetworkAM = new QNetworkAccessManager(this);
        m_fromScreenshotUtility = fromScreenshotUtility;
    }

    void PrivateUploader::handleReply(FlowinityValidUploadResponse response)
    {
        m_currentImageName = response.getUrl();
        int lastSlash = m_currentImageName.lastIndexOf("/");
        if (lastSlash >= 0) {
            m_currentImageName = m_currentImageName.mid(lastSlash + 1);
        }

        setImageURL(response.getUrl());
        setFilePath(response.getFilePath());
        if (m_fromScreenshotUtility && ConfigHandler().uploadWindowImageEnabled())
        {
            QImageReader reader(response.getFilePath());
            reader.setAutoTransform(true);
            QSize originalSize = reader.size();
            QSize scaledSize = originalSize;
            scaledSize.scale(QSize(512, 512), Qt::KeepAspectRatio);
            reader.setScaledSize(scaledSize);

            QImage img = reader.read();
            if (!img.isNull()) {
                QPixmap pixmap = QPixmap::fromImage(img);
                setPixmap(pixmap);
            }
        }
        emit uploadOk(response.getUrl());

        // Create shortcut on the main thread - use 'this' as parent to ensure proper thread affinity
        QShortcut* shortcut = new QShortcut(Qt::Key_Escape, this);
        connect(shortcut, &QShortcut::activated, this, &PrivateUploader::close);
    }

    void PrivateUploader::upload()
    {
        bool useByteArray = m_fromScreenshotUtility && !pixmap().isNull();
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        if (useByteArray)
        {
            pixmap().save(&buffer, "PNG");
        }

        // if (Experiments::FLOWSHOT2_USE_NEW_UPLOAD_BACKEND == 1)
        {
            PrivateUploaderUploadHandler* uploader = new PrivateUploaderUploadHandler(m_NetworkAM, nullptr);
            connect(uploader,
                    &PrivateUploaderUploadHandler::uploadOk,
                    [this, uploader](FlowinityValidUploadResponse response) {
                        handleReply(std::move(response));
                        uploader->deleteLater();
                    });
            const QString& fileName =
              FileNameHandler().parsedPattern().toLower().endsWith(".png")
                ? FileNameHandler().parsedPattern()
                : FileNameHandler().parsedPattern() + ".png";
            AbstractLogger::info() << filePath();
            if (!filePath().isNull()) {
                uploader->uploadFile(filePath(), fileName, "image/png");
            } else if (useByteArray)
            {
                uploader->uploadBytes(byteArray, fileName, "image/png");
            }
            buffer.close();
            byteArray.clear();

            connect(uploader,
                    &PrivateUploaderUploadHandler::uploadProgress,
                    this,
                    &PrivateUploader::updateProgress);
        }

        AbstractLogger::info() << "PrivateUploader::upload() completed";
    }

    void PrivateUploader::deleteImage(const QString& fileName,
                                      const QString& deleteToken)
    {
        Q_UNUSED(fileName)
        Q_UNUSED(deleteToken)
    }

/*
void PrivateUploader::deleteImage(const QString& fileName, const QString&
deleteToken)
{
    Q_UNUSED(fileName)

    QUrl url(QString("%1/api/v3/gallery/%2").arg(ConfigHandler().serverTPU(),
deleteToken)); std::cout << url.toString().toStdString() << std::endl;

    QNetworkRequest request(url);
    request.setRawHeader("Authorization",
QString("%1").arg(ConfigHandler().uploadTokenTPU()).toUtf8());

    QNetworkReply *reply = m_NetworkAM->deleteResource(request);

    // Use QEventLoop to wait for the reply to finish
    QEventLoop loop;

    // Connect the finished signal to a lambda function
    connect(reply, &QNetworkReply::finished, [&]() {
        // Ensure reply is not nullptr before accessing
        if (reply && reply->error() == QNetworkReply::NoError) {
            qDebug() << "Image deleted successfully";
        } else {
            // Check if reply is nullptr to avoid dereferencing a null pointer
            qDebug() << "Error deleting image:" << (reply ? reply->errorString()
: "Reply is nullptr");
        }

        // Delete the reply and exit the event loop
        reply->deleteLater();
        loop.quit();
    });

    // Start the event loop
    loop.exec();

    // Emit the signal after the event loop has finished
    emit deleteOk();
}*/