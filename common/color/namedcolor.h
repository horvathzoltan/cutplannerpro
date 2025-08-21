#pragma once

#include <QString>
#include <QColor>

enum class RalSystem {
    Classic,
    Design,
    Plastic1,
    Plastic2,
    Effect,
    Unknown
};

namespace RalSystemUtils {
    inline QString toString(RalSystem system) {
        switch (system) {
        case RalSystem::Classic: return "Classic";
        case RalSystem::Design:  return "Design";
        case RalSystem::Plastic1: return "Plastic 1";
        case RalSystem::Plastic2: return "Plastic 2";
        case RalSystem::Effect:  return "Effect";
        default:                 return "Unknown";
        }
    }
}


struct RalSource {
    RalSystem system;
    QString filePath;
};

class NamedColor {
public:
    NamedColor() = default;
    NamedColor(const QColor& color, const QString& name);
    NamedColor(const QString& code); // HEX vagy RAL
    //NamedColor(const QColor &color, const QString &name, const QString &code);
    NamedColor(const QColor& color, const QString& name, const QString& code, RalSystem system);

    static bool initRalColors(const QList<RalSource>& sources);
    //static NamedColor fromRal(const QString& ralCode);
    //static QMap<QString, NamedColor> loadRalDatabase(const QString& filePath);

    QColor color() const;
    QString name() const;
    QString code() const;
    RalSystem system() const;

    static NamedColor fromRal(RalSystem system, const QString& ralCode);
    static NamedColor fromHex(const QString& hexCode);
    static NamedColor fromRal(const QString &ralCode);

    QString toString() const;
    bool isValid() const;
private:
    QColor m_color;
    QString m_name;
    QString m_code;
    RalSystem m_system = RalSystem::Unknown;

    static QMap<RalSystem, QMap<QString, NamedColor>> ralColors_;
    static QString normalizeRalExtended(const QString &raw);
};
