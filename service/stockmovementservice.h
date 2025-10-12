#include "common/logger.h"
#include "presenter/CuttingPresenter.h"
#include <model/registries/materialregistry.h>
#include <model/registries/stockregistry.h>
#include <model/registries/storageregistry.h>
#include "movementlogger.h"     // MovementLogger::log
//#include "model/movementlogmodel.h"    // MovementLogModel

class StockMovementService {
public:
    explicit StockMovementService(CuttingPresenter* presenter)
        : presenter_(presenter) {}

    // Convenience wrapper: ID alapján betölt, majd moveStock hívás
    bool move(const QUuid& originalId,
              const QUuid& toStorageId,
              int qty,
              const QString& comment)
    {
        if (qty <= 0) {
            zWarning(QStringLiteral("move: invalid qty=%1 (original=%2)").arg(qty).arg(originalId.toString()));
            return false;
        }

        auto opt = StockRegistry::instance().findById(originalId);
        if (!opt) {
            zWarning(QStringLiteral("move: source entry not found: %1").arg(originalId.toString()));
            return false;
        }

        const StockEntry& src = *opt;

        MovementData md;
        md.fromEntryId = originalId;          // source entry
        md.toStorageId  = toStorageId;        // cél storage (ha üres => deposit-only nem értelmezett itt)
        md.materialId   = src.materialId;     // material reference szükséges target-only esetekhez
        md.quantity     = qty;
        md.comment      = comment;
        // md.toEntryId marad QUuid() ha nincs specifikus cél entry

        zInfo(QStringLiteral("move: preparing move from entry=%1 storage=%2 material=%3 qty=%4 toStorage=%5")
                  .arg(md.fromEntryId.toString())
                  .arg(src.storageId.toString())
                  .arg(md.materialId.toString())
                  .arg(md.quantity)
                  .arg(md.toStorageId.toString()));

        // Meghívjuk az új moveStock(MovementData, presenter) implementációt
        return moveStock(md, presenter_);
    }

