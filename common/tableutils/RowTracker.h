#pragma once

#include <QHash>
#include <QMap>
#include <QVariant>
#include <QUuid>
#include <optional>

// Egyszerű, stabil sor–azonosító leképező. Nem tárol modelleket, csak az ID-kat és a sorindexet.
class RowTracker {
public:
    struct RowInfo {
        int rowIndex = -1;
        QUuid id;
        QVariant extra; // opcionális: pl. groupId, rowType, stb.
    };

    QMap<QUuid, int> _rowIndexMap;

    // Regisztrál egy sort (insertRow után hívd)
    inline void registerRow(int rowIndex, const QUuid& id, const QVariant& extra = {}) {
        RowInfo info{rowIndex, id, extra};
        _byRow.insert(rowIndex, info);
        _byId.insert(id, rowIndex);
        _rowIndexMap[id] = rowIndex;
    }

    // Töröld, ha egy sort eltávolítasz a táblából (removeRow előtt vagy után is jó, lásd sync)
    inline void unregisterRowByIndex(int rowIndex) {
        auto it = _byRow.find(rowIndex);
        if (it == _byRow.end()) return;
        _byId.remove(it->id);
        _byRow.erase(it);
    }

    // Azonosító alapján gyors elérés a sorindexhez
    inline std::optional<int> rowOf(const QUuid& id) const {
        auto it = _byId.constFind(id);
        if (it == _byId.constEnd()) return std::nullopt;
        return it.value();
    }

    // Sorindex alapján gyors elérés a RowInfo-hoz
    inline std::optional<RowInfo> infoAt(int rowIndex) const {
        auto it = _byRow.constFind(rowIndex);
        if (it == _byRow.constEnd()) return std::nullopt;
        return it.value();
    }

    // Extra adat elérése ID alapján (ha használsz extra-t)
    inline std::optional<QVariant> extraOf(const QUuid& id) const {
        auto r = rowOf(id);
        if (!r) return std::nullopt;
        auto it = _byRow.constFind(*r);
        if (it == _byRow.constEnd()) return std::nullopt;
        return it->extra;
    }

    // Tábla műveletek után hívd a szinkronokat:

    // Használd, miután egy sort BESZÚRTÁL az adott indexre (insertRow)
    inline void syncAfterInsert(int insertedAtRow) {
        // minden sorindex, ami >= insertedAtRow, tolódik +1-gyel
        QMap<int, RowInfo> newByRow;
        _byRowDetached.clear();

        for (auto it = _byRow.begin(); it != _byRow.end(); ++it) {
            RowInfo info = it.value();
            if (info.rowIndex >= insertedAtRow) {
                info.rowIndex += 1;
            }
            newByRow.insert(info.rowIndex, info);
            _byId[info.id] = info.rowIndex;
        }
        _byRow = newByRow;
    }

    // Használd, MIUTÁN egy sort eltávolítottál (removeRow)
    inline void syncAfterRemove(int removedAtRow) {
        // előbb vedd ki a törölt sort (unregisterRowByIndex), majd hívd ezt
        QMap<int, RowInfo> newByRow;
        _byRowDetached.clear();

        for (auto it = _byRow.begin(); it != _byRow.end(); ++it) {
            RowInfo info = it.value();
            if (info.rowIndex > removedAtRow) {
                info.rowIndex -= 1;
            }
            newByRow.insert(info.rowIndex, info);
            _byId[info.id] = info.rowIndex;
        }
        _byRow = newByRow;
    }

    // Ha teljesen újratöltöd a táblát
    inline void clear() {
        _byRow.clear();
        _byId.clear();
        _byRowDetached.clear();
    }

    // Opcionális: ha mozgatod a sort (drag&drop vagy programozott move)
    inline void movedRow(int oldRow, int newRow) {
        auto it = _byRow.find(oldRow);
        if (it == _byRow.end()) return;
        RowInfo info = it.value();
        _byRow.erase(it);

        // köztesek eltolása
        QMap<int, RowInfo> newByRow;
        for (auto jt = _byRow.begin(); jt != _byRow.end(); ++jt) {
            RowInfo r = jt.value();
            if (oldRow < newRow) {
                // lefelé mozgatás: a (oldRow, newRow] tartomány -1
                if (r.rowIndex > oldRow && r.rowIndex <= newRow) r.rowIndex -= 1;
            } else if (newRow < oldRow) {
                // felfelé mozgatás: a [newRow, oldRow) tartomány +1
                if (r.rowIndex >= newRow && r.rowIndex < oldRow) r.rowIndex += 1;
            }
            newByRow.insert(r.rowIndex, r);
            _byId[r.id] = r.rowIndex;
        }

        info.rowIndex = newRow;
        newByRow.insert(newRow, info);
        _byId[info.id] = newRow;

        _byRow = newByRow;
    }

    inline const QMap<QUuid, int>& rowIndexMap() const {
        return _rowIndexMap;
    }

private:
    // sorindex -> info (rendezett a kulcs szerint)
    QMap<int, RowInfo> _byRow;
    // id -> sorindex (gyors lookup)
    QHash<QUuid, int> _byId;

    // fenntartott: ha később szeretnél ideiglenesen leválasztott sorokat kezelni
    QMap<int, RowInfo> _byRowDetached;
};
