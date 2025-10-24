// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include <QSettings>
#include <QStringList>
#include <QVariant>
#include <QVector>

#define CONFIG_GROUP_GENERAL "General"
#define CONFIG_GROUP_SHORTCUTS "Shortcuts"

class QFileSystemWatcher;
class ValueHandler;
template<class T>
class QSharedPointer;
class QTextStream;
class AbstractLogger;

/**
 * Declare and implement a getter for a config option. `KEY` is the option key
 * as it appears in the config file, `TYPE` is the C++ type. At the same time
 * `KEY` is the name of the generated getter function.
 */
// clang-format off
#define CONFIG_GETTER(KEY, TYPE)                                               \
    TYPE KEY()                                                                 \
    {                                                                          \
        return value(QStringLiteral(#KEY)).value<TYPE>();                      \
    }
// clang-format on

/**
 * Declare and implement a setter for a config option. `FUNC` is the name of the
 * generated function, `KEY` is the option key as it appears in the config file
 * and `TYPE` is the C++ type.
 */
#define CONFIG_SETTER(FUNC, KEY, TYPE)                                         \
    void FUNC(const TYPE& val)                                                 \
    {                                                                          \
        QString key = QStringLiteral(#KEY);                                    \
        /* Without this check, multiple `flameshot gui` instances running */   \
        /* simultaneously would cause an endless loop of fileWatcher calls */  \
        if (QVariant::fromValue(val) != value(key)) {                          \
            setValue(key, QVariant::fromValue(val));                           \
        }                                                                      \
    }

/**
 * Combines the functionality of `CONFIG_GETTER` and `CONFIG_SETTER`. `GETFUNC`
 * is simultaneously the name of the getter function and the option key as it
 * appears in the config file. `SETFUNC` is the name of the setter function.
 * `TYPE` is the C++ type of the value.
 */
#define CONFIG_GETTER_SETTER(GETFUNC, SETFUNC, TYPE)                           \
    CONFIG_GETTER(GETFUNC, TYPE)                                               \
    CONFIG_SETTER(SETFUNC, GETFUNC, TYPE)

class ConfigHandler : public QObject
{
    Q_OBJECT

public:
    explicit ConfigHandler();

    static ConfigHandler* getInstance();

    // Definitions of getters and setters for config options
    // Some special cases are implemented regularly, without the macro
    // NOTE: When adding new options, make sure to add an entry in
    // recognizedGeneralOptions in the cpp file.
    CONFIG_GETTER_SETTER(uploadClientSecret, setUploadClientSecret, QString)
    CONFIG_GETTER_SETTER(uploadTokenTPU, setUploadTokenTPU, QString)
    CONFIG_GETTER_SETTER(serverTPU, setServerTPU, QString)
    CONFIG_GETTER_SETTER(uploadWindowTimeout, setUploadWindowTimeout, int)
    CONFIG_GETTER_SETTER(uploadWindowOffsetY, setUploadWindowOffsetY, int)
    CONFIG_GETTER_SETTER(uploadWindowOffsetX, setUploadWindowOffsetX, int)
    CONFIG_GETTER_SETTER(uploadWindowScaleWidth, setUploadWindowScaleWidth, int)
    CONFIG_GETTER_SETTER(uploadWindowScaleHeight,
                             setUploadWindowScaleHeight,
                         int)
    CONFIG_GETTER_SETTER(uploadWindowImageWidth,
                         setUploadWindowImageWidth,
                         int)
    CONFIG_GETTER_SETTER(uploadWindowStackPadding,
                         setUploadWindowStackPadding,
                         int)
    CONFIG_GETTER_SETTER(uploadWindowEnabled, setUploadWindowEnabled, bool)
    CONFIG_GETTER_SETTER(uploadWindowDisplay, setUploadWindowDisplay, int)
    CONFIG_GETTER_SETTER(uploadWindowImageEnabled,
                         setUploadWindowImageEnabled,
                         bool)
    CONFIG_GETTER_SETTER(uploadWindowButtonsEnabled,
                         setUploadWindowButtonsEnabled,
                         bool)
    CONFIG_GETTER_SETTER(saveLastRegion, setSaveLastRegion, bool)
    CONFIG_GETTER_SETTER(showSelectionGeometry, setShowSelectionGeometry, int)
    CONFIG_GETTER_SETTER(jpegQuality, setJpegQuality, int)
    CONFIG_GETTER_SETTER(reverseArrow, setReverseArrow, bool)
    CONFIG_GETTER_SETTER(insecurePixelate, setInsecurePixelate, bool)
    CONFIG_GETTER_SETTER(showSelectionGeometryHideTime,
                         showSelectionGeometryHideTime,
                         int)
    CONFIG_GETTER_SETTER(platform, setPlatform, QString)
    // Flowinity Endpoints update (2024)
    CONFIG_GETTER_SETTER(serverEndpoints, setServerEndpoints, QString)
    CONFIG_GETTER_SETTER(serverAPIEndpoint, setServerAPIEndpoint, QString)
    CONFIG_GETTER_SETTER(serverSupportsEndpoints,
                         setServerSupportsEndpoints,
                         bool)

    CONFIG_GETTER_SETTER(screenshotUtility, setScreenshotUtility, int)
    CONFIG_GETTER_SETTER(copyURLAfterUpload, setCopyURLAfterUpload, bool)
    CONFIG_GETTER_SETTER(savePath, setSavePath, QString)
    CONFIG_GETTER_SETTER(savePathFixed, setSavePathFixed, bool)
    CONFIG_GETTER_SETTER(saveAsFileExtension, setSaveAsFileExtension, QString)

    // DEFAULTS
    QString filenamePatternDefault();
    void setDefaultSettings();
    QString configFilePath() const;

    // GENERIC GETTERS AND SETTERS
    bool setShortcut(const QString& actionName, const QString& shortcut);
    QString shortcut(const QString& actionName);
    void setValue(const QString& key, const QVariant& value);
    QVariant value(const QString& key) const;
    void remove(const QString& key);
    void resetValue(const QString& key);

    // INFO
    static QSet<QString>& recognizedGeneralOptions();
    static QSet<QString>& recognizedShortcutNames();
    QSet<QString> keysFromGroup(const QString& group) const;

    // ERROR HANDLING
    bool checkForErrors(AbstractLogger* log = nullptr) const;
    bool checkUnrecognizedSettings(AbstractLogger* log = nullptr,
                                   QList<QString>* offenders = nullptr) const;
    bool checkShortcutConflicts(AbstractLogger* log = nullptr) const;
    bool checkSemantics(AbstractLogger* log = nullptr,
                        QList<QString>* offenders = nullptr) const;
    void checkAndHandleError() const;
    void setErrorState(bool error) const;
    bool hasError() const;
    QString errorMessage() const;

    // Import from Flameshot to Flowshot2
    void checkImport();


signals:
    void error() const;
    void errorResolved() const;
    void fileChanged() const;

private:
    mutable QSettings m_settings;

    static bool m_hasError, m_errorCheckPending, m_skipNextErrorCheck;
    static QSharedPointer<QFileSystemWatcher> m_configWatcher;

    void ensureFileWatched() const;
    QSharedPointer<ValueHandler> valueHandler(const QString& key) const;
    void assertKeyRecognized(const QString& key) const;
    bool isShortcut(const QString& key) const;
    QString baseName(const QString& key) const;
    void cleanUnusedKeys(const QString& group, const QSet<QString>& keys) const;
};
