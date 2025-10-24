// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "ConfigHandler.h"
#include "ValueHandler.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QKeySequence>
#include <QMap>
#include <QSharedPointer>
#include <QStandardPaths>
#include <QVector>
#include <algorithm>
#include <stdexcept>

#include "abstractlogger.h"
#include "../app/ScreenshotManager.h"

#if defined(Q_OS_MACOS)
#include <QProcess>
#endif

// HELPER FUNCTIONS

bool verifyLaunchFile()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
    QString path = QStandardPaths::locate(QStandardPaths::GenericConfigLocation,
                                          "autostart/",
                                          QStandardPaths::LocateDirectory) +
                   "Flowshot2.desktop";
    bool res = QFile(path).exists();
#elif defined(Q_OS_WIN)
    QSettings bootUpSettings(
      "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
      QSettings::NativeFormat);
    bool res =
      bootUpSettings.value("Flowshot").toString() ==
      QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
#endif
    return res;
}

// VALUE HANDLING

/**
 * Use this to declare a setting with a type that is either unrecognized by
 * QVariant or if you need to place additional constraints on its value.
 * @param KEY Name of the setting as in the config file
 *            (a C-style string literal)
 * @param TYPE An instance of a `ValueHandler` derivative. This must be
 *             specified in the form of a constructor, or the macro will
 *             misbehave.
 */
#define OPTION(KEY, TYPE)                                                      \
    {                                                                          \
        QStringLiteral(KEY), QSharedPointer<ValueHandler>(new TYPE)            \
    }

#define SHORTCUT(NAME, DEFAULT_VALUE)                                          \
    {                                                                          \
        QStringLiteral(NAME), QSharedPointer<KeySequence>(new KeySequence(     \
                                QKeySequence(QLatin1String(DEFAULT_VALUE))))   \
    }

/**
 * This map contains all the information that is needed to parse, verify and
 * preprocess each configuration option in the General section.
 * NOTE: Please keep it well structured
 */
// clang-format off
static QMap<class QString, QSharedPointer<ValueHandler>>
        recognizedGeneralOptions = {
//         KEY                            TYPE                 DEFAULT_VALUE
    OPTION("uploadClientSecret"          ,String             ( ""            )),
    OPTION("uploadTokenTPU"              ,String             ( "")),
    OPTION("serverTPU"                   ,String             ( "https://flowinity.com")),
    OPTION("uploadWindowTimeout"         ,LowerBoundedInt (0, 25000)),
    OPTION("uploadWindowOffsetY"         ,LowerBoundedInt (-99999, 100)),
    OPTION("uploadWindowOffsetX"         ,LowerBoundedInt (-99999, 10)),
    OPTION("uploadWindowStackPadding"    ,LowerBoundedInt (0, 25)),
    OPTION("uploadWindowEnabled"         ,Bool               ( true          )),
    OPTION("uploadWindowScaleWidth"      ,LowerBoundedInt(0, 580)),
    OPTION("uploadWindowScaleHeight"     ,LowerBoundedInt(0, 120)),
    OPTION("uploadWindowImageWidth"      ,LowerBoundedInt(0, 125)),
    OPTION("uploadWindowDisplay"         ,LowerBoundedInt             ( -1, -1     )),
    OPTION("uploadWindowImageEnabled", Bool               ( true          )),
    OPTION("uploadWindowButtonsEnabled", Bool               ( true          )),
    OPTION("uploadWindowPreviewWidth"    ,LowerBoundedInt(0, 125)),
    // Endpoints
    OPTION("serverEndpoints", String("https://flowinity.com/endpoints.json")),
    OPTION("serverAPIEndpoint", String("https://api.flowinity.com/v3")),
    OPTION("serverSupportsEndpoints", Bool(true)),
    OPTION("screenshotUtility", BoundedInt(0, ScreenshotUtilityMax, static_cast<int>(ScreenshotUtility::SPECTACLE))),
    OPTION("copyURLAfterUpload"          ,Bool               ( true          )),
    OPTION("savePath"                    ,ExistingDir        (               )),
    OPTION("savePathFixed"               ,Bool               ( false         )),
    OPTION("saveAsFileExtension"         ,SaveFileExtension  (               )),
};

// clang-format on

// CLASS CONFIGHANDLER

ConfigHandler::ConfigHandler()
  : m_settings(QSettings::IniFormat,
               QSettings::UserScope,
               qApp->organizationName(),
               qApp->applicationName())
{
    static bool firstInitialization = true;
    if (firstInitialization) {
        // check for error every time the file changes
        m_configWatcher.reset(new QFileSystemWatcher());
        ensureFileWatched();
        QObject::connect(m_configWatcher.data(),
                         &QFileSystemWatcher::fileChanged,
                         [](const QString& fileName) {
                             emit getInstance()->fileChanged();

                             if (QFile(fileName).exists()) {
                                 m_configWatcher->addPath(fileName);
                             }
                             if (m_skipNextErrorCheck) {
                                 m_skipNextErrorCheck = false;
                                 return;
                             }
                             ConfigHandler().checkAndHandleError();
                             if (!QFile(fileName).exists()) {
                                 // File watcher stops watching a deleted file.
                                 // Next time the config is accessed, force it
                                 // to check for errors (and watch again).
                                 m_errorCheckPending = true;
                             }
                         });
    }
    firstInitialization = false;
}

/// Serves as an object to which slots can be connected.
ConfigHandler* ConfigHandler::getInstance()
{
    static ConfigHandler config;
    return &config;
}

// DEFAULTS

QString ConfigHandler::filenamePatternDefault()
{
    return QStringLiteral("%F_%H-%M");
}

void ConfigHandler::setDefaultSettings()
{
    for (const auto& key : m_settings.allKeys()) {
        if (isShortcut(key)) {
            // Do not reset Shortcuts
            continue;
        }
        m_settings.remove(key);
    }
    m_settings.sync();
}

QString ConfigHandler::configFilePath() const
{
    return m_settings.fileName();
}

// GENERIC GETTERS AND SETTERS

bool ConfigHandler::setShortcut(const QString& actionName,
                                const QString& shortcut)
{
    qDebug() << actionName;
    static QVector<QKeySequence> reservedShortcuts = {
#if defined(Q_OS_MACOS)
        Qt::CTRL | Qt::Key_Backspace,
        Qt::Key_Escape,
#else
        Qt::Key_Backspace,
        Qt::Key_Escape,
#endif
    };

    if (hasError()) {
        return false;
    }

    bool errorFlag = false;

    m_settings.beginGroup(CONFIG_GROUP_SHORTCUTS);
    if (shortcut.isEmpty()) {
        setValue(actionName, "");
    } else if (reservedShortcuts.contains(QKeySequence(shortcut))) {
        // do not allow to set reserved shortcuts
        errorFlag = true;
    } else {
        errorFlag = false;
        // Make no difference for Return and Enter keys
        QString newShortcut = KeySequence().value(shortcut).toString();
        for (auto& otherAction : m_settings.allKeys()) {
            if (actionName == otherAction) {
                continue;
            }
            QString existingShortcut =
              KeySequence().value(m_settings.value(otherAction)).toString();
            if (newShortcut == existingShortcut) {
                errorFlag = true;
                goto done;
            }
        }
        m_settings.setValue(actionName, KeySequence().value(shortcut));
    }
done:
    m_settings.endGroup();
    return !errorFlag;
}

QString ConfigHandler::shortcut(const QString& actionName)
{
    QString setting = CONFIG_GROUP_SHORTCUTS "/" + actionName;
    QString shortcut = value(setting).toString();
    if (!m_settings.contains(setting)) {
        // The action uses a shortcut that is a flameshot default
        // (not set explicitly by user)
        m_settings.beginGroup(CONFIG_GROUP_SHORTCUTS);
        for (auto& otherAction : m_settings.allKeys()) {
            if (m_settings.value(otherAction) == shortcut) {
                // We found an explicit shortcut - it will take precedence
                m_settings.endGroup();
                return {};
            }
        }
        m_settings.endGroup();
    }
    return shortcut;
}

void ConfigHandler::setValue(const QString& key, const QVariant& value)
{
    assertKeyRecognized(key);
    if (!hasError()) {
        // don't let the file watcher initiate another error check
        m_skipNextErrorCheck = true;
        auto val = valueHandler(key)->representation(value);
        m_settings.setValue(key, val);
    }
}

