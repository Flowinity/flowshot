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
        AbstractLogger::error() << "Failed to open file: " << filePath;
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
        if (bytesTotal > 0) {
            emit uploadProgress(static_cast<int>((bytesSent * 100) / bytesTotal));
        }
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


void PrivateUploaderUploadV2::uploadToServer(QIODevice* headerDevice, QIODevice* fileDevice, QIODevice* footerDevice,
                                             qint64 totalSize, const QString& url, const QString& token, const QByteArray& boundary)
{
    AbstractLogger::info() << "uploadToServer called, totalSize: " << QString::number(totalSize);

    if (!headerDevice->open(QIODevice::ReadOnly))
        AbstractLogger::error() << "Failed to open header device";
    if (!fileDevice->isOpen() && !fileDevice->open(QIODevice::ReadOnly))
        AbstractLogger::error() << "Failed to open file device";
    if (!footerDevice->open(QIODevice::ReadOnly))
        AbstractLogger::error() << "Failed to open footer device";

    // Compose custom QIODevice to stream the three parts sequentially
    class MultipartStream : public QIODevice {
    public:
        MultipartStream(QIODevice* head, QIODevice* body, QIODevice* foot, QObject* parent = nullptr)
            : QIODevice(parent), header(head), file(body), footer(foot) {}

        bool isSequential() const override { return true; }
        bool open(OpenMode mode) override { setOpenMode(mode); return true; }
        void close() override { header->close(); file->close(); footer->close(); QIODevice::close(); }

    protected:
        qint64 readData(char* data, qint64 maxlen) override {
            qint64 bytes = 0;
            if (header->bytesAvailable() > 0)
                bytes = header->read(data, maxlen);
            else if (file->bytesAvailable() > 0)
                bytes = file->read(data, maxlen);
            else
                bytes = footer->read(data, maxlen);
            return bytes;
        }

        qint64 writeData(const char*, qint64) override { return -1; }

    private:
        QIODevice* header;
        QIODevice* file;
        QIODevice* footer;
    };

    MultipartStream* stream = new MultipartStream(headerDevice, fileDevice, footerDevice, this);

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("Authorization", token.toUtf8());
    request.setRawHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    emit uploadProgress(0);

    AbstractLogger::info() << "Starting POST request";
    m_currentReply = m_NetworkAM->post(request, stream);
    stream->setParent(m_currentReply);

    connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        AbstractLogger::info() << "QNetworkReply::finished signal received";
        QNetworkReply* reply = m_currentReply;
        m_currentReply = nullptr;

        if (reply->error() == QNetworkReply::NoError) {
            AbstractLogger::info() << "Upload completed.";
            handleReply(reply);
        } else {
            AbstractLogger::error() << "Upload failed: " << reply->errorString();
            EndpointsJSON* endpoints = new EndpointsJSON(this);
            endpoints->getAPIFromEndpoints(true);
            emit uploadError(reply);
        }

        reply->deleteLater();
    });

    connect(m_currentReply, &QNetworkReply::uploadProgress, this,
        [this](qint64 bytesSent, qint64 bytesTotal) {
            AbstractLogger::info() << "Upload progress: " << QString::number(bytesSent) << "/" << QString::number(bytesTotal);
            if (bytesTotal == 0) return;
            int progress = static_cast<int>((bytesSent * 100) / bytesTotal);
            emit uploadProgress(progress);
        });

    connect(m_currentReply, &QNetworkReply::errorOccurred, this,
        [this](QNetworkReply::NetworkError code) {
            AbstractLogger::error() << "Network error occurred: " << QString::number(code);
        });

    QTimer* statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, [this, statusTimer]() {
        if (m_currentReply) {
            AbstractLogger::info() << "Reply active - running:" << (m_currentReply->isRunning() ? "true" : "false")
                                   << ", error:" << QString::number(m_currentReply->error())
                                   << ", bytesAvailable:" << QString::number(m_currentReply->bytesAvailable());
        } else {
            AbstractLogger::info() << "Reply null, stopping timer";
            statusTimer->stop();
            statusTimer->deleteLater();
        }
    });
    statusTimer->start(1000);

    request.setTransferTimeout(10000);

    AbstractLogger::info() << "All connections established";
}

