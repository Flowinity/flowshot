// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include <QtDBus/QDBusAbstractAdaptor>
#include <QObject>

class FlowshotDbusAdapter : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.flowinity.flowshot2")

public:
    explicit FlowshotDbusAdapter(QObject* parent = nullptr);
    ~FlowshotDbusAdapter() override;

public slots:
    Q_NOREPLY void captureScreen(const QString& captureMode);
    Q_NOREPLY void uploadFile(const QString& path);
    Q_NOREPLY void checkIfRunning();
    // Q_NOREPLY void compressAndUploadFolder(const QString& path);
};
