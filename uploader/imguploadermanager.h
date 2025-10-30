// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Yurii Puchkov & Contributors
//

#ifndef FLAMESHOT_IMGUPLOADERMANAGER_H
#define FLAMESHOT_IMGUPLOADERMANAGER_H

#include "imguploaderbase.h"
#include <QObject>

#define IMG_UPLOADER_STORAGE_DEFAULT "privateuploader"

class QPixmap;
class QWidget;

using namespace Flowshot;

class ImgUploaderManager : public QObject
{
    Q_OBJECT
public:
    explicit ImgUploaderManager(QObject* parent = nullptr);

    ImgUploaderBase* uploader(const QPixmap& capture,
                              bool fromScreenshotUtility,
                              QWidget* parent = nullptr);
    ImgUploaderBase* uploader(const QString& path,
                              bool fromScreenshotUtility,
                              QWidget* parent = nullptr);
    const QString& url();
    const QString& uploaderPlugin();

signals:
    // void uploadFinished(ImgUploaderBase* uploader);

private:
    void init();

private:
    ImgUploaderBase* m_imgUploaderBase;
    QString m_urlString;
    QString m_imgUploaderPlugin;

};

#endif // FLAMESHOT_IMGUPLOADERMANAGER_H
