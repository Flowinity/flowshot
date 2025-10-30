// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2023 Troplo & Contributors

#include "PrivateUploaderUploadV2.h"
#include "../../utils/flowinity/EndpointsJSON.h"
#include "../../utils/abstractlogger.h"
#include "../../utils/rng.h"
#include "../../utils/ConfigHandler.h"
#include <QBuffer>
#include <QFile>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QtGlobal>
#include <QTimer>

#include "responses/FlowinityValidUploadResponse.h"

PrivateUploaderUploadV2::PrivateUploaderUploadV2(QObject* parent)
  : QObject(parent)
  , m_NetworkAM(new QNetworkAccessManager(this))
  , m_currentReply(nullptr)
{}

PrivateUploaderUploadV2::~PrivateUploaderUploadV2()
{
    cancelUpload();
}

void PrivateUploaderUploadV2::cancelUpload()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void PrivateUploaderUploadV2::uploadBytes(const QByteArray& byteArray, const QString& fileName, const QString& fileType)
{
    QByteArray boundary = ("BoUnDaRy-" + Flowshot::randomString(16)).toUtf8();
    QByteArray postData;

    postData.append("--" + boundary + "\r\n");
    postData.append(R"(Content-Disposition: form-data; name="attachment"; filename=")" + fileName.toUtf8() + "\"\r\n");
    postData.append("Content-Type: " + fileType.toUtf8() + "\r\n");
    postData.append("\r\n");
    postData.append(byteArray);
    postData.append("\r\n");
    postData.append("--" + boundary + "--\r\n");

    QBuffer* buffer = new QBuffer();
    buffer->setData(postData);

    QString url = QStringLiteral("%1/gallery").arg(ConfigHandler().serverAPIEndpoint());
    QString token = QStringLiteral("%1").arg(ConfigHandler().uploadTokenTPU());

    // uploadToServer(buffer, TODO, TODO, postData.size(), url, token, boundary);
}

void PrivateUploaderUploadV2::uploadFile(const QString& filePath, const QString& fileName, const QString& fileType)
{
    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        return;
    }

    m_filePath = filePath;

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"attachment\"; filename=\"" + fileName + "\""));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(fileType));
    filePart.setBodyDevice(file);
    file->setParent(multiPart);  // multiPart will delete the file
    multiPart->append(filePart);

    QString url = QStringLiteral("%1/gallery").arg(ConfigHandler().serverAPIEndpoint());
    QString token = QStringLiteral("%1").arg(ConfigHandler().uploadTokenTPU());

    QNetworkRequest request{ QUrl(url) };
    request.setRawHeader("Authorization", token.toUtf8());

    m_currentReply = m_NetworkAM->post(request, multiPart);
    multiPart->setParent(m_currentReply);  // reply deletes the multiPart

    connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply* reply = m_currentReply;
        m_currentReply = nullptr;
        if (reply->error() == QNetworkReply::NoError) {
            handleReply(reply);
        } else {
            emit uploadError(reply);
        }
        reply->deleteLater();
    });

    connect(m_currentReply, &QNetworkReply::uploadProgress, this,
        [this](qint64 bytesSent, qint64 bytesTotal) {
            if (bytesTotal == 0) return;

             // Calculate progress percentage
             int progress = static_cast<int>((bytesSent * 100) / bytesTotal);

             qint64 deltaBytes = bytesSent - m_lastBytesSent;
             qint64 elapsedMs = m_lastTime.elapsed();
             if (elapsedMs > 0) {
                 double mbps = (deltaBytes * 8.0 / 1'000'000) / (elapsedMs / 1000.0);
                 emit uploadProgress(progress, mbps);
             } else
             {
                 emit uploadProgress(progress, 0);
             }

             m_lastBytesSent = bytesSent;
             m_lastTime.restart();
        });
}

void PrivateUploaderUploadV2::handleReply(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
        QJsonObject json = response.object();
        QString url = json[QStringLiteral("url")].toString();
        FlowinityValidUploadResponse flowinityResponse = FlowinityValidUploadResponse(url, m_filePath);
        emit uploadOk(flowinityResponse);
    } else {
        emit uploadError(reply);
    }
}