    inline bool finalizeRelocation(RelocationInstruction& instr)
    {
        zTrace();
        StockRegistry::instance().dumpAll(); // kezdő snapshot debughoz

        // 1) Strict előfeltétel: csak akkor folytatjuk, ha a teljes maradó mennyiség fedezett és nincs már finalizálva.
        if (!instr.isReadyToFinalize_Strict() || instr.isAlreadyFinalized()) {
            zInfo(QStringLiteral("finalizeRelocation failed: strict preconditions not met or already finalized for material %1")
                      .arg(instr.materialId.toString()));
            return false;
        }

        // 2) Mennyi a végrehajtandó mennyiség
        const int toExecute = instr.plannedRemaining();
        if (toExecute <= 0) {
            zWarning(QStringLiteral("finalizeRelocation: nothing to execute for material %1").arg(instr.materialId.toString()));
            return false;
        }

        // 3) Forrás oldal: végigmegyünk az instr.sources-on és a segédfüggvényeidet használjuk (moveStock végzi a tényleges consume/update-et)
        //    A dialogban megadott src.moved azt jelzi, mennyit akarunk ebből az entry-ből; tényleges elvétel a src.available alapján.
        int remaining = toExecute;
        for (const auto& src : instr.sources) {
            if (remaining <= 0) break;

            int want = src.moved;
            int avail = src.available;
            int take = qMin(remaining, qMin(want, avail));
            if (take <= 0) continue; // ebből a source-ból nincs mit venni

            // resolve entry: ha src.entryId megvan, moveStock használja azt; ha nincs, moveStock belső resolve-ot végezhet,
            // de mi itt preferáljuk a fromEntryId-kitöltést, ha elérhető
            QUuid fromEntry = src.entryId;
            if (fromEntry.isNull()) {
                // ha nincs explicit entryId, próbáljuk feloldani storage+material alapján
                if (!src.locationId.isNull()) {
                    if (auto resolved = StockRegistry::instance().findFirstByStorageAndMaterial(src.locationId, instr.materialId)) {
                        fromEntry = resolved->entryId;
                        zInfo(QStringLiteral("finalizeRelocation: resolved source entry %1 for storage %2")
                                  .arg(fromEntry.toString()).arg(src.locationId.toString()));
                    } else {
                        zWarning(QStringLiteral("finalizeRelocation: cannot resolve source entry for storage=%1; skipping this source")
                                     .arg(src.locationId.toString()));
                        continue;
                    }
                } else {
                    zWarning(QStringLiteral("finalizeRelocation: source has no entryId or locationId; skipping"));
                    continue;
                }
            }

            // Make MovementData for consume (fromEntry -> no target)
            MovementData md;
            md.fromEntryId = fromEntry;
            md.materialId  = instr.materialId;
            md.toStorageId = QUuid(); // consume-only step
            md.quantity    = take;
            md.comment     = QStringLiteral("Relocation strict finalize - source");

            zInfo(QStringLiteral("Calling moveStock(source): fromEntry=%1 qty=%2").arg(md.fromEntryId.toString()).arg(md.quantity));
            if (!moveStock(md, presenter_)) {
                zWarning(QStringLiteral("finalizeRelocation: moveStock failed for source entry=%1 qty=%2 — aborting")
                             .arg(md.fromEntryId.toString()).arg(md.quantity));
                return false;
            }

            remaining -= take;
        }

        // 4) Ellenőrzés: ha nem sikerült minden forrásból elvinni a szükséges mennyiséget, abort
        if (remaining != 0) {
            zWarning(QStringLiteral("finalizeRelocation: not enough moved from sources for material %1; remaining=%2")
                         .arg(instr.materialId.toString()).arg(remaining));
            return false;
        }

        // 5) Cél oldal: a dialogban megadott targets[].placed értékek alapján írjuk be a célokra.
        remaining = toExecute;
        for (const auto& tgt : instr.targets) {
            if (remaining <= 0) break;

            int put = qMin(remaining, tgt.placed);
            if (put <= 0) continue;

            MovementData md;
            md.fromEntryId = QUuid();         // deposit / aggregate lépésnél nincs explicit fromEntry
            md.materialId  = instr.materialId;
            md.toStorageId = tgt.locationId;  // pontos cél storage
            md.quantity    = put;
            md.comment     = QStringLiteral("Relocation strict finalize - target");

            zInfo(QStringLiteral("Calling moveStock(target): toStorage=%1 qty=%2").arg(md.toStorageId.toString()).arg(md.quantity));
            if (!moveStock(md, presenter_)) {
                zWarning(QStringLiteral("finalizeRelocation: moveStock failed for target storage=%1 qty=%2 — aborting")
                             .arg(md.toStorageId.toString()).arg(md.quantity));
                return false;
            }

            remaining -= put;
        }

        // 6) Ellenőrzés: minden célra sikeresen írtunk-e
        if (remaining != 0) {
            zWarning(QStringLiteral("finalizeRelocation: not enough deposited to targets for material %1; remaining=%2")
                         .arg(instr.materialId.toString()).arg(remaining));
            return false;
        }

        // 7) Siker: frissítjük az instr állapotát, auditálunk és persisteljük a registry-t
        instr.finalizedQuantity = instr.finalizedQuantitySoFar() + toExecute;
       // instr.isFinalized = true;

        //Auditor::instance().logFinalizeFull(instr.rowId, instr.materialId, toExecute, presenter_->currentUser());

        // moveStock belső műveletei általában hívják a presenter->update_StockEntry/remove_StockEntry,
        // de itt biztosítjuk a StockRegistry persist meghívását a végén
        //StockRegistry::instance().persist();

        StockRegistry::instance().dumpAll(); // végeredmény debug
        zInfo(QStringLiteral("finalizeRelocation succeeded for material %1 executed=%2").arg(instr.materialId.toString()).arg(toExecute));
        return true;
    }

    // Relocation finalize: minden forrás és cél a moveStock()-on keresztül megy
    // bool finalizeRelocation(RelocationInstruction& instr)
    // {
    //     zTrace();
    //     StockRegistry::instance().dumpAll(); // debug

    //     if (!instr.isReadyToFinalize() || instr.isAlreadyFinalized()) {
    //         zInfo(L("finalizeRelocation failed"));
    //         return false;
    //     }