StreamingUploadDevice::StreamingUploadDevice(QFile* file, const QByteArray& header, const QByteArray& footer, QObject* parent)
  : QIODevice(parent)
  , m_file(file)
  , m_header(header)
  , m_footer(footer)
  , m_headerPos(0)
  , m_filePos(0)
  , m_footerPos(0)
  , m_headerSent(false)
  , m_fileSent(false)
{
    m_file->setParent(this);
    m_totalSize = m_header.size() + m_file->size() + m_footer.size();
}

StreamingUploadDevice::~StreamingUploadDevice()
{
    if (m_file && m_file->isOpen()) {
        m_file->close();
    }
}

bool StreamingUploadDevice::open(OpenMode mode)
{
    if (!(mode & QIODevice::ReadOnly)) {
        return false;
    }

    if (m_file && !m_file->isOpen()) {
        if (!m_file->open(QIODevice::ReadOnly)) {
            return false;
        }
    }

    m_headerPos = 0;
    m_filePos = 0;
    m_footerPos = 0;
    m_headerSent = false;
    m_fileSent = false;

    return QIODevice::open(mode);
}

void StreamingUploadDevice::close()
{
    if (m_file && m_file->isOpen()) {
        m_file->close();
    }
    QIODevice::close();
}

qint64 StreamingUploadDevice::size() const
{
    return m_totalSize;
}

qint64 StreamingUploadDevice::bytesAvailable() const
{
    qint64 available = 0;

    // Always report remaining bytes from all parts
    available += m_header.size() - m_headerPos;

    if (m_file) {
        available += m_file->size() - m_filePos;
    }

    available += m_footer.size() - m_footerPos;

    return available + QIODevice::bytesAvailable();
}

bool StreamingUploadDevice::isSequential() const
{
    return true;
}

qint64 StreamingUploadDevice::readData(char* data, qint64 maxSize)
{
    AbstractLogger::info() << "readData called, maxSize: " << QString::number(maxSize);

    qint64 totalRead = 0;

    if (!m_headerSent && m_headerPos < m_header.size()) {
        qint64 toRead = qMin(maxSize, static_cast<qint64>(m_header.size() - m_headerPos));
        memcpy(data, m_header.constData() + m_headerPos, toRead);
        m_headerPos += toRead;
        totalRead += toRead;
        maxSize -= toRead;
        data += toRead;

        if (m_headerPos >= m_header.size()) {
            m_headerSent = true;
        }
        AbstractLogger::info() << "Read header: " << QString::number(toRead) << " bytes";
    }

    if (m_headerSent && !m_fileSent && maxSize > 0 && m_file) {
        qint64 fileRead = m_file->read(data, maxSize);
        AbstractLogger::info() << "Read from file: " << QString::number(fileRead) << " bytes";
        if (fileRead > 0) {
            m_filePos += fileRead;
            totalRead += fileRead;
            maxSize -= fileRead;
            data += fileRead;
        }

        if (m_filePos >= m_file->size() || fileRead <= 0) {
            m_fileSent = true;
            AbstractLogger::info() << "File reading complete";
        }
    }

    if (m_fileSent && maxSize > 0 && m_footerPos < m_footer.size()) {
        qint64 toRead = qMin(maxSize, static_cast<qint64>(m_footer.size() - m_footerPos));
        memcpy(data, m_footer.constData() + m_footerPos, toRead);
        m_footerPos += toRead;
        totalRead += toRead;
        AbstractLogger::info() << "Read footer: " << QString::number(toRead) << " bytes";
    }

    AbstractLogger::info() << "readData returning: " << QString::number(totalRead) << " bytes";
    return totalRead;
}

qint64 StreamingUploadDevice::writeData(const char* data, qint64 maxSize)
{
    Q_UNUSED(data)
    Q_UNUSED(maxSize)
    return -1;
}

bool StreamingUploadDevice::atEnd() const
{
    bool isAtEnd = m_headerSent && m_fileSent && m_footerPos >= m_footer.size();
    return isAtEnd && QIODevice::atEnd();
}