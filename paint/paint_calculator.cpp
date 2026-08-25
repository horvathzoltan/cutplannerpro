#include "paint_calculator.h"
#include "materialbundles/model/bundle_definition.h"

#include <model/registries/cuttingplanrequestregistry.h>
#include <materials/registry/material_registry.h>
#include <materials/model/material_family_utils.h>
#include <paint/registry/powder_consumption_registry.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>
#include <materialbundles/registry/bundle_registry.h>

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
            const MaterialMaster* mat = MaterialRegistry::instance().findById(req.materialId);

            // 0) Ha nincs anyag → kihagyjuk
            if(!mat)
                continue;

            // 1) Ha nem festhető → kihagyjuk
            if (mat->paintingMode == PaintingMode::None)
                continue;

            // 2) Ha a request színe és felülete megegyezik az anyag saját színével → kihagyjuk
            bool sameColor = (req.requiredColor.code() == mat->color.code());
            bool sameSurface = (req.surface == mat->surface || req.surface == SurfaceType::Unknown);

            // if(req.requiredColor.code().contains("9010")){
            //     zInfo("FEHÉR");
            //     }

            if (sameColor && sameSurface)
                 continue;

            //if (sameColor)
            //    continue;

            QString colorCode;

            NamedColor nc;
            SurfaceType sf = SurfaceType::Unknown;
            if(req.requiredColor.system() != RalSystem::Unknown){
                colorCode = req.requiredColor.code();
                nc = req.requiredColor;
            } else{
                auto [colorPart, surfacePart] = SurfaceTypeUtils::extractSurface(req.requiredColor.code());
                nc = NamedColor::fromUserInput(colorPart);
                colorCode = nc.code();
                sf = surfacePart;
            }

            if(req.surface != SurfaceType::Unknown){
                sf = req.surface;
            }

            if (colorCode.isEmpty()){
                zInfo("Nincs szín megadva");
                continue;
            }

            if (sf == SurfaceType::Unknown){
                zInfo("Nincs felület megadva"); // ettől még lehet festeni!
            }

            QString groupCode = (sf != SurfaceType::Unknown)
                ?colorCode +"_"+ SurfaceTypeUtils::toCode(sf):
                colorCode;

            auto& colorGroup = plan.byColor[groupCode];
            colorGroup.color = nc;
            colorGroup.surface = sf;
            colorGroup.pofaFestheto = false;
            colorGroup.csavarFestheto = false;

            QString barcode = mat->barcode;

           // // bool isCL  = MaterialFamilyUtils::matchPrefix(barcode, "NP-CL*");
           // // bool isCLT = MaterialFamilyUtils::matchPrefix(barcode, "NP-CLT*");


           //  // // Szorzó (CL/SL → 2)
           //  // int szorzo = 1;
           //  // if (MaterialFamilyUtils::matchPrefix(barcode, "NP-CL*")
           //  //     || MaterialFamilyUtils::matchPrefix(barcode,"NP-SL*")
           //  //     || MaterialFamilyUtils::matchPrefix(barcode, "NP-CLT*"))
           //  //     szorzo = 2;

           //  // --- REFRAKTORÁLT KOMPOZIT LOGIKA ---
           //  QUuid targetMaterialId = req.materialId;
           //  bool shouldAddLength   = true;

           //  // if (isCL || isCLT)
           //  // {
           //  //      targetMaterialId = CL_COMPOSITE_ID;

           //  //      if (isCLT)
           //  //          shouldAddLength = false;   // CLT nem növeli a festési hosszt
           //  //  }

           //  // ANYAG AGGREGÁLÁS (egységes logika)
           //  PaintMaterialSummary &summary =
           //      colorGroup.materials[targetMaterialId];
           //  summary.materialId = targetMaterialId;

           //  // darabszám mindig nő (CLT is tartozik a lábhoz)
           //  summary.totalPieces += req.quantity;// * szorzo;
           //  summary.requestIds.append(req.requestId);

           //  // hossz csak CL esetén nő
           //  if (shouldAddLength)
           //      summary.totalLength_mm += req.quantity * req.requiredLength;// * szorzo;


           //  // --- PORFOGYÁS SZÁMÍTÁSA ---
           //  double meters = 0.0;
           //  if (shouldAddLength){
           //      //meters = (req.requiredLength * szorzo) / 1000.0;
           //      meters = (req.requiredLength) / 1000.0;
           //  }


           //  auto model = PowderConsumptionRegistry::instance().find(req.productTypeId,
           //                                                          req.productSubtypeId);

           //  double kgPerMeter = model.kgPerMeterCorrected();
           //  double kg = meters * kgPerMeter * req.quantity;

           //  summary.powderKg += kg;
           //  colorGroup.powderKg += kg;

            const BundleDefinition* bundle =
                BundleRegistry::instance().findByCode(mat->bundleCode);

            if (bundle)
            {
                auto comps = BundleRegistry::instance().componentsOf(bundle->code);

                auto model = PowderConsumptionRegistry::instance().find(
                    req.productTypeId, req.productSubtypeId);
                double kgPerMeter = model.kgPerMeterCorrected();

                for (const auto& bc : comps)
                {
                    const MaterialMaster* compMat =
                        MaterialRegistry::instance().findById(bc.materialId);

                    if(compMat->paintingMode == PaintingMode::None)
                        continue;
                    //bool isCLT = MaterialFamilyUtils::matchPrefix(compMat->barcode, "NP-CLT*");

                    int pieceCount = req.quantity * bc.count;
                    int length_mm  = req.requiredLength * pieceCount;

                    addComponent(colorGroup,
                                 compMat,
                                 pieceCount,
                                 length_mm,
                                 kgPerMeter,
                                 req.requestId);
                }

                continue;
            } else{
                auto model = PowderConsumptionRegistry::instance().find(
                    req.productTypeId, req.productSubtypeId);
                double kgPerMeter = model.kgPerMeterCorrected();

                // const MaterialMaster* mat =
                //     MaterialRegistry::instance().findById(req.materialId);

                int pieceCount = req.quantity;
                int length_mm  = req.requiredLength * pieceCount;

                addComponent(colorGroup,
                             mat,
                             pieceCount,
                             length_mm,
                             kgPerMeter,
                             req.requestId);
            }


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
        }// req ciklus vége
    }

    return plan;
}

void PaintCalculator::addComponent(PaintColorGroup& colorGroup,
                                   const MaterialMaster* mat,
                                   int pieceCount,
                                   int length_mm,
                                   double kgPerMeter,
                                   const QUuid& requestId)
{
    auto& summary = colorGroup.materials[mat->id];
    summary.materialId = mat->id;

    summary.totalPieces += pieceCount;
    summary.requestIds.append(requestId);

    bool isCLT = MaterialFamilyUtils::matchPrefix(mat->barcode, "NP-CLT*");

    if (length_mm > 0)
        summary.totalLength_mm += length_mm;

    if(!isCLT){
        double meters = length_mm / 1000.0;
        double kg = meters * kgPerMeter;

        summary.powderKg   += kg;
        colorGroup.powderKg += kg;
    }
}
