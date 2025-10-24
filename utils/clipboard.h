//
// Created by troplo on 10/24/25.
//

#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <QDBusMessage>
#include <QDBusConnection>

class QString;

namespace Flowshot
{
    class Clipboard {
    private:
        bool m_persist = false;
        bool m_hostingClipboard = false;
        bool m_clipboardSignalBlocked = false;

        void attachTextToClipboard(const QString& text,
                                   const QString& notification);
        static Clipboard* m_instance;
        static QDBusMessage createMethodCall(const QString& method);
        static void checkDBusConnection(const QDBusConnection& connection);
        static void call(const QDBusMessage& m);
        static QString ShowSaveFileDialog(const QString& title, const QString& directory);
        void attachScreenshotToClipboard(const QPixmap& pixmap);
    public:
        static void start();
        static Clipboard* instance();
        static void createPin(const QPixmap& capture, QRect geometry);
        static void copyToClipboard(const QPixmap& capture);
        static void copyToClipboard(const QString& text,
                                    const QString& notification = "");
        static bool saveToFilesystemGUI(const QPixmap& capture);
    };
}



#endif //CLIPBOARD_H
