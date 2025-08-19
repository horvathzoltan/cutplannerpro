#include "namedcolor.h"
#include <QMap>
#include <QColor>
#include <QDebug>
#include "../csvimporter.h" //"../../csvreader.h"

// Példa RAL adatbázis (valóságban fájlból vagy globális mapből jönne)
// static const QMap<QString, NamedColor> ralDatabase = {
//     { "RAL 1001", NamedColor(QColor("#C2B078"), "Beige", "RAL 1001") },
//     { "RAL 1002", NamedColor(QColor("#D6C48E"), "Sand Yellow", "RAL 1002") },
//     // ...
// };

//static const QMap<QString, NamedColor> ralDatabase = loadRalColorsFromFile(":/data/ral_colors.txt");

QMap<RalSystem, QMap<QString, NamedColor>> NamedColor::ralColors_;


// NamedColor implementáció
NamedColor::NamedColor(const QColor& color, const QString& name)
    : m_color(color), m_name(name), m_code(color.name().toUpper()) {}

// NamedColor::NamedColor(const QColor& color, const QString& name, const QString& code)
//     : m_color(color), m_name(name), m_code(code) {}

NamedColor::NamedColor(const QColor& color, const QString& name, const QString& code, RalSystem system)
    : m_color(color), m_name(name), m_code(code), m_system(system) {}


NamedColor::NamedColor(const QString& code)
{
    if (code.startsWith("RAL", Qt::CaseInsensitive)) {
        *this = fromRal(code);
    } else if(code.startsWith('#')) {
        QColor c(code);
        if (c.isValid()) {
            m_color = c;
            m_name = code.toUpper();
            m_code = code.toUpper();
        } else {
            m_color = QColor(Qt::black);
            m_name = "Invalid HEX";
            m_code = code;
        }

        *this = fromHex(code);
    }
    else if (QColor::colorNames().contains(code.toLower())) {
        m_color = QColor(code.toLower());
        m_name = code.toLower();
        m_code = m_color.name().toUpper(); // HEX formában
    } else{
        m_color = QColor(Qt::black);
        m_name = "Invalid HEX";
        m_code = code;
    }

}

QColor NamedColor::color() const { return m_color; }
QString NamedColor::name() const { return m_name; }
QString NamedColor::code() const { return m_code; }

// NamedColor NamedColor::fromRal(const QString& ralCode)
// {
//     QString key = ralCode.trimmed().toUpper();
//     if (ralDatabase.contains(key)) {
//         return ralDatabase.value(key);
//     }
//     qWarning() << "Ismeretlen RAL kód:" << ralCode;
//     return NamedColor();
// }

// NamedColor NamedColor::fromRal(RalSystem system, const QString& ralCode)
// {
//     const QString key = ralCode.trimmed().toUpper();
//     return ralColors_.value(system).value(key, NamedColor(Qt::black, "Ismeretlen RAL", key, system));
// }

NamedColor NamedColor::fromRal(RalSystem system, const QString& ralCode)
{
    const QString key = ralCode.trimmed().toUpper();
    return ralColors_.value(system).value(key, NamedColor(Qt::black, "Ismeretlen RAL", key, system));
}

NamedColor NamedColor::fromRal(const QString& ralCode)
{
    const QString key = ralCode.trimmed().toUpper();

    for (auto it = ralColors_.constBegin(); it != ralColors_.constEnd(); ++it) {
        const auto& systemMap = it.value();
        if (systemMap.contains(key))
            return systemMap.value(key);
    }

    qWarning() << "Ismeretlen RAL kód (globális keresés):" << ralCode;
    return NamedColor(Qt::black, "Ismeretlen RAL", key, RalSystem::Unknown);
}

NamedColor NamedColor::fromHex(const QString& hexCode)
{
    QColor c(hexCode.trimmed());
    if (!c.isValid()) {
        qWarning() << "Érvénytelen HEX kód:" << hexCode;
        return NamedColor();
    }

    QString name = QColor::colorNames().contains(hexCode.toLower())
                       ? hexCode.toLower()
                       : QString();

    return NamedColor(c, name);
}

void NamedColor::initRalColors(const QList<RalSource>& sources)
{
    // auto converter = [](const QVector<QString>& row, int lineNumber) -> std::optional<std::pair<QString, NamedColor>> {
    //     if (row.size() != 3) {
    //         zWarning(L("❌ Hibás sor a RAL CSV-ben (%1): %2").arg(lineNumber).arg(row.join(";")));
    //         return std::nullopt;
    //     }

    //     const QString code = "RAL " + row[0].trimmed();      // pl. "RAL 1000"
    //     const QString name = row[1].trimmed();               // pl. "Green beige"
    //     const QString hex = row[2].trimmed();                // pl. "#CDBA88"

    //     const QColor color(hex);
    //     if (!color.isValid()) {
    //         zWarning(L("❌ Érvénytelen színkód a RAL CSV-ben (%1): %2").arg(lineNumber).arg(hex));
    //         return std::nullopt;
    //     }

    //     return std::make_pair(code.toUpper(), NamedColor(color, name));
    // };

    ralColors_.clear();

    // const auto items = CsvReader::readAndConvert<std::pair<QString, NamedColor>>(filePath, converter);
    // for (const auto& [key, value] : items)
    //     ralColors_.insert(key, value);

    // for (const QString& path : filePaths) {
    //     const auto items = CsvReader::readAndConvert<std::pair<QString, NamedColor>>(path, converter);
    //     for (const auto& [key, value] : items)
    //         ralColors_.insert(key, value);
    // }

    for (const RalSource& src : sources) {
        auto converter = [system = src.system](const QVector<QString>& row, int lineNumber) -> std::optional<std::pair<QString, NamedColor>> {
            if (row.size() != 3) return std::nullopt;

            const QString code = "RAL " + row[0].trimmed();
            const QString name = row[1].trimmed();
            const QString hex = row[2].trimmed();

            const QColor color(hex);
            if (!color.isValid()) return std::nullopt;

            return std::make_pair(code.toUpper(), NamedColor(color, name, code.toUpper(), system));
        };

        const auto items = CsvReader::readAndConvert<std::pair<QString, NamedColor>>(src.filePath, converter);
        for (const auto& [key, value] : items)
            ralColors_[src.system].insert(key, value);
    }
}

QString NamedColor::toString() const
{
    QString systemStr = RalSystemUtils::toString(m_system);
    return QString("%1 (%2) - %3").arg(m_code, systemStr, m_name);
}