QVariant ConfigHandler::value(const QString& key) const
{
    assertKeyRecognized(key);

    auto val = m_settings.value(key);

    auto handler = valueHandler(key);

    // Check the value for semantic errors
    if (val.isValid() && !handler->check(val)) {
        setErrorState(true);
    }
    if (m_hasError) {
        return handler->fallback();
    }

    return handler->value(val);
}

void ConfigHandler::remove(const QString& key)
{
    m_settings.remove(key);
}

void ConfigHandler::resetValue(const QString& key)
{
    m_settings.setValue(key, valueHandler(key)->fallback());
}

QSet<QString>& ConfigHandler::recognizedGeneralOptions()
{
    auto keys = ::recognizedGeneralOptions.keys();
    static QSet<QString> options = QSet<QString>(keys.begin(), keys.end());
    return options;
}

/**
 * @brief Return keys from group `group`.
 * Use CONFIG_GROUP_GENERAL (General) for general settings.
 */
QSet<QString> ConfigHandler::keysFromGroup(const QString& group) const
{
    QSet<QString> keys;
    for (const QString& key : m_settings.allKeys()) {
        if (group == CONFIG_GROUP_GENERAL && !key.contains('/')) {
            keys.insert(key);
        } else if (key.startsWith(group + "/")) {
            keys.insert(baseName(key));
        }
    }
    return keys;
}

// ERROR HANDLING

bool ConfigHandler::checkForErrors(AbstractLogger* log) const
{
    return checkUnrecognizedSettings(log) && checkShortcutConflicts(log) &&
           checkSemantics(log);
}

/**
 * @brief Parse the config to find settings with unrecognized names.
 * @return Whether the config passes this check.
 *
 * @note An unrecognized option is one that is not included in
 * `recognizedGeneralOptions` or `recognizedShortcutNames` depending on the
 * group the option belongs to.
 */
bool ConfigHandler::checkUnrecognizedSettings(AbstractLogger* log,
                                              QList<QString>* offenders) const
{
    // sort the config keys by group
    QSet<QString> generalKeys = keysFromGroup(CONFIG_GROUP_GENERAL);

    return true;
}

/**
 * @brief Check if there are multiple actions with the same shortcut.
 * @return Whether the config passes this check.
 *
 * @note It is not considered a conflict if action A uses shortcut S because it
 * is the flameshot default (not because the user explicitly configured it), and
 * action B uses the same shortcut.
 */
bool ConfigHandler::checkShortcutConflicts(AbstractLogger* log) const
{
    bool ok = true;
    m_settings.beginGroup(CONFIG_GROUP_SHORTCUTS);
    QStringList shortcuts = m_settings.allKeys();
    QStringList reportedInLog;
    for (auto key1 = shortcuts.begin(); key1 != shortcuts.end(); ++key1) {
        for (auto key2 = key1 + 1; key2 != shortcuts.end(); ++key2) {
            // values stored in variables are useful when running debugger
            QString value1 = m_settings.value(*key1).toString(),
                    value2 = m_settings.value(*key2).toString();
            // The check will pass if:
            // - one shortcut is empty (the action doesn't use a shortcut)
            // - or one of the settings is not found in m_settings, i.e.
            //   user wants to use flameshot's default shortcut for the action
            // - or the shortcuts for both actions are different
            if (!(value1.isEmpty() || !m_settings.contains(*key1) ||
                  !m_settings.contains(*key2) || value1 != value2)) {
                ok = false;
                if (log == nullptr) {
                    break;
                } else if (!reportedInLog.contains(*key1) && // No duplicate
                           !reportedInLog.contains(*key2)) { // log entries
                    reportedInLog.append(*key1);
                    reportedInLog.append(*key2);
                }
            }
        }
    }
    m_settings.endGroup();
    return ok;
}

/**
 * @brief Check each config value semantically.
 * @param log Destination for error log output.
 * @param offenders Destination for the semantically invalid keys.
 * @return Whether the config passes this check.
 */
bool ConfigHandler::checkSemantics(AbstractLogger* log,
                                   QList<QString>* offenders) const
{
    QStringList allKeys = m_settings.allKeys();
    bool ok = true;
    return ok;
}

/**
 * @brief Parse the configuration to find any errors in it.
 *
 * If the error state changes as a result of the check, it will perform the
 * appropriate action, e.g. notify the user.
 *
 * @see ConfigHandler::setErrorState for all the actions.
 */
