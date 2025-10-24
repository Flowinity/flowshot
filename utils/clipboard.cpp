//
// Created by troplo on 10/24/25.
//

#include "clipboard.h"

#include <QClipboard>
#ifdef USE_WAYLAND_CLIPBOARD
#include <KF6/KGuiAddons/KSystemClipboard>
#endif
#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QStandardPaths>
#include <QMimeDatabase>
#include <QDialog>
#include <QFileDialog>
#include <QImageWriter>
#include <QMessageBox>

#include "abstractlogger.h"
#include "filenamehandler.h"
#include "../app/Application.h"
#include "desktopinfo.h"

class QClipboard;

namespace Flowshot
{
    void Clipboard::attachTextToClipboard(const QString& text,
                                                const QString& notification)
    {
        // Must send notification before clipboard modification on linux
        if (!notification.isEmpty()) {
            AbstractLogger::info() << notification;
        }

        m_hostingClipboard = true;

#ifdef USE_WAYLAND_CLIPBOARD
        auto* mimeData = new QMimeData();
        mimeData->setText(text);
        KSystemClipboard::instance()->setMimeData(mimeData, QClipboard::Clipboard);
#else
        QClipboard* clipboard = QApplication::clipboard();

        clipboard->blockSignals(true);
        // This variable is necessary because the signal doesn't get blocked on
        // windows for some reason
        m_clipboardSignalBlocked = true;
        clipboard->setText(text);
        clipboard->blockSignals(false);
#endif
    }

    void saveToClipboardMime(const QPixmap& capture, const QString& imageType)
    {
        QByteArray array;
        QBuffer buffer{ &array };
        QImageWriter imageWriter{ &buffer, imageType.toUpper().toUtf8() };
        if (imageType == "jpeg") {
            imageWriter.setQuality(ConfigHandler().jpegQuality());
        }
        imageWriter.write(capture.toImage());

        QPixmap formattedPixmap;
        bool isLoaded =
          formattedPixmap.loadFromData(reinterpret_cast<uchar*>(array.data()),
                                       array.size(),
                                       imageType.toUpper().toUtf8());
        if (isLoaded) {

            auto* mimeData = new QMimeData();

#ifdef USE_WAYLAND_CLIPBOARD
            mimeData->setImageData(formattedPixmap.toImage());
            mimeData->setData(QStringLiteral("x-kde-force-image-copy"),
                              QByteArray());
            KSystemClipboard::instance()->setMimeData(mimeData,
                                                      QClipboard::Clipboard);
#else
            mimeData->setData("image/" + imageType, array);
            QApplication::clipboard()->setMimeData(mimeData);
#endif

        } else {
            AbstractLogger::error()
              << QObject::tr("Error while saving to clipboard");
        }
    }

    void saveToClipboard(const QPixmap& capture)
    {
        // If we are able to properly save the file, save the file and copy to
        // clipboard.
            // Need to send message before copying to clipboard
#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
            if (DesktopInfo().waylandDetected()) {
                saveToClipboardMime(capture, "png");
            } else {
                QApplication::clipboard()->setPixmap(capture);
            }
#else
            QApplication::clipboard()->setPixmap(capture);
#endif
    }

    Clipboard* Clipboard::instance()
    {
        return m_instance;
    }

    void Clipboard::attachScreenshotToClipboard(const QPixmap& pixmap)
    {
        m_hostingClipboard = true;
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->blockSignals(true);
        // This variable is necessary because the signal doesn't get blocked on
        // windows for some reason
        m_clipboardSignalBlocked = true;
        saveToClipboard(pixmap);
        clipboard->blockSignals(false);
    }

    void Clipboard::copyToClipboard(const QPixmap& capture)
    {
        if (instance()) {
            instance()->attachScreenshotToClipboard(capture);
            return;
        }

        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);

#if defined(USE_KDSINGLEAPPLICATION) &&                                        \
(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
        auto kdsa = KDSingleApplication(QStringLiteral("com.flowinity.flowshot2"));
        stream << QStringLiteral("attachScreenshotToClipboard") << capture;
        kdsa.sendMessage(data);
#else
        stream << capture;
        QDBusMessage m =
          createMethodCall(QStringLiteral("attachScreenshotToClipboard"));

        m << data;
        call(m);
#endif
    }