    //     // Forrásokból levonás (entry-szintű moveStock hívások)
    //     for (const auto& src : instr.sources) {
    //         if (src.moved <= 0) continue;

    //         QUuid sourceEntryId = src.entryId;
    //         std::optional<StockEntry> opt;
    //         if (!sourceEntryId.isNull()) {
    //             opt = StockRegistry::instance().findById(sourceEntryId);
    //             if (!opt) {
    //                 zWarning(QStringLiteral("finalizeRelocation: source entry not found: %1. Will try resolve by storage+material.")
    //                              .arg(sourceEntryId.toString()));
    //             }
    //         }

    //         // Ha nincs entryId vagy nem található, próbáljuk feloldani storage+material alapján
    //         if (!opt && !src.locationId.isNull()) {
    //             // FIX: helyes sorrend: storageId, materialId
    //             opt = StockRegistry::instance().findFirstByStorageAndMaterial(src.locationId, instr.materialId);
    //             if (opt) {
    //                 zInfo(QStringLiteral("finalizeRelocation: resolved source entry from storage: %1 -> entry=%2")
    //                           .arg(src.locationId.toString()).arg(opt->entryId.toString()));
    //                 sourceEntryId = opt->entryId;
    //             } else {
    //                 zWarning(QStringLiteral("finalizeRelocation: cannot resolve source entry for storage=%1 material=%2; skipping")
    //                              .arg(src.locationId.toString()).arg(instr.materialId.toString()));
    //                 continue;
    //             }
    //         }

    //         const StockEntry sourceEntry = *opt; // safe: opt valid here

    //         // extra sanity: ensure material matches instr material
    //         if (sourceEntry.materialId != instr.materialId) {
    //             zWarning(QStringLiteral("finalizeRelocation: resolved entry material mismatch: entry=%1 material=%2 expected=%3")
    //                          .arg(sourceEntry.entryId.toString())
    //                          .arg(sourceEntry.materialId.toString())
    //                          .arg(instr.materialId.toString()));
    //             // döntés: folytatod-e vagy skippeled; itt skip
    //             continue;
    //         }

    //         MovementData md;
    //         md.fromEntryId = sourceEntry.entryId;
    //         md.materialId      = sourceEntry.materialId;
    //         md.toStorageId = QUuid(); // for consumption/relocation source step we may not have target here
    //         md.quantity    = src.moved;
    //         md.comment     = "";//QStringLiteral("Relocation: source");

    //         zInfo(QStringLiteral("Calling moveStock(source): fromEntryId=%1 qty=%2")
    //                   .arg(md.fromEntryId.toString()).arg(md.quantity));

    //         if (!moveStock(md, presenter_)) {
    //             zWarning(QStringLiteral("finalizeRelocation: moveStock failed for source entry=%1 qty=%2 — aborting finalization")
    //                          .arg(md.fromEntryId.toString()).arg(md.quantity));
    //             // Ha szeretnél: itt lehet rollback/retry logika, jelenleg abort
    //             return false;
    //         }

    //     }

    //     // Célokra felírás (deposit / aggregate hívások)
    //     for (const auto& tgt : instr.targets) {
    //         zInfo(QStringLiteral("Finalize target: locationId=%1, placed=%2")
    //                   .arg(tgt.locationId.toString()).arg(tgt.placed));

    //         if (tgt.placed <= 0) continue;

    //         MovementData md;
    //         md.fromEntryId = QUuid();          // nincs forrásentry a deposit-only lépéshez
    //         md.materialId      = instr.materialId; // material reference is required for aggregation/creation
    //         md.toStorageId = tgt.locationId;   // pontos cél storage
    //         md.quantity    = tgt.placed;
    //         md.comment     = "";//QStringLiteral("Relocation: target");

    //         zInfo(QStringLiteral("Calling moveStock(target): toStorageId=%1 qty=%2")
    //                   .arg(md.toStorageId.toString()).arg(md.quantity));

