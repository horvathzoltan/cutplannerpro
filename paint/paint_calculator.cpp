#include "paint_calculator.h"

#include <model/registries/cuttingplanrequestregistry.h>
#include <materials/registry/material_registry.h>
#include <materials/model/material_family_utils.h>
#include <paint/registry/powder_consumption_registry.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

const QUuid PaintCalculator::CL_COMPOSITE_ID = QUuid::fromString("{00000000-0000-0000-0000-00000000CL27}");

PaintPlan PaintCalculator::buildPlan()
{
    PaintPlan plan;

    // 1) Minden request beolvasása
    const auto all = CuttingPlanRequestRegistry::instance().readAll();

    // 2) Csoportosítás tételszám szerint
    QHash<QString, QVector<Cutting::Plan::Request>> groups;
    for (const auto& req : all) {
        QString ref = req.externalReference.trimmed();
        if (ref.isEmpty())
            ref = "<NINCS_TETELSZAM>";
        groups[ref].append(req);
    }

    // 3) Minden tételszám-csoport feldolgozása
    for (auto it = groups.begin(); it != groups.end(); ++it)
    {
        const auto& list = it.value();

        // --- TÍPUSDETEKTÁLÁS ---
        //NaphaloType type = NaphaloTypeDetector::detect(list);

        // --- A CSOPORT ÖSSZES REQUESTJÉNEK FELDOLGOZÁSA ---
        for (const auto& req : list)
        {
            QString colorCode = req.requiredColor.code();
            if (colorCode.isEmpty())
                colorCode = QStringLiteral("Nincs szín megadva");

            auto& colorGroup = plan.byColor[colorCode];
            colorGroup.color = req.requiredColor;
            colorGroup.pofaFestheto = false;
            colorGroup.csavarFestheto = false;
            // Anyag lekérése
            const MaterialMaster* mat = MaterialRegistry::instance().findById(req.materialId);

            // 0) Ha nincs anyag → kihagyjuk
            if(!mat)
                continue;

            // 1) Ha nem festhető → kihagyjuk
            if (mat->paintingMode == PaintingMode::None)
                continue;

            // 2) Ha a request színe és felülete megegyezik az anyag saját színével → kihagyjuk
            bool sameColor = (req.requiredColor.code() == mat->color.code());
            //bool sameSurface = (req.surface == mat->surface);

            // if(req.requiredColor.code().contains("9010")){
            //     zInfo("FEHÉR");
            //     }

            // if (sameColor && sameSurface)
            //     continue;

            if (sameColor)
                continue;

            QString barcode = mat->barcode;

            bool isCL  = MaterialFamilyUtils::matchPrefix(barcode, "NP-CL*");
            bool isCLT = MaterialFamilyUtils::matchPrefix(barcode, "NP-CLT*");


            // Szorzó (CL/SL → 2)
            int szorzo = 1;
            if (MaterialFamilyUtils::matchPrefix(barcode, "NP-CL*")
                || MaterialFamilyUtils::matchPrefix(barcode,"NP-SL*")
                || MaterialFamilyUtils::matchPrefix(barcode, "NP-CLT*"))
                szorzo = 2;

            // --- REFRAKTORÁLT KOMPOZIT LOGIKA ---
            QUuid targetMaterialId = req.materialId;
            bool shouldAddLength   = true;

            if (isCL || isCLT)
            {
                targetMaterialId = CL_COMPOSITE_ID;

                if (isCLT)
                    shouldAddLength = false;   // CLT nem növeli a festési hosszt
            }

            // ANYAG AGGREGÁLÁS (egységes logika)
            auto& summary = colorGroup.materials[targetMaterialId];
            summary.materialId = targetMaterialId;

            // darabszám mindig nő (CLT is tartozik a lábhoz)
            summary.totalPieces += req.quantity * szorzo;
            summary.requestIds.append(req.requestId);

            // hossz csak CL esetén nő
            if (shouldAddLength)
                summary.totalLength_mm += req.quantity * req.requiredLength * szorzo;

            // --- PORFOGYÁS SZÁMÍTÁSA ---
            double meters = 0.0;
            if (shouldAddLength)
                meters = (req.requiredLength * szorzo) / 1000.0;

            auto model = PowderConsumptionRegistry::instance().find(req.productTypeId,
                                                                    req.productSubtypeId);

            double kgPerMeter = model.kgPerMeterCorrected();
            double kg = meters * kgPerMeter * req.quantity;

            summary.powderKg += kg;
            colorGroup.powderKg += kg;

            // --- NP-CL / NP-CLT összevonása egy festési egységgé ---
            // if (isCL || isCLT)
            // {
            //     auto& summary = colorGroup.materials[CL_COMPOSITE_ID];
            //     summary.materialId = CL_COMPOSITE_ID;

            //     // --- NP-CL: festendő hossz ---
            //     if (isCL)
            //     {
            //         summary.totalPieces += req.quantity * szorzo;
            //         summary.totalLength_mm += req.quantity * req.requiredLength * szorzo;
            //         summary.requestIds.append(req.requestId);

            //         // FESTÉSI NORMA SZÁMÍTÁSA
            //         double meters = (req.requiredLength * szorzo) / 1000.0;

            //         auto model = PowderConsumptionRegistry::instance().find(req.productTypeId,
            //                                                                 req.productSubtypeId);

            //         double kgPerMeter = model.kgPerMeterCorrected();
            //         double kg = meters * kgPerMeter * req.quantity;

            //         summary.powderKg += kg;
            //         colorGroup.powderKg += kg;
            //     }

            //     // --- NP-CLT: csak jelölés, nem növeli a festési hosszt ---
            //     if (isCLT)
            //     {
            //         summary.hasCLT = true;
            //         summary.requestIds.append(req.requestId); // CLT is tartozik a lábhoz
            //     }

            //     continue; // ❗ CL és CLT nem mehet tovább a normál ágra
            // }


            // ANYAG AGGREGÁLÁS
            // PaintMaterialSummary &summary = colorGroup.materials[req.materialId];
            // summary.materialId = req.materialId;
            // summary.totalPieces += req.quantity * szorzo;
            // summary.totalLength_mm += req.quantity * req.requiredLength * szorzo;
            // summary.requestIds.append(req.requestId);

            // // --- PORFOGYÁS SZÁMÍTÁSA ---
            // double meters = (req.requiredLength * szorzo) / 1000.0;

            // auto typeId    = req.productTypeId;
            // auto subtypeId = req.productSubtypeId;

            // auto model = PowderConsumptionRegistry::instance().find(typeId, subtypeId);
            // double kgPerMeter = model.kgPerMeterCorrected();

            // double kg = meters * kgPerMeter * req.quantity;

            // // anyag szintű összegzés
            // summary.powderKg += kg;

            // // színcsoport szintű összegzés
            // colorGroup.powderKg += kg;

            // --- POFA / CSAVAR TÍPUS SZERINT ---
            if (MaterialFamilyUtils::matchPrefix(barcode, "NP-T*")) {
                colorGroup.pofaFestheto = true;

                int pofaDbForThisRequest = 0;

                auto* t = ProductTypeRegistry::instance().findById(req.productTypeId);
                if(t && t->code == "NP"){
                    auto* s = ProductSubtypeRegistry::instance().findById(req.productSubtypeId);
                    if(s){
                        if(s->code == "CIP")
                        {
                            pofaDbForThisRequest = 2*req.quantity;
                            colorGroup.cipzarosPofa += pofaDbForThisRequest;
                        }
                        else if(s->code =="SIN")
                        {
                            pofaDbForThisRequest = 2*req.quantity;
                            colorGroup.sinesPofa += pofaDbForThisRequest;
                        }
                        else if (s->code == "BOW")
                        {
                            pofaDbForThisRequest = 2*req.quantity;
                            colorGroup.bowdenesPofa += pofaDbForThisRequest;
                        } else{
                            zInfo("ismeretlen napháló típus:" + s->code);
                        }

                    }
                }


                // --- POFA PORFOGYÁS SZÁMÍTÁSA ---

                //zInfo(L("tetelszam:")+req.externalReference+", totalPofaDb:"+QString::number(pofaDbForThisRequest));
                if (pofaDbForThisRequest > 0)
                {
                    // pofa hossza méterben
                    double meters = POFA_LENGTH_MM / 1000.0;

                    // festési norma lekérése a tok (NP-T) alapján
                    auto model = PowderConsumptionRegistry::instance().find(req.productTypeId,
                                                                            req.productSubtypeId);

                    double kgPerMeter = model.kgPerMeterCorrected();

                    // teljes pofa porfogyás
                    double kg = meters * kgPerMeter * pofaDbForThisRequest;

                    // színcsoport szintű összegzés
                    colorGroup.powderKg += kg;
                    colorGroup.pofaPowderKg += kg;
                }
            }
            else if (MaterialFamilyUtils::matchPrefix(barcode, "NP-TF*"))
            {
                colorGroup.csavarFestheto = true;
                colorGroup.csavar +=2*req.quantity;
            }
        }
    }

    return plan;
}