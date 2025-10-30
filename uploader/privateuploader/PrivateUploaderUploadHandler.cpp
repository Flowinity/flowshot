//
// Created by troplo on 10/30/25.
//

#include "PrivateUploaderUploadHandler.h"

#include <QTimer>

#include "../../utils/abstractlogger.h"

PrivateUploaderUploadHandler::PrivateUploaderUploadHandler(QNetworkAccessManager* networkAM, QObject* parent)
    : QObject(parent),
      m_thread(new QThread(this)),
      m_worker(new PrivateUploaderUploadV2())
{
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &PrivateUploaderUploadHandler::destroyed, m_thread, &QThread::quit);

    connect(m_worker, &PrivateUploaderUploadV2::uploadProgress, this, &PrivateUploaderUploadHandler::uploadProgress);

    connect(m_worker, &PrivateUploaderUploadV2::uploadOk, this, [this](FlowinityValidUploadResponse response)
    {
        emit uploadOk(std::move(response));

        // Quit the thread from the main thread
        m_thread->quit();
    }, Qt::QueuedConnection);

    connect(m_worker, &PrivateUploaderUploadV2::uploadError, this, &PrivateUploaderUploadHandler::uploadError, Qt::QueuedConnection);

    m_thread->start();
}

PrivateUploaderUploadHandler::~PrivateUploaderUploadHandler()
{
    AbstractLogger::info() << "PrivateUploaderUploadHandler destructor called";
    if (m_thread->isRunning()) {
        m_thread->quit();
        if (!m_thread->wait(3000)) {
            AbstractLogger::warning() << "Thread did not quit in time, terminating";
            m_thread->terminate();
            m_thread->wait();
        }
    }
}

void PrivateUploaderUploadHandler::uploadFile(const QString& filePath, const QString& fileName, const QString& fileType)
{
    QMetaObject::invokeMethod(m_worker, [=]() {
        m_worker->uploadFile(filePath, fileName, fileType);
    }, Qt::QueuedConnection);
}

void PrivateUploaderUploadHandler::uploadBytes(const QByteArray& data, const QString& fileName, const QString& fileType)
{
    QMetaObject::invokeMethod(m_worker, [=]() {
        m_worker->uploadBytes(data, fileName, fileType);
    }, Qt::QueuedConnection);
}

void PrivateUploaderUploadHandler::cancel()
{
    QMetaObject::invokeMethod(m_worker, [=]() {
        m_worker->cancelUpload();
    }, Qt::QueuedConnection);
}