    //         if (!moveStock(md, presenter_)) {
    //             zWarning(QStringLiteral("finalizeRelocation: moveStock failed for target storage=%1 qty=%2 — aborting")
    //                          .arg(md.toStorageId.toString()).arg(md.quantity));
    //             return false;
    //         }

    //     }

    //     // Instr frissítése
    //     instr.executedQuantity = instr.plannedQuantity;
    //     instr.isFinalized = true;

    //     StockRegistry::instance().dumpAll(); // debug
    //     zInfo(L("finalizeRelocation suceeded"));
    //     return true;
    // }


    // 🔹 Forrás frissítése vagy törlése
    static bool updateOrRemoveSource(const StockEntry& original,
                                     int remaining,
                                     CuttingPresenter* presenter)
    {
        zTrace();

        if (original.entryId.isNull()) {
            zWarning(QStringLiteral("updateOrRemoveSource: original.entryId is NULL for storage=%1 material=%2")
                         .arg(original.storageId.toString(), original.materialId.toString()));
            return false;
        }

        if (remaining < 0) {
            zWarning(QStringLiteral("updateOrRemoveSource: negative remaining=%1 for entry=%2")
                         .arg(remaining).arg(original.entryId.toString()));
            return false;
        }

        const auto* storage = StorageRegistry::instance().findById(original.storageId);
        const auto* material = MaterialRegistry::instance().findById(original.materialId);
        QString storageName = storage ? storage->name : original.storageId.toString();
        QString materialName = material ? material->name : original.materialId.toString();

        if (remaining > 0) {
            StockEntry updated = original;
            int oldQty = updated.quantity;
            updated.quantity = remaining;

            zInfo(QStringLiteral("📦 Forrás frissítése: entry=%1 storage=%2 material=%3 old=%4 new=%5")
                      .arg(updated.entryId.toString())
                      .arg(storageName)
                      .arg(materialName)
                      .arg(oldQty)
                      .arg(updated.quantity));

            // Presenter felé update kérés; ha a presenter aszinkron, később ellenőrizd a registry-t
            presenter->update_StockEntry(updated);
            return true;
        } else {
            zInfo(QStringLiteral("🗑️ Forrás sor törlése: entry=%1 storage=%2 material=%3")
                      .arg(original.entryId.toString()).arg(storageName).arg(materialName));

            presenter->remove_StockEntry(original.entryId);
            return true;
        }
    }

    // visszatérési típus: használt vagy létrehozott entryId, különben std::nullopt
    static std::optional<QUuid> ensureTargetEntry(const QUuid& storageId,
                                                  const QUuid& materialId,
                                                  CuttingPresenter* presenter)
    {
        zTrace();

        if (storageId.isNull() || materialId.isNull()) {
            zWarning(QStringLiteral("ensureTargetEntry: invalid params storage=%1 material=%2")
                         .arg(storageId.toString(), materialId.toString()));
            return std::nullopt;
        }

        // 1) próbáljuk meg találni
        if (auto existing = StockRegistry::instance().findByMaterialAndStorage(materialId, storageId)) {
            return existing->entryId;
        }

        // 2) létrehozás via presenter
        StockEntry newEntry;
        newEntry.entryId    = QUuid::createUuid();
        newEntry.storageId  = storageId;
        newEntry.materialId = materialId;
        newEntry.quantity   = 0;
        newEntry.comment    = QStringLiteral("Auto-created target entry (ensureTargetEntry)");

        presenter->add_StockEntry(newEntry);

        // 3) újrapróbáljuk a lekérést (másik eljárás közben létrehozva lehetett)
        if (auto again = StockRegistry::instance().findById(newEntry.entryId)) {
            return again->entryId;
        }
        if (auto byPair = StockRegistry::instance().findByMaterialAndStorage(materialId, storageId)) {
            return byPair->entryId;
        }

        zWarning(QStringLiteral("ensureTargetEntry: failed to create/find entry for storage=%1 material=%2")
                     .arg(storageId.toString(), materialId.toString()));
        return std::nullopt;
    }


