// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2025 Troplo & Contributors

#ifndef PRIVATEUPLOADERUPLOAD_H
#define PRIVATEUPLOADERUPLOAD_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>

#include "responses/FlowinityValidUploadResponse.h"

class StreamingUploadDevice;

class PrivateUploaderUploadV2 : public QObject
{
    Q_OBJECT

public:
    PrivateUploaderUploadV2(QObject* parent = nullptr);
    ~PrivateUploaderUploadV2();

    void uploadBytes(const QByteArray& byteArray, const QString& fileName, const QString& fileType);
    void uploadFile(const QString& filePath, const QString& fileName, const QString& fileType);
    void handleReply(QNetworkReply* reply);
    void cancelUpload();

    signals:
        void uploadProgress(int progress, double speed);
        void uploadOk(FlowinityValidUploadResponse response);
        void uploadError(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_NetworkAM;
    QNetworkReply* m_currentReply;
    QString m_filePath;
    qint64 m_lastBytesSent = 0;
    QElapsedTimer m_lastTime;
};

#endif // PRIVATEUPLOADERUPLOAD_H
