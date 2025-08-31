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
struct RowContext {
    int lineIndex;
    QString filepath;
 //   T row;
};

struct ImortError_Line{
    int lineIndex;
    QString message;
};

struct ImportError {
    QString fileName;
    QVector<ImortError_Line> errors;
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
static QVector<T> readAndConvert(const QString& filepath,
                                 std::function<std::optional<T>(const QVector<QString>&, RowContext&)> converter,
                                 bool skipHeader = true)
{
    const auto rows = read(filepath);
    QVector<T> result;

    for (int i = 0; i < rows.size(); ++i) {
        if (skipHeader && i == 0) continue;

        const auto& row = rows[i];        
        int linenumber = i + 1;

        RowContext ctx(linenumber, filepath);
        auto maybeObj = converter(row, ctx);
        if (maybeObj.has_value())
            result.append(std::move(maybeObj.value()));
    }

    return result;
}

// template<typename T>
// static QVector<T> readAndConvert(const QString& filepath,
//                                  std::function<std::optional<T>(const QVector<QString>&, int)> converter,
//                                  bool skipHeader = true)
// {
//     const auto rows = read(filepath);
//     QVector<T> result;

//     for (int i = 0; i < rows.size(); ++i) {
//         if (skipHeader && i == 0) continue;

//         const auto& row = rows[i];
//         auto maybeObj = converter(row, i + 1);
//         if (maybeObj.has_value())
//             result.append(std::move(maybeObj.value()));
//     }

//     return result;
// }
}// endof namespace CsvReader

/**
 * @brief Segédosztály CSV sorok feldolgozásához sablonos konverterrel.
 *
 * A sorokat generikusan lehet konvertálni egy kívánt típusra. A fejléc sor automatikusan kihagyásra kerül.
 */
// class CsvImporter {
// public:
//     /**
//      * @brief Feldolgoz egy CSV-táblázatot és konvertálja a sorokat objektumokká.
//      *
//      * @tparam T A cél típus, amit a sorokból szeretnél visszakapni.
//      * @param rows A parse-olt CSV sorok, már split-elve (például FileHelper::parseCSV eredménye).
//      * @param converter Függvény, amely egy sorból és fájlbeli sorszámból visszaadja az objektumot, vagy nullopt, ha a konvertálás sikertelen.
//      * @return QVector<T> A sikeresen konvertált objektumok listája.
//      */
//     template<typename T>
//     static QVector<T> processCsvRows(const QList<QVector<QString>>& rows,
//                                      std::function<std::optional<T>(const QVector<QString>&, int)> converter)
//     {
//         QVector<T> result;
//         for (int i = 0; i < rows.size(); ++i) {
//             if (i == 0) continue; // Fejléc sor kihagyása

//             const auto& row = rows[i];
//             auto maybeObj = converter(row, i + 1); // fájlbeli sorszám = index + 1
//             if (maybeObj.has_value())
//                 result.append(std::move(maybeObj.value()));
//         }
//         return result;
//     }
// };
