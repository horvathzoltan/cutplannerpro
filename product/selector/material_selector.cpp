// #include "material_selector.h"

// MaterialSelector::Result
// MaterialSelector::select(
//     const QUuid& productTypeId,
//     const QUuid& productSubtypeId,
//     MaterialFamily family,
//     const QString& desiredColor,
//     double requiredLength
//     )
// {
//     MaterialSelector::Result out;

//     // 1) Prefixek lekérése a rolemapből
//     auto prefixes = MaterialRoleRegistry::instance().prefixesFor(
//         productTypeId, productSubtypeId, family);

//     if (prefixes.isEmpty()) {
//         out.reason = "❌ Nincs rolemap prefix ehhez a family-hez";
//         return out;
//     }

//     // 2) Prefixek alapján anyagok összegyűjtése
//     QVector<const MaterialMaster*> candidates;

//     for (const auto& px : prefixes) {
//         auto list = MaterialRegistry::instance().findByPrefix(px);
//         for (auto* m : list)
//             candidates.append(m);
//     }

//     if (candidates.isEmpty()) {
//         out.reason = "❌ Nincs anyag a prefixek alapján";
//         return out;
//     }

//     // 3) Szín szerinti szűrés (pontos egyezés)
//     QVector<const MaterialMaster*> colorMatch;
//     for (auto* m : candidates) {
//         if (m->color.compare(desiredColor, Qt::CaseInsensitive) == 0)
//             colorMatch.append(m);
//     }

//     QVector<const MaterialMaster*> finalList =
//         colorMatch.isEmpty() ? candidates : colorMatch;

//     // 4) Hossz szerinti szűrés (legalább requiredLength)
//     QVector<const MaterialMaster*> lengthMatch;
//     for (auto* m : finalList) {
//         if (m->stockLength >= requiredLength)
//             lengthMatch.append(m);
//     }

//     if (lengthMatch.isEmpty()) {
//         out.reason = "❌ Nincs elég hosszú anyag";
//         return out;
//     }

//     // 5) Leghosszabb szál kiválasztása
//     const MaterialMaster* best = nullptr;
//     double bestLen = -1;

//     for (auto* m : lengthMatch) {
//         if (m->stockLength > bestLen) {
//             bestLen = m->stockLength;
//             best = m;
//         }
//     }

//     out.material = best;
//     out.reason = "✅ Anyag kiválasztva prefix + szín + hossz alapján";
//     return out;
// }