    // addOrAggregateTarget_Storage — nincs lokális fallback. Ha nincs meglévő entry,
    // ensureTargetEntry-t hívjuk; ha az sem ad vissza entryId-t, a függvény sikertelen.
    // Siker esetén a használt entryId-t adjuk vissza.
    static std::optional<QUuid> addOrAggregateTarget_Storage(const QUuid& materialId,
                                                             const QUuid& toStorageId,
                                                             int quantity,
                                                             const QString& comment,
                                                             CuttingPresenter* presenter)
    {
        zTrace();

        if (materialId.isNull()) {
            zWarning(QStringLiteral("addOrAggregateTarget_Storage: missing materialId"));
            return std::nullopt;
        }
        if (toStorageId.isNull()) {
            zWarning(QStringLiteral("addOrAggregateTarget_Storage: missing toStorageId"));
            return std::nullopt;
        }
        if (quantity == 0) {
            zWarning(QStringLiteral("addOrAggregateTarget_Storage: no quantity"));
            return std::nullopt;
        }

        const auto* toStorage = StorageRegistry::instance().findById(toStorageId);
        const auto* material  = MaterialRegistry::instance().findById(materialId);
        QString toStorageName  = toStorage ? toStorage->name : toStorageId.toString();
        QString materialName   = material ? material->name : materialId.toString();

        zInfo(QStringLiteral("addOrAggregateTarget_Storage: material=%1 storage=%2 qty=%3")
                  .arg(materialName).arg(toStorageName).arg(quantity));

        // 1) Ha van meglévő entry material+storage alapján, delegáljunk entry-aggregálásra
        if (auto existing = StockRegistry::instance().findByMaterialAndStorage(materialId, toStorageId)) {
            if (auto usedId = addOrAggregateTarget_Entry(materialId, existing->entryId, quantity, comment, presenter)) {
                return usedId;
            }
            return std::nullopt;
        }

        // 2) Nincs meglévő entry — próbáljuk meg atomikusan biztosítani az entry-t (ensureTargetEntry)
        //    Ha ensureTargetEntry sikeres, delegáljuk az aggregálást az Entry-variánsra.
        auto ensured = ensureTargetEntry(toStorageId, materialId, presenter);
        if (!ensured) {
            zWarning(QStringLiteral("addOrAggregateTarget_Storage: ensureTargetEntry failed for storage=%1 material=%2")
                         .arg(toStorageId.toString(), materialId.toString()));
            return std::nullopt;
        }

        // ensured tartalmazza a használható entryId-t (meglévő vagy frissen létrehozott)
        if (auto usedId = addOrAggregateTarget_Entry(materialId, *ensured, quantity, comment, presenter)) {
            return usedId;
        }

        zWarning(QStringLiteral("addOrAggregateTarget_Storage: addOrAggregateTarget_Entry failed for entry=%1")
                     .arg(ensured->toString()));
        return std::nullopt;
    }




    // visszatérési érték: használható entryId vagy nullopt
    static std::optional<QUuid> addOrAggregateTarget_Entry(const QUuid& materialId,
                                                           const QUuid& toEntryId,
                                                           int quantity,
                                                           const QString& comment,
                                                           CuttingPresenter* presenter)
    {
        zTrace();

        if (materialId.isNull()) {
            zWarning(QStringLiteral("addOrAggregateTarget_Entry: missing materialId"));
            return std::nullopt;
        }
        if (toEntryId.isNull()) {
            zWarning(QStringLiteral("addOrAggregateTarget_Entry: missing toEntryId"));
            return std::nullopt;
        }
        if (quantity == 0) {
            zWarning(QStringLiteral("addOrAggregateTarget_Entry: no quantity"));
            return std::nullopt;
        }

        if (auto toEntry = StockRegistry::instance().findById(toEntryId)) {
            StockEntry updated = *toEntry;
            int before = updated.quantity;
            updated.quantity += quantity;
            if (!comment.isEmpty()) {
                if (!updated.comment.isEmpty()) updated.comment += "\n";
                updated.comment += comment;
            }

            presenter->update_StockEntry(updated);

            zInfo(QStringLiteral("✅ Aggregálás[entry]: entry=%1 %2→%3")
                      .arg(updated.entryId.toString()).arg(before).arg(updated.quantity));
            return updated.entryId;
        }

        zInfo(QStringLiteral("addOrAggregateTarget_Entry: explicit toEntryId not found: %1").arg(toEntryId.toString()));
        return std::nullopt;
    }



