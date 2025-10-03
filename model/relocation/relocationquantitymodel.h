#pragma once

#include "model/relocation/relocationquantityrow.h"
#include <QVector>

/**
 * @brief A relokációs dialógusban használt mennyiségsorokat kezelő segédmodell.
 *
 * A RelocationQuantityModel egy egyszerű wrapper, amely a dialógusban megjelenő
 * RelocationQuantityRow sorokat kezeli, és segít a validációban, összesítésben,
 * valamint a dialógus eredményének kiolvasásában.
 */
class RelocationQuantityModel {
public:
    QVector<RelocationQuantityRow> rows;

    /**
     * @brief Visszaadja az összes felhasználó által megadott mennyiség összegét.
     */
    int totalSelected() const {
        int sum = 0;
        for (const auto& row : rows)
            sum += row.selected;
        return sum;
    }

    /**
     * @brief Visszaadja, hogy van-e bármilyen megadott mennyiség (munkaközi állapot).
     */
    bool hasAnySelection() const {
        for (const auto& row : rows)
            if (row.selected > 0)
                return true;
        return false;
    }

    /**
     * @brief Visszaadja, hogy minden sorban a megadott mennyiség megfelel-e a szabályoknak.
     *
     * Forrás esetén: selected ≤ available
     * Cél esetén: nincs külön limit, de a dialógusban globálisan validáljuk
     */
    bool isValid() const {
        for (const auto& row : rows) {
            if (!row.isTarget && row.selected > row.available)
                return false;
        }
        return true;
    }

    /**
     * @brief Beállítja az összes selected értéket nullára (reset).
     */
    void clearSelections() {
        for (auto& row : rows)
            row.selected = 0;
    }

    /**
     * @brief Egyenletesen elosztja a megadott mennyiséget a cél sorok között.
     *
     * Csak cél sorokra alkalmazható.
     */
    void distributeEvenly(int totalToDistribute) {
        int targetCount = 0;
        for (const auto& row : rows)
            if (row.isTarget)
                ++targetCount;

        if (targetCount == 0)
            return;

        int base = totalToDistribute / targetCount;
        int remainder = totalToDistribute % targetCount;

        for (auto& row : rows) {
            if (!row.isTarget)
                continue;
            row.selected = base + (remainder > 0 ? 1 : 0);
            if (remainder > 0)
                --remainder;
        }
    }
};
