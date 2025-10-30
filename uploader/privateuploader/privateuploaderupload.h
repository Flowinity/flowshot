// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include "../imguploaderbase.h"
#include "../../utils/filenamehandler.h"
#include <QUrl>
#include <QWidget>

class QNetworkReply;
class QNetworkAccessManager;
class QUrl;

class PrivateUploaderUpload : public QObject
{
    Q_OBJECT

public:
    PrivateUploaderUpload(QObject* parent = nullptr);

public slots:
    void uploadBytes(const QByteArray& byteArray,
                     const QString& fileName,
                     const QString& fileType);
    void uploadToServer(const QByteArray& postData,
                        const QString& url,
                        const QString& token,
                        const QByteArray& boundary);
    void uploadFile(const QString& filePath,
                    const QString& fileName,
                    const QString& fileType,
                    const QByteArray& boundary);

signals:
    void uploadOk(QNetworkReply* reply);
    void uploadError(QNetworkReply* error);
    void uploadProgress(int progress, double speed);

private:
    QNetworkAccessManager* m_NetworkAM;
};