    // Bemenet: materialId - mely anyag; fromStorageId - honnan; totalToMove - összesen levonandó; toStorageId - cél storage (nullable)
    // Visszatér: true ha sikerült teljesen elosztani és hívni moveStock‑ot minden szükséges entry­re; false ha bármi hiba történt.
    bool distributeAndMove(const QUuid& materialId,
                           const QUuid& fromStorageId,
                           int totalToMove,
                           const QUuid& toStorageId,
                           CuttingPresenter* presenter)
    {
        if (totalToMove <= 0) {
            zInfo(QStringLiteral("distributeAndMove: nothing to move (totalToMove=%1)").arg(totalToMove));
            return true;
        }

        // Lekérjük az összes entryt a storage-ból, rendezve: legkisebb darabszám először
        auto candidates = StockRegistry::instance().findAllByMaterialAndStorageSorted(materialId, fromStorageId);
        zInfo(QStringLiteral("DISTRIBUTE START material=%1 from=%2 to=%3 total=%4 candidates=%5")
                  .arg(materialId.toString(), fromStorageId.toString(), toStorageId.toString())
                  .arg(totalToMove).arg(candidates.size()));

        int remaining = totalToMove;

        for (const auto& entry : candidates) {
            if (remaining <= 0) break;

            // Ha nincs készlet az entryben, ugrunk
            if (entry.quantity <= 0) continue;

            int take = std::min(entry.quantity, remaining);

            // Felkészítjük a MovementData-t — fromEntryId authoritative
            MovementData md;
            md.fromEntryId = entry.entryId;
            md.toStorageId = toStorageId;
            md.materialId = materialId;
            md.quantity = take;

            zInfo(QStringLiteral("DISTRIBUTE STEP: entry=%1 qtyBefore=%2 take=%3 remainingBefore=%4")
                      .arg(entry.entryId.toString()).arg(entry.quantity).arg(take).arg(remaining));

            bool ok = moveStock(md, presenter);
            if (!ok) {
                zWarning(QStringLiteral("DISTRIBUTE: moveStock failed for entry=%1 take=%2. Aborting.")
                             .arg(entry.entryId.toString()).arg(take));
                return false;
            }

            remaining -= take;
        }

        if (remaining > 0) {
            zWarning(QStringLiteral("DISTRIBUTE FAILED: insufficient stock in storage=%1 for material=%2 missing=%3")
                         .arg(fromStorageId.toString()).arg(materialId.toString()).arg(remaining));
            return false;
        }

        zInfo(QStringLiteral("DISTRIBUTE DONE: material=%1 from=%2 to=%3 totalMoved=%4")
                  .arg(materialId.toString(), fromStorageId.toString(), toStorageId.toString()).arg(totalToMove));
        return true;
    }

    bool distributeAcrossSources(const QUuid& materialId,
                                 const QVector<std::pair<QUuid,int>>& fromStorageAndQty, // pair(storageId, qty)
                                 const QUuid& toStorageId,
                                 CuttingPresenter* presenter)
    {
        for (const auto& p : fromStorageAndQty) {
            const QUuid fromStorage = p.first;
            int qty = p.second;
            if (!distributeAndMove(materialId, fromStorage, qty, toStorageId, presenter))
                return false; // abort on first failure
        }
        return true;
    }

    static bool consumeFromEntry(int quantity, StockEntry* sourceEntry, CuttingPresenter* presenter)
    {
        zTrace();

        if (!sourceEntry) {
            zWarning(QStringLiteral("consumeFromEntry: source entry not found"));
            return false;
        }

        if (quantity <= 0) {
            zWarning(QStringLiteral("consumeFromEntry: invalid quantity=%1").arg(quantity));
            return false;
        }

        if (quantity > sourceEntry->quantity) {
            zWarning(QStringLiteral("consumeFromEntry: insufficient qty in source entry %1 (have=%2 need=%3)")
                         .arg(sourceEntry->entryId.toString()).arg(sourceEntry->quantity).arg(quantity));
            return false;
        }

        int remaining = sourceEntry->quantity - quantity;

        // updateOrRemoveSource feltételezett viselkedése: ha remaining==0 -> remove, egyébként update
        updateOrRemoveSource(*sourceEntry, remaining, presenter);

        zInfo(QStringLiteral("➖ consumeFromEntry: %1 units deducted from entry=%2 (before=%3 remaining=%4)")
                  .arg(quantity).arg(sourceEntry->entryId.toString()).arg(sourceEntry->quantity).arg(remaining));

        return true;
    }


