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
        void uploadProgress(int progress);
        void uploadOk(FlowinityValidUploadResponse response);
        void uploadError(QNetworkReply* reply);

private:
    void uploadToServer(QIODevice* headerDevice, QIODevice* fileDevice, QIODevice* footerDevice, qint64 totalSize, const QString& url, const
                        QString& token, const QByteArray& boundary);

    QNetworkAccessManager* m_NetworkAM;
    QNetworkReply* m_currentReply;
    QString m_filePath;
};

class StreamingUploadDevice : public QIODevice
{
    Q_OBJECT

public:
    StreamingUploadDevice(QFile* file, const QByteArray& header, const QByteArray& footer, QObject* parent = nullptr);
    ~StreamingUploadDevice();

    bool open(OpenMode mode) override;
    void close() override;
    qint64 size() const override;
    qint64 bytesAvailable() const override;
    bool isSequential() const override;
    bool atEnd() const override;

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;

private:
    QFile* m_file;
    QByteArray m_header;
    QByteArray m_footer;
    qint64 m_headerPos;
    qint64 m_filePos;
    qint64 m_footerPos;
    qint64 m_totalSize;
    bool m_headerSent;
    bool m_fileSent;
};

#endif // PRIVATEUPLOADERUPLOAD_H
