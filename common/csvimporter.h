#pragma once
#include <QVector>
#include <QString>
#include <QList>
#include <functional>
#include <optional>
#include <QIODevice>
#include <QFile>
#include <QTextStream>
#include "common/logger.h"
#include "filehelper.h"

namespace CsvReader{
// inline QList<QVector<QString>> read(const QString& filepath, QChar separator = ';'){
//     QFile file(filepath);
//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         QString msg = L("❌ Nem sikerült megnyitni a csv fájlt:").arg(filepath);
//         zWarning(msg);
//         return {};
//     }

//     QTextStream in(&file);
//     in.setEncoding(QStringConverter::Utf8);
//     const auto rows = FileHelper::parseCSV(&in, separator);
//     return rows;
// }

//template<typename T>

struct RowError{
private:
    int _lineIndex;
    QString _errorMessage;
public:
    RowError(int lineIndex, const QString& errorMessage = QString())
        : _lineIndex(lineIndex), _errorMessage(errorMessage) {}

    QString toString() const {
        if (_errorMessage.isEmpty())
            return QString("⚠️ Ismeretlen hiba (Sor: %1)\n").arg(_lineIndex);
        return QString("%1 (Sor: %2)\n").arg(_errorMessage, QString::number(_lineIndex));
    }

};

struct FileContext {
private:
    QString _filepath;
    QVector<RowError> _errors;
    int _currentLineNumber = 0;
public:

    FileContext(const QString& filepath)
        : _filepath(filepath) {}

    QString filepath() const { return _filepath; }

    void setCurrentLineNumber(int lineNumber) {
        _currentLineNumber = lineNumber;
    }

    int currentLineNumber() const {
        return _currentLineNumber;
    }

    void addError(int l, const QString& msg) {
        _errors.append({l, msg});
    }

    bool hasErrors() const {
        return !_errors.isEmpty();
    }

    int errorsSize() const {
        return _errors.size();
    }

    QString toString() const {
        QString result = QString("📄 Fájl: %1\n").arg(_filepath);

        if (_errors.isEmpty()) {
            result += "✅ Nincs hiba.\n";
        } else {
            result += "Hibalista:\n";
            int idx = 1;
            for (const auto& err : _errors) {
                result += QString("  %1. %2").arg(idx++).arg(err.toString());
            }
        }

        return result;
    }

};

inline QList<QVector<QString>> read(const QString& filepath, QChar separator = QChar()) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString msg = L("❌ Nem sikerült megnyitni a csv fájlt:").arg(filepath);
        zWarning(msg);
        return {};
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    // 🔍 Automatikus szeparátor detektálás, ha nincs megadva
    if (separator.isNull()) {
     //   zInfo("🔍 Automatikus szeparátor keresés...");
        separator = FileHelper::detectSeparatorSmart(&in);
        if (separator.isNull()) {
    //        zWarning(L("❌ Nem sikerült szeparátort detektálni a fájlban:").arg(filepath));
            return {};
        }
        file.seek(0); // 🔁 Vissza az elejére, újraolvasáshoz
        in.seek(0);
    }

    const auto rows = FileHelper::parseCSV(&in, separator);
    return rows;
}

template<typename T>
static QVector<T> readAndConvert(CsvReader::FileContext& ctx,
                                 std::function<std::optional<T>(const QVector<QString>&, FileContext&)> converter,
                                 bool skipHeader = true)
{
    const auto rows = read(ctx.filepath());
    QVector<T> result;

    for (int i = 0; i < rows.size(); ++i) {
        if (skipHeader && i == 0) continue;

        const auto& row = rows[i];
        ctx.setCurrentLineNumber(i + 1);

        //RowContext ctx(linenumber, filepath);
        auto maybeObj = converter(row, ctx);
        if (maybeObj.has_value())
            result.append(std::move(maybeObj.value()));
    }

    return result;
}
}
