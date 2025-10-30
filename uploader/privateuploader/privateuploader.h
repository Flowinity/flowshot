// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include "../imguploaderbase.h"
#include <QUrl>
#include <QWidget>

#include "responses/FlowinityValidUploadResponse.h"

class QNetworkReply;
class QNetworkAccessManager;
class QUrl;

using namespace Flowshot;

class PrivateUploader : public ImgUploaderBase
{
    Q_OBJECT
public:
    explicit PrivateUploader(const QPixmap& capture, QWidget* parent = nullptr, bool fromScreenshotUtility = false);
    explicit PrivateUploader(const QString& filePath, QWidget* parent = nullptr, bool fromScreenshotUtility = false);

    void deleteImage(const QString& fileName, const QString& deleteToken);
    void uploadBytes(const QByteArray& bytes);

private slots:
    void handleReply(FlowinityValidUploadResponse reply);

private:
    QNetworkAccessManager* m_NetworkAM;
    bool m_fromScreenshotUtility;
    void upload();
};