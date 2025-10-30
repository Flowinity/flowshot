//
// Created by troplo on 10/30/25.
//

#ifndef PRIVATEUPLOADERUPLOADHANDLER_H
#define PRIVATEUPLOADERUPLOADHANDLER_H
#include <QObject>
#include <QThread>
#include "PrivateUploaderUploadV2.h"

class PrivateUploaderUploadHandler : public QObject
{
    Q_OBJECT

public:
    explicit PrivateUploaderUploadHandler(QNetworkAccessManager* networkAM, QObject* parent = nullptr);
    ~PrivateUploaderUploadHandler();

public slots:
    void uploadFile(const QString& filePath, const QString& fileName, const QString& fileType);
    void uploadBytes(const QByteArray& data, const QString& fileName, const QString& fileType);
    void cancel();

    signals:
        void uploadProgress(int progress, double speed);
    void uploadOk(FlowinityValidUploadResponse response);
    void uploadError(QNetworkReply* reply);

private:
    QThread* m_thread;
    PrivateUploaderUploadV2* m_worker;
};

#endif //PRIVATEUPLOADERUPLOADHANDLER_H
