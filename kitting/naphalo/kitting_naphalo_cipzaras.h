#pragma once

#include "common/logger.h"
#include "kitting/model/kittinginstruction.h"
#include "product/utils/material_role_utils.h"
#include <QMap>
#include <QString>
#include <kitting/kittingengine.h>
#include <materials/model/material_family_utils.h>
#include <materials/model/material_master.h>
#include <materials/registry/material_registry.h>
#include <product/registry/material_role_registry.h>
#include <product/selector/material_selector.h>

namespace Kitting{
namespace Naphalo{
namespace Cipzaras{// KITTING_NAPHALO_CIPZARAS_H

inline QVector<KittingInstruction> expandTok(
    const Cutting::Plan::Request& req,
    const Cutting::Piece::PieceWithMaterial& pwm,
    const Cutting::Plan::CutPlan& plan)
{
    QVector<KittingInstruction> out;

    auto pofa_R = MaterialRegistry::instance().findByBarcode("NP-CP-R");
    if(pofa_R){
        out << KittingInstruction::makeKitItem(req, pwm, pofa_R, "NP-CP");
    }

    auto pofa_L = MaterialRegistry::instance().findByBarcode("NP-CP-L");
    if(pofa_L){
        out << KittingInstruction::makeKitItem(req, pwm, pofa_L, "NP-CP");
    }

    auto csavar = MaterialRegistry::instance().findByBarcode("NP-CS3");
    if(csavar){
        out << KittingInstruction::makeKitItem(req, pwm, csavar, "NP-CS");
        out << KittingInstruction::makeKitItem(req, pwm, csavar, "NP-CS");
        out << KittingInstruction::makeKitItem(req, pwm, csavar, "NP-CS");
    }

    return out;
}

inline QVector<KittingInstruction> expandTokFedel(
    const Cutting::Plan::Request& req,
    const Cutting::Piece::PieceWithMaterial& pwm,
    const Cutting::Plan::CutPlan& plan)
{
    QVector<KittingInstruction> out;

    auto csavar = MaterialRegistry::instance().findByBarcode("NP-CS1");
    if(csavar){
        out << KittingInstruction::makeKitItem(req, pwm, csavar, "NP-CS");
        out << KittingInstruction::makeKitItem(req, pwm, csavar, "NP-CS");
    }

    return out;
}

inline QVector<KittingInstruction> expandTengely_Motoros(
    const Cutting::Plan::Request& req,
    const Cutting::Piece::PieceWithMaterial& pwm,
    const Cutting::Plan::CutPlan& plan)
{

    QVector<KittingInstruction> out;
    // pofa anyag (pl. NP-POFA-CIP)
    auto motors = MaterialRegistry::instance().findAllByFamily(MaterialFamily::Motor);
    if(!motors.isEmpty()){
        QVector<QUuid> motors_ranked = MaterialSelector::rankMaterials(motors, req);

        zInfo("=== expandTengely motor ===");
        for(auto &a:motors_ranked){
            auto motor = MaterialRegistry::instance().findById(a);
            zInfo("motor:"+motor->toReportLabel());
        }

        if(!motors_ranked.isEmpty() ){
            auto motor = MaterialRegistry::instance().findById(motors_ranked.first());

            if(motor){
                out << KittingInstruction::makeKitItem(req, pwm, motor, "NP-MOT");
            }
        }
    }
    return out;
}

inline QVector<KittingInstruction> expandTengely_Dugo(
    const Cutting::Plan::Request& req,
    const Cutting::Piece::PieceWithMaterial& pwm,
    const Cutting::Plan::CutPlan& plan)
{
    QVector<KittingInstruction> out;

    auto mat = MaterialRegistry::instance().findById(pwm.materialId);
    if(mat){
        if(mat->diameter_mm==70){
            auto dugo = MaterialRegistry::instance().findByBarcode("NP-ROLL70-D");
            if(dugo){
                out << KittingInstruction::makeKitItem(req, pwm, dugo, "NP-ROLL70-D");
            }
        } else if(mat->diameter_mm ==78){
            auto dugo = MaterialRegistry::instance().findByBarcode("NP-ROLL78-D");
            if(dugo){
                out << KittingInstruction::makeKitItem(req, pwm, dugo, "NP-ROLL78-D");
            }
        }
    }

    // TODO motor adapter a tengely átmérőtől függ
    return out;
}

inline QVector<KittingInstruction> expandZaro(
    const Cutting::Plan::Request& req,
    const Cutting::Piece::PieceWithMaterial& pwm,
    const Cutting::Plan::CutPlan& plan)
{
    QVector<KittingInstruction> out;

    auto zaro = MaterialRegistry::instance().findByBarcode("NP-CZ-D");
    if(zaro){
        out << KittingInstruction::makeKitItem(req, pwm, zaro, "NP-CZ");
        out << KittingInstruction::makeKitItem(req, pwm, zaro, "NP-CZ");
        out << KittingInstruction::makeKitItem(req, pwm, zaro, "NP-CZ");
        out << KittingInstruction::makeKitItem(req, pwm, zaro, "NP-CZ");
    }

    auto csavar = MaterialRegistry::instance().findByBarcode("NP-CS4");
    if(csavar) {
        out << KittingInstruction::makeKitItem(req, pwm, csavar, "NP-CZ");
        out << KittingInstruction::makeKitItem(req, pwm, csavar, "NP-CZ");
        out << KittingInstruction::makeKitItem(req, pwm, csavar, "NP-CZ");
        out << KittingInstruction::makeKitItem(req, pwm, csavar, "NP-CZ");
    }

    auto kefe = MaterialRegistry::instance().findByBarcode("NP-CZ-K");
    if(kefe){
        double hossz_mm = pwm.info.length_mm-20;   // vagy pwm.info.requiredLength
        out << KittingInstruction::makeKitItem(req, pwm, kefe, "NP-CZ", hossz_mm, "mm");
    }

    return out;
}

inline QVector<KittingInstruction> expandTengely_Kurblis(
    const Cutting::Plan::Request& req,
    const Cutting::Piece::PieceWithMaterial& pwm,
    const Cutting::Plan::CutPlan& plan)
{
    return {};
}


inline QVector<KittingInstruction> expand(
    const Cutting::Plan::Request& req,
    const Cutting::Piece::PieceWithMaterial& pwm,
    const Cutting::Plan::CutPlan& plan)
{
    QVector<KittingInstruction> out;

    // 🔹 1) Levágott darab szerepköre – pl. "NP-T", "NP-ROLL", "NP-CZ", ...
    //QString roleCode = pwm.info.roleCode;   // ezt neked kell biztosan kitölteni a PieceInfo-ban
    auto mat = MaterialRegistry::instance().findById(pwm.materialId);
    if(!mat) return{};
    MaterialRole role = MaterialRoleUtils::makeRole(req, mat);

    // --- TOK → pofa + csavar + tokfedél ---
    if (role.barcodePrefix == "NP-T") {
        out << expandTok(req,pwm,plan);
    }

    if (role.barcodePrefix == "NP-TF") {
        out << expandTokFedel(req,pwm,plan);
    }

    // --- TENGELY → motor + adapter + dugó ---
    if (role.barcodePrefix == "NP-ROLL") {
        //zInfo("tétel: "+req.externalReference+". tengely:" + mat->toReportLabel());
        out << expandTengely_Dugo(req,pwm,plan);
        const QString meghajtas_key = "meghajtas";
        if(req.attributes.contains(meghajtas_key)){
            QString meghajtas = req.attributes.value(meghajtas_key);
            if (meghajtas == "motoros"){
                out << expandTengely_Motoros(req,pwm,plan);
            }else if (meghajtas == "kurblis") {
                out << expandTengely_Kurblis(req,pwm,plan);
            }else {
                zInfo("Ismeretlen attr value: "+meghajtas);
            }
        }
        else
        {
            zInfo(L("Ismeretlen attr key: ")+meghajtas_key);
        }
    }

    // --- CIPZÁRAS ZÁRÓ → zárósúly ---
    if (role.barcodePrefix == "NP-CZ") {
        out << expandZaro(req,pwm,plan);
    }

    // --- LAB → labbetét ---
    if (role.barcodePrefix == "NP-CL") {
        // const MaterialMaster* labBetet =
        //     MaterialRegistry::instance().findByBarcodePrefix("NP-LABB");
        // out << makeKitItem(req, pwm, labBetet, "NP-LABB");
    }

    return out;
}




} //end of namespace Cipzaras
}
}