void ConfigHandler::checkAndHandleError() const
{
    if (!QFile(m_settings.fileName()).exists()) {
        setErrorState(false);
    } else {
        setErrorState(!checkForErrors());
    }

    ensureFileWatched();
}

/**
 * @brief Update the tracked error state of the config.
 * @param error The new error state.
 *
 * The error state is tracked so that signals are not emitted and the user is
 * not spammed every time the config file changes. Instead, only changes in
 * error state get reported.
 */
void ConfigHandler::setErrorState(bool error) const
{
    bool hadError = m_hasError;
    m_hasError = error;
    // Notify user every time m_hasError changes
    if (!hadError && m_hasError) {
        QString msg = errorMessage();
        emit getInstance()->error();
    } else if (hadError && !m_hasError) {
        auto msg =
          tr("You have successfully resolved the configuration error.");
        emit getInstance()->errorResolved();
    }
}

/**
 * @brief Return if the config contains an error.
 *
 * If an error check is due, it will be performed.
 */
bool ConfigHandler::hasError() const
{
    if (m_errorCheckPending) {
        checkAndHandleError();
        m_errorCheckPending = false;
    }
    return m_hasError;
}

/// Error message that can be used by other classes as well
QString ConfigHandler::errorMessage() const
{
    return tr(
      "The configuration contains an error. Open configuration to resolve.");
}

void ConfigHandler::ensureFileWatched() const
{
    QFile file(m_settings.fileName());
    if (!file.exists()) {
        if (!file.open(QFileDevice::WriteOnly)) {
            AbstractLogger::warning() << "Failed to open config file.";
        }
        file.close();
    }
    if (m_configWatcher != nullptr && m_configWatcher->files().isEmpty() &&
        qApp != nullptr // ensures that the organization name can be accessed
    ) {
        m_configWatcher->addPath(m_settings.fileName());
    }
}

void ConfigHandler::checkImport()
{
    if (ConfigHandler::getInstance()->uploadTokenTPU() != "") return;

    QString oldConfigPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                            + "/flameshot/flameshot.ini";

    QFile oldConfigFile(oldConfigPath);
    if (!oldConfigFile.exists()) {
        return;
    }

    QSettings oldSettings(oldConfigPath, QSettings::IniFormat);

    ConfigHandler* config = ConfigHandler::getInstance();
    QSet<QString> recognizedKeys = config->recognizedGeneralOptions();

    // Import general options
    for (const QString& key : recognizedKeys) {
        AbstractLogger::info() << "Importing key: " + key;
        if (oldSettings.contains(key)) {
            QVariant value = oldSettings.value(key);
            config->setValue(key, value);
        }
    }

    config->m_settings.sync();
}

/**
 * @brief Obtain a `ValueHandler` for the config option with the given key.
 * @return Smart pointer to the handler.
 *
 * @note If the key is from the CONFIG_GROUP_GENERAL (General) group, the
 * `recognizedGeneralOptions` map is looked up. If it is from
 * CONFIG_GROUP_SHORTCUTS (Shortcuts), a generic `KeySequence` value handler is
 * returned.
 */
QSharedPointer<ValueHandler> ConfigHandler::valueHandler(
  const QString& key) const
{
    QSharedPointer<ValueHandler> handler;
    if (isShortcut(key)) {
    } else { // General group
        handler = ::recognizedGeneralOptions.value(key);
    }
    return handler;
}

/**
 * This is used so that we can check if there is a mismatch between a config key
 * and its getter function.
 * Debug: throw an exception; Release: set error state
 */
void ConfigHandler::assertKeyRecognized(const QString& key) const
{
    return;
}

bool ConfigHandler::isShortcut(const QString& key) const
{
    return m_settings.group() == QStringLiteral(CONFIG_GROUP_SHORTCUTS) ||
           key.startsWith(QStringLiteral(CONFIG_GROUP_SHORTCUTS "/"));
}

QString ConfigHandler::baseName(const QString& key) const
{
    return QFileInfo(key).baseName();
}

// STATIC MEMBER DEFINITIONS

bool ConfigHandler::m_hasError = false;
bool ConfigHandler::m_errorCheckPending = true;
bool ConfigHandler::m_skipNextErrorCheck = false;

QSharedPointer<QFileSystemWatcher> ConfigHandler::m_configWatcher;