    // Fő logika
    static bool moveStock(const MovementData& data, CuttingPresenter* presenter)
    {
        zInfo(QStringLiteral("🔍 moveStock called: itemId=%1 fromEntryId=%2 toStorageId=%3 toEntryId=%4 qty=%5")
                  .arg(data.materialId.toString(), data.fromEntryId.toString(), data.toStorageId.toString(), data.toEntryId.toString())
                  .arg(data.quantity));

        if (data.quantity <= 0) {
            zWarning(QStringLiteral("moveStock: invalid quantity=%1").arg(data.quantity));
            return false;
        }

        const bool hasSource = !data.fromEntryId.isNull();
        const bool hasTarget = !data.toStorageId.isNull() || !data.toEntryId.isNull();

        if (!hasSource && !hasTarget) {
            zWarning("moveStock: neither source nor target provided");
            return false;
        }

        // Betöltjük a forrásentry-t, ha van
        std::optional<StockEntry> sourceOpt;
        if (hasSource) {
            sourceOpt = StockRegistry::instance().findById(data.fromEntryId);
            if (!sourceOpt) {
                zWarning(QStringLiteral("moveStock: source entry not found: %1").arg(data.fromEntryId.toString()));
                return false;
            }
        }

        // Próbáljuk betölteni explicit célentry-t (ha van)
        std::optional<StockEntry> targetOpt;
        if (!data.toEntryId.isNull()) {
            targetOpt = StockRegistry::instance().findById(data.toEntryId);
            if (!targetOpt) {
                zInfo(QStringLiteral("moveStock: toEntryId provided but not found: %1 (will try storage aggregation/create)")
                          .arg(data.toEntryId.toString()));
            }
        }

        // Logginghoz: storage objektumok
        const StorageEntry* srcStorage = nullptr;
        const StorageEntry* dstStorage = nullptr;
        if (sourceOpt) srcStorage = StorageRegistry::instance().findById(sourceOpt->storageId);
        if (!data.toStorageId.isNull()) dstStorage = StorageRegistry::instance().findById(data.toStorageId);

        if (sourceOpt && dstStorage && srcStorage && srcStorage->id == dstStorage->id) {
            zInfo("moveStock: source and destination storage are identical -> skipping");
            return false;
        }

        std::optional<QUuid> usedTargetId; // ha célra írtunk, ide kerül az entryId

        // --- Forrás + cél (klasszikus áthelyezés)
        if (hasSource && hasTarget) {
            StockEntry* sourceEntry = sourceOpt ? &*sourceOpt : nullptr;
            if (!sourceEntry) {
                zWarning("moveStock: source unexpectedly missing");
                return false;
            }

            // ellenőrzés: elég a mennyiség
            if (data.quantity > sourceEntry->quantity) {
                zWarning(QStringLiteral("moveStock: insufficient qty in source entry %1 (have=%2 need=%3)")
                             .arg(sourceEntry->entryId.toString()).arg(sourceEntry->quantity).arg(data.quantity));
                return false;
            }

            // Levonás: data.quantity
            if (!consumeFromEntry(data.quantity, sourceEntry, presenter)) {
                zWarning(QStringLiteral("moveStock: consumeFromEntry failed for entry=%1").arg(sourceEntry->entryId.toString()));
                return false;
            }

            // Célra írás — ha explicit targetOpt van, használjuk az entry-variánst; különben storage-variáns
            if (targetOpt) {
                usedTargetId = addOrAggregateTarget_Entry(data.materialId, targetOpt->entryId, data.quantity, data.comment, presenter);
                if (!usedTargetId) {
                    zWarning(QStringLiteral("moveStock: addOrAggregateTarget_Entry failed for entry=%1").arg(targetOpt->entryId.toString()));
                    return false;
                }
            } else {
                // ha nincs explicit entry, delegálunk storage alapú aggregálásra, ami ensureTargetEntry-t használ
                usedTargetId = addOrAggregateTarget_Storage(data.materialId, data.toStorageId, data.quantity, data.comment, presenter);
                if (!usedTargetId) {
                    zWarning(QStringLiteral("moveStock: addOrAggregateTarget_Storage failed for storage=%1 material=%2")
                                 .arg(data.toStorageId.toString(), data.materialId.toString()));
                    return false;
                }
            }
        }
        // --- Csak levonás (consume)
        else if (hasSource) {
            StockEntry* sourceEntry = sourceOpt ? &*sourceOpt : nullptr;
            if (!sourceEntry) {
                zWarning("moveStock: source unexpectedly missing on consume-only path");
                return false;
            }

            if (!consumeFromEntry(data.quantity, sourceEntry, presenter)) {
                zWarning(QStringLiteral("moveStock: consumeFromEntry failed for entry=%1").arg(sourceEntry->entryId.toString()));
                return false;
            }

            // Logoljuk a fogyasztást (fromEntryId kitöltése)
            MovementData md = data;
            md.fromEntryId = sourceEntry->entryId;
            MovementLogger::log(MovementLogModel{md});
            zInfo(QStringLiteral("➖ Fogyasztás: %1 db levonva entry=%2").arg(md.quantity).arg(md.fromEntryId.toString()));
        }
        // --- Csak betét (deposit)
        else if (hasTarget) {
            // Ha van explicit célentry objektum
            if (targetOpt) {
                usedTargetId = addOrAggregateTarget_Entry(data.materialId, targetOpt->entryId, data.quantity, data.comment, presenter);
                if (!usedTargetId) {
                    zWarning(QStringLiteral("moveStock: addOrAggregateTarget_Entry failed for explicit entry=%1").arg(targetOpt->entryId.toString()));
                    return false;
                }
            } else {
                // storage alapú aggregálás (ensureTargetEntry belül lesz hívva)
                usedTargetId = addOrAggregateTarget_Storage(data.materialId, data.toStorageId, data.quantity, data.comment, presenter);
                if (!usedTargetId) {
                    zWarning(QStringLiteral("moveStock: addOrAggregateTarget_Storage failed for storage=%1 material=%2")
                                 .arg(data.toStorageId.toString(), data.materialId.toString()));
                    return false;
                }
            }
        }

        // Általános logolás: build MovementData a végső from/to entryId-kkel és loggolás
        MovementData md = data;
        if (sourceOpt) md.fromEntryId = sourceOpt->entryId;
        if (usedTargetId) md.toEntryId = *usedTargetId;

        // opcionális: kiegészítő human mezők kitöltése (srcStorage/dstStorage/itemName)
        MovementLogger::log(MovementLogModel{md});

        QString fromName = srcStorage ? srcStorage->name : QStringLiteral("—");
        QString toName   = dstStorage ? dstStorage->name : QStringLiteral("—");
        QString itemName = MaterialRegistry::instance().findById(md.materialId)
                               ? MaterialRegistry::instance().findById(md.materialId)->name : md.materialId.toString();

        if (hasSource && hasTarget) {
            zInfo(QStringLiteral("📦 Mozgatás befejezve: %1 db %2 áthelyezve %3 → %4")
                      .arg(md.quantity).arg(itemName).arg(fromName).arg(toName));
        } else if (hasSource) {
            zInfo(QStringLiteral("➖ Fogyasztás: %1 db %2 levonva a(z) %3 készletből")
                      .arg(md.quantity).arg(itemName).arg(fromName));
        } else if (hasTarget) {
            zInfo(QStringLiteral("➕ Betét: %1 db %2 hozzáadva a(z) %3 készlethez")
                      .arg(md.quantity).arg(itemName).arg(toName));
        }

        return true;
    }




private:
    CuttingPresenter* presenter_ = nullptr;
};