    void Clipboard::copyToClipboard(const QString& text,
                                          const QString& notification)
    {
        if (instance()) {
            instance()->attachTextToClipboard(text, notification);
            return;
        }

#if defined(USE_KDSINGLEAPPLICATION) &&                                        \
(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
        auto kdsa = KDSingleApplication(QStringLiteral("com.flowinity.flowshot2"));
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << QStringLiteral("attachTextToClipboard") << text << notification;
        kdsa.sendMessage(data);
#else
        auto m = createMethodCall(QStringLiteral("attachTextToClipboard"));
        m << text << notification;
        call(m);
#endif
    }


#if !(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
    QDBusMessage Clipboard::createMethodCall(const QString& method)
    {
        QDBusMessage m =
          QDBusMessage::createMethodCall(QStringLiteral("com.flowinity.flowshot2"),
                                         QStringLiteral("/"),
                                         QLatin1String(""),
                                         method);
        return m;
    }

    void Clipboard::checkDBusConnection(const QDBusConnection& connection)
    {
        if (!connection.isConnected()) {
            AbstractLogger::error() << "Unable to connect via DBus";
            qApp->exit();
        }
    }

    void Clipboard::call(const QDBusMessage& m)
    {
        QDBusConnection sessionBus = QDBusConnection::sessionBus();
        checkDBusConnection(sessionBus);
        sessionBus.call(m);
    }
#endif

    void Clipboard::start()
    {
        if (!m_instance) {
            m_instance = new Clipboard();
        }
    }

    bool Clipboard::saveToFilesystemGUI(const QPixmap& capture)
{
    bool okay = false;
    ConfigHandler config;
    QString defaultSavePath = ConfigHandler().savePath();
    if (defaultSavePath.isEmpty() || !QDir(defaultSavePath).exists() ||
        !QFileInfo(defaultSavePath).isWritable()) {
        defaultSavePath =
          QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }
    QString savePath = FileNameHandler().properScreenshotPath(
      defaultSavePath, ConfigHandler().saveAsFileExtension());
#if defined(Q_OS_MACOS)
    for (QWidget* widget : qApp->topLevelWidgets()) {
        QString className(widget->metaObject()->className());
        if (0 ==
            className.compare(CaptureWidget::staticMetaObject.className())) {
            widget->showNormal();
            widget->hide();
            break;
        }
    }
#endif
    if (!config.savePathFixed()) {
        savePath = ShowSaveFileDialog(QObject::tr("Save screenshot"), savePath);
    }
    if (savePath == "") {
        return okay;
    }

    QFile file{ savePath };
    if (!file.open(QIODevice::WriteOnly))
    {
        AbstractLogger::error() << "Unable to open config file.";
    }

    QString saveExtension;
    saveExtension = QFileInfo(savePath).suffix().toLower();
    if (saveExtension == "jpg" || saveExtension == "jpeg") {
        okay = capture.save(&file, nullptr, ConfigHandler().jpegQuality());
    } else {
        okay = capture.save(&file);
    }

    if (okay) {
        // Don't use QDir::separator() here, as Qt internally always uses '/'
        QString pathNoFile = savePath.left(savePath.lastIndexOf('/'));

        ConfigHandler().setSavePath(pathNoFile);

        QString msg = QObject::tr("Capture saved as ") + savePath;
        AbstractLogger().attachNotificationPath(savePath) << msg;
    } else {
        QString msg = QObject::tr("Error trying to save as ") + savePath;

        if (file.error() != QFile::NoError) {
            msg += ": " + file.errorString();
        }

        QMessageBox saveErrBox(
          QMessageBox::Warning, QObject::tr("Save Error"), msg);
        saveErrBox.exec();
    }

    return okay;
}


QString Clipboard::ShowSaveFileDialog(const QString& title, const QString& directory)
{
    QFileDialog dialog(nullptr, title, directory);
    dialog.setAcceptMode(QFileDialog::AcceptSave);

    // Build string list of supported image formats
    QStringList mimeTypeList;
    for (const auto& mimeType : QImageWriter::supportedMimeTypes()) {
        // image/heif has several aliases and they cause glitch in save dialog
        // It is necessary to keep the image/heif (otherwise HEIF plug-in from
        // kimageformats will not work) but the aliases could be filtered out.
        if (mimeType != "image/heic" && mimeType != "image/heic-sequence" &&
            mimeType != "image/heif-sequence") {
            mimeTypeList.append(mimeType);
        }
    }
    dialog.setMimeTypeFilters(mimeTypeList);

    QString suffix = ConfigHandler().saveAsFileExtension();
    if (suffix.isEmpty()) {
        suffix = "png";
    }
    QString defaultMimeType =
      QMimeDatabase().mimeTypeForFile("image." + suffix).name();
    dialog.selectMimeTypeFilter(defaultMimeType);
    dialog.setDefaultSuffix(suffix);
    if (dialog.exec() == QDialog::Accepted) {
        return dialog.selectedFiles().constFirst();
    } else {
        return {};
    }
}
}

Clipboard* Clipboard::m_instance = nullptr;
