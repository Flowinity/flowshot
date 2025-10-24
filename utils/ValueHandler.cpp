#include "ValueHandler.h"
#include <QColor>
#include <QFileInfo>
#include <QImageWriter>
#include <QKeySequence>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVariant>


// VALUE HANDLER

QVariant ValueHandler::value(const QVariant& val)
{
    if (!val.isValid() || !check(val)) {
        return fallback();
    } else {
        return process(val);
    }
}

QVariant ValueHandler::fallback()
{
    return {};
}

QVariant ValueHandler::representation(const QVariant& val)
{
    return val.toString();
}

QString ValueHandler::expected()
{
    return {};
}

QVariant ValueHandler::process(const QVariant& val)
{
    return val;
}

// BOOL

Bool::Bool(bool def)
  : m_def(def)
{}

bool Bool::check(const QVariant& val)
{
    QString str = val.toString();
    if (str != "true" && str != "false") {
        return false;
    }
    return true;
}

QVariant Bool::fallback()
{
    return m_def;
}

QString Bool::expected()
{
    return QStringLiteral("true or false");
}

// STRING

String::String(QString def)
  : m_def(std::move(def))
{}

bool String::check(const QVariant&)
{
    return true;
}

QVariant String::fallback()
{
    return m_def;
}

QString String::expected()
{
    return QStringLiteral("string");
}

// COLOR

Color::Color(QColor def)
  : m_def(std::move(def))
{}

bool Color::check(const QVariant& val)
{
    QString str = val.toString();
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
    bool validColor = QColor::isValidColor(str);
#else
    bool validColor = QColor::isValidColorName(str);
#endif
    // Disable #RGB, #RRRGGGBBB and #RRRRGGGGBBBB formats that QColor supports
    return validColor &&
           (str[0] != '#' ||
            (str.length() != 4 && str.length() != 10 && str.length() != 13));
}

QVariant Color::process(const QVariant& val)
{
    QString str = val.toString();
    QColor color(str);
    if (str.length() == 9 && str[0] == '#') {
        // Convert #RRGGBBAA (flameshot) to #AARRGGBB (QColor)
        int blue = color.blue();
        color.setBlue(color.green());
        color.setGreen(color.red());
        color.setRed(color.alpha());
        color.setAlpha(blue);
    }
    return color;
}

QVariant Color::fallback()
{
    return m_def;
}

QVariant Color::representation(const QVariant& val)
{
    QString str = val.toString();
    QColor color(str);
    if (str.length() == 9 && str[0] == '#') {
        // Convert #AARRGGBB (QColor) to #RRGGBBAA (flameshot)
        int alpha = color.alpha();
        color.setAlpha(color.red());
        color.setRed(color.green());
        color.setGreen(color.blue());
        color.setBlue(alpha);
    }
    return color.name();
}

QString Color::expected()
{
    return QStringLiteral("color name or hex value");
}

// BOUNDED INT

BoundedInt::BoundedInt(int min, int max, int def)
  : m_min(min)
  , m_max(max)
  , m_def(def)
{}

bool BoundedInt::check(const QVariant& val)
{
    QString str = val.toString();
    bool conversionOk;
    int num = str.toInt(&conversionOk);
    return conversionOk && m_min <= num && num <= m_max;
}

QVariant BoundedInt::fallback()
{
    return m_def;
}

QString BoundedInt::expected()
{
    return QStringLiteral("number between %1 and %2").arg(m_min, m_max);
}

// LOWER BOUNDED INT

LowerBoundedInt::LowerBoundedInt(int min, int def)
  : m_min(min)
  , m_def(def)
{}

bool LowerBoundedInt::check(const QVariant& val)
{
    QString str = val.toString();
    bool conversionOk;
    int num = str.toInt(&conversionOk);
    return conversionOk && num >= m_min;
}

QVariant LowerBoundedInt::fallback()
{
    return m_def;
}

QString LowerBoundedInt::expected()
{
    return QStringLiteral("number >= %1").arg(m_min);
}


// LOWER BOUNDED FLOAT

LowerBoundedDouble::LowerBoundedDouble(double min, double def)
  : m_min(min)
  , m_def(def)
{}

bool LowerBoundedDouble::check(const QVariant& val)
{
    QString str = val.toString();
    bool conversionOk;
    int num = str.toDouble(&conversionOk);
    return conversionOk && num >= m_min;
}

QVariant LowerBoundedDouble::fallback()
{
    return m_def;
}

QString LowerBoundedDouble::expected()
{
    return QStringLiteral("number >= %1").arg(m_min);
}

// KEY SEQUENCE

KeySequence::KeySequence(const QKeySequence& fallback)
  : m_fallback(fallback)
{}

bool KeySequence::check(const QVariant& val)
{
    QString str = val.toString();
    if (!str.isEmpty() && QKeySequence(str).toString().isEmpty()) {
        return false;
    }
    return true;
}

QVariant KeySequence::fallback()
{
    return process(m_fallback);
}

QString KeySequence::expected()
{
    return QStringLiteral("keyboard shortcut");
}

QVariant KeySequence::representation(const QVariant& val)
{
    QString str(val.toString());
    if (QKeySequence(str) == QKeySequence(Qt::Key_Return)) {
        return QStringLiteral("Enter");
    }
    return str;
}

QVariant KeySequence::process(const QVariant& val)
{
    QString str(val.toString());
    if (str == "Enter") {
        return QKeySequence(Qt::Key_Return).toString();
    }
    if (str.length() > 0) {
        // Make the "main" key in sequence (last one) lower-case.
        str[str.length() - 1] = str[str.length() - 1].toLower();
    }
    return str;
}

// EXISTING DIR

bool ExistingDir::check(const QVariant& val)
{
    if (!val.canConvert<QString>() || val.toString().isEmpty()) {
        return false;
    }
    QFileInfo info(val.toString());
    return info.isDir() && info.exists();
}

QVariant ExistingDir::fallback()
{
    using SP = QStandardPaths;
    for (auto location :
         { SP::PicturesLocation, SP::HomeLocation, SP::TempLocation }) {
        QString path = SP::writableLocation(location);
        if (QFileInfo(path).isDir()) {
            return path;
        }
    }
    return {};
}

QString ExistingDir::expected()
{
    return QStringLiteral("existing directory");
}

// FILENAME PATTERN

bool FilenamePattern::check(const QVariant&)
{
    return true;
}

QVariant FilenamePattern::fallback()
{
    return ConfigHandler().filenamePatternDefault();
}

QVariant FilenamePattern::process(const QVariant& val)
{
    QString str = val.toString();
    return !str.isEmpty() ? val : fallback();
}

QString FilenamePattern::expected()
{
    return QStringLiteral("please edit using the GUI");
}

// SET SAVE FILE AS EXTENSION

bool SaveFileExtension::check(const QVariant& val)
{
    if (!val.canConvert<QString>() || val.toString().isEmpty()) {
        return false;
    }

    QString extension = val.toString();

    if (extension.startsWith(".")) {
        extension.remove(0, 1);
    }

    QStringList imageFormatList;
    for (const auto& imageFormat : QImageWriter::supportedImageFormats())
        imageFormatList.append(imageFormat);

    if (!imageFormatList.contains(extension)) {
        return false;
    }

    return true;
}

QVariant SaveFileExtension::process(const QVariant& val)
{
    QString extension = val.toString();

    if (extension.startsWith(".")) {
        extension.remove(0, 1);
    }

    return QVariant::fromValue(extension);
}

QString SaveFileExtension::expected()
{
    return QStringLiteral("supported image extension");
}

