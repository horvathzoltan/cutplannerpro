#pragma once
#include <QVector>
#include <QString>
#include <QList>
#include <functional>
#include <optional>

/**
 * @brief Segédosztály CSV sorok feldolgozásához sablonos konverterrel.
 *
 * A sorokat generikusan lehet konvertálni egy kívánt típusra. A fejléc sor automatikusan kihagyásra kerül.
 */
class CsvImporter {
public:
    /**
     * @brief Feldolgoz egy CSV-táblázatot és konvertálja a sorokat objektumokká.
     *
     * @tparam T A cél típus, amit a sorokból szeretnél visszakapni.
     * @param rows A parse-olt CSV sorok, már split-elve (például FileHelper::parseCSV eredménye).
     * @param converter Függvény, amely egy sorból és fájlbeli sorszámból visszaadja az objektumot, vagy nullopt, ha a konvertálás sikertelen.
     * @return QVector<T> A sikeresen konvertált objektumok listája.
     */
    template<typename T>
    static QVector<T> processCsvRows(const QList<QVector<QString>>& rows,
                                     std::function<std::optional<T>(const QVector<QString>&, int)> converter)
    {
        QVector<T> result;
        for (int i = 0; i < rows.size(); ++i) {
            if (i == 0) continue; // Fejléc sor kihagyása

            const auto& row = rows[i];
            auto maybeObj = converter(row, i + 1); // fájlbeli sorszám = index + 1
            if (maybeObj.has_value())
                result.append(maybeObj.value());
        }
        return result;
    }
};
