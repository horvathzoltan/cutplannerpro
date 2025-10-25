#pragma once

#include "model/storageaudit/auditcontext.h"
#include <QString>
#include <QMap>

namespace TableUtils {

/**
 * @brief AuditContext csoportokhoz betűjelet rendel (A, B, C, ...).
 *
 * Célja:
 * - Vizuálisan megkülönböztethetővé tenni az auditcsoportokat.
 * - Segíteni a cellákban való megjelenítést (pl. "5 db (anyagcsoport B)").
 * - A felhasználó számára kontextust adni a csoporttagsághoz.
 */
class AuditGroupLabeler {
public:
    /**
     * @brief Visszaadja a csoporthoz tartozó betűjelet.
     * Ha még nincs hozzárendelve, automatikusan generál egyet.
     */
    QString labelFor(const AuditContext* ctx);

    /**
     * @brief Törli az összes hozzárendelt betűjelet.
     * Hasznos új táblabetöltéskor.
     */
    void clear();

private:
    QMap<QString, QString> _labels; // 🔑 groupKey → label
};

} // namespace TableUtils
