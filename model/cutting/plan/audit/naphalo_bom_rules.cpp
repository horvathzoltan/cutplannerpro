#include "naphalo_bom_rules.h"
#include "naphalo_material_aggregator.h"
#include "naphalo_type_detector.h"
#include "naphalo_prefix_utils.h"

NaphaloBomAuditResult NaphaloBomRules::check(const QVector<Cutting::Plan::Request>& list)
{
    NaphaloBomAuditResult r;

    auto materials = NaphaloMaterialAggregator::aggregate(list);

    // Prefix-alapú darabszámok

    r.labCount       = NaphaloMaterialAggregator::sumGroup(materials, NaphaloGroup::Lab) * 2;
    r.labbetetCount  = NaphaloMaterialAggregator::sumGroup(materials, NaphaloGroup::LabBetet) * 2;
    r.tokCount       = NaphaloMaterialAggregator::sumGroup(materials, NaphaloGroup::Tok);
    r.tokFedCount    = NaphaloMaterialAggregator::sumGroup(materials, NaphaloGroup::TokFed);
    r.czCount        = NaphaloMaterialAggregator::sumGroup(materials, NaphaloGroup::CipzarosZaro);
    r.szCount        = NaphaloMaterialAggregator::sumGroup(materials, NaphaloGroup::SinesZaro);

    int tokFamily = NaphaloMaterialAggregator::sumFamily(materials, NaphaloFamily::Tok);
    int labFamily = NaphaloMaterialAggregator::sumFamily(materials, NaphaloFamily::Lab);
    int zaroFamily = NaphaloMaterialAggregator::sumFamily(materials, NaphaloFamily::Zaro);

    if (tokFamily == 0)
        r.messages << "⚠️ Nincs tokcsalád, de mégis vannak lábak vagy zárók!";

    if (r.isCipzaros && zaroFamily != r.tokCount)
        r.messages << "⚠️ CIPZÁROS: zárócsalád darabszám eltér a tokcsaládtól!";

    if (r.isSines && zaroFamily != r.tokCount)
        r.messages << "⚠️ SÍNES: zárócsalád darabszám eltér a tokcsaládtól!";


    // Típusdetektálás – egységesítve
    auto type = NaphaloTypeDetector::detect(list);

    r.isCipzaros = (type == NaphaloType::Cipzaros);
    r.isSines    = (type == NaphaloType::Sines);
    r.isBowdenes = (type == NaphaloType::Bowdenes);

    if (type == NaphaloType::Unknown) {
        r.messages << "⚠️ Típus: ISMERETLEN / HIÁNYOS ADAT";
        r.hasError = true;
        return r;
    }

    // Tok–tokfedél ellenőrzés
    if (r.tokCount != r.tokFedCount) {
        r.messages << QString("⚠️ ELTÉRÉS: Tok (%1) és tokfedél (%2) darabszáma nem egyezik!")
                          .arg(r.tokCount).arg(r.tokFedCount);
        r.hasError = true;
    }

    // CIPZÁROS BOM szabályok
    if (r.isCipzaros) {
        int expectedLab = r.tokCount * 2;
        int expectedLabBet = r.tokCount * 2;
        int expectedCZ = r.tokCount;

        if (r.labCount != expectedLab)
            r.messages << QString("⚠️ CIPZÁROS: Láb (%1) != elvárt (%2)").arg(r.labCount).arg(expectedLab);

        if (r.labbetetCount != expectedLabBet)
            r.messages << QString("⚠️ CIPZÁROS: Lábbetét (%1) != elvárt (%2)").arg(r.labbetetCount).arg(expectedLabBet);

        if (r.czCount != expectedCZ)
            r.messages << QString("⚠️ CIPZÁROS: Záró (%1) != elvárt (%2)").arg(r.czCount).arg(expectedCZ);

        r.hasError |= (r.labCount != expectedLab || r.labbetetCount != expectedLabBet || r.czCount != expectedCZ);
    }

    // SÍNES BOM szabályok
    if (r.isSines) {
        int expectedLab = r.tokCount * 2;
        int expectedLabBet = r.tokCount * 2;
        int expectedSZ = r.tokCount;

        if (r.labCount != expectedLab)
            r.messages << QString("⚠️ SINES: Láb (%1) != elvárt (%2)").arg(r.labCount).arg(expectedLab);

        if (r.labbetetCount != expectedLabBet)
            r.messages << QString("⚠️ SINES: Lábbetét (%1) != elvárt (%2)").arg(r.labbetetCount).arg(expectedLabBet);

        if (r.szCount != expectedSZ)
            r.messages << QString("⚠️ SINES: Záró (%1) != elvárt (%2)").arg(r.szCount).arg(expectedSZ);

        r.hasError |= (r.labCount != expectedLab || r.labbetetCount != expectedLabBet || r.szCount != expectedSZ);
    }

    // BOWDENES BOM szabályok
    if (r.isBowdenes) {
        if (r.labCount > 0)
            r.messages << "⚠️ BOWDENES: Láb nem lehet, de van!";

        if (r.labbetetCount > 0)
            r.messages << "⚠️ BOWDENES: Lábbetét nem lehet, de van!";

        if (r.szCount != r.tokCount)
            r.messages << QString("⚠️ BOWDENES: Sines záró (%1) != tok (%2)").arg(r.szCount).arg(r.tokCount);

        r.hasError |= (r.labCount > 0 || r.labbetetCount > 0 || r.szCount != r.tokCount);
    }

    return r;
}
