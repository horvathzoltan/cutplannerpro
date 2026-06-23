#include "leftoverstockrepository.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "../../common/filenamehelper.h"
#include "../../service/cutting/result/leftoversourceutils.h"
#include "materials/registry/material_registry.h"
#include "../registries/storageregistry.h"
#include "../../common/filehelper.h"
#include "../../common/csvimporter.h"
#include "model/leftover/leftoverstatusutils.h"

bool LeftoverStockRepository::loadFromCSV(LeftoverStockRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    QString path = helper.getLeftoversCsvFile();
    if (path.isEmpty()) {
        zWarning("❌ Nincs elérhető leftovers.csv fájl");
        return false;
    }

    CsvReader::FileContext ctx(path);
    QVector<LeftoverStockEntry> entries = loadFromCSV_private(ctx);

    // 🔍 Hibák loggolása
    if (ctx.hasErrors()) {
        zWarning(QString("⚠️ Hibák az importálás során (%1 sor):").arg(ctx.errorsSize()));
        zWarning(ctx.toString());
    }

    if (entries.isEmpty()) {
        zWarning("❌ A leftovers.csv fájl üres vagy hibás sorokat tartalmaz.");
        return false;
    }

    // registry.setPersist(false);
    // registry.clearAll(); // 🔄 Korábbi készlet törlése
    // for (const auto& entry : std::as_const(entries))
    //     registry.registerEntry(entry);
    // registry.setPersist(true);

    registry.setData(entries); // 🔧 Itt történik a készletregisztráció
    zInfo(L("✅ %1 készletbejegyzés sikeresen importálva a fájlból: %2").arg(entries.size()).arg(path));

    return true;
}

// QVector<ReusableStockEntry> ReusableStockRepository::loadFromCSV_private(const QString& filepath) {
//     QVector<ReusableStockEntry> result;

//     QFile file(filepath);
//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         qWarning() << "❌ Nem sikerült megnyitni a leftovers fájlt:" << filepath;
//         return result;
//     }

//     QTextStream in(&file);
//     in.setEncoding(QStringConverter::Utf8);

//     const QList<QVector<QString>> rows = FileHelper::parseCSV(&in, ';');
//     if (rows.isEmpty()) {
//         qWarning() << "⚠️ A leftovers.csv fájl üres vagy hibás.";
//         return result;
//     }

//     for (int i = 0; i < rows.size(); ++i) {
//         // ✅ Fejléc sor kihagyása
//         if (i == 0) continue;

//         const QVector<QString>& parts = rows[i];
//         auto maybeEntry = convertRowToReusableEntry(parts, i + 1);
//         if (maybeEntry.has_value())
//             result.append(maybeEntry.value());
//     }

//     return result;
// }

QVector<LeftoverStockEntry>
LeftoverStockRepository::loadFromCSV_private(CsvReader::FileContext& ctx) {
    // QFile file(filepath);
    // if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    //     qWarning() << "❌ Nem sikerült megnyitni a leftovers fájlt:" << filepath;
    //     return {};
    // }

    // QTextStream in(&file);
    // in.setEncoding(QStringConverter::Utf8);
    // const auto rows = FileHelper::parseCSV(&in, ';');

    //return CsvImporter::processCsvRows<LeftoverStockEntry>(rows, convertRowToReusableEntry);
    return CsvReader::readAndConvert<LeftoverStockEntry>(ctx, convertRowToReusableEntry, true);
}


std::optional<LeftoverStockRepository::ReusableStockRow>
LeftoverStockRepository::convertRowToReusableRow(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    if (parts.size() < 9) {
        QString msg = L("⚠️ Kevés oszlop");
        ctx.addError(ctx.currentLineNumber(), msg);

        return std::nullopt;
    }

    ReusableStockRow row;
    row.materialBarcode = parts[0].trimmed();

    bool okLength = false;
    row.availableLength_mm = parts[1].trimmed().toInt(&okLength);
    if (row.materialBarcode.isEmpty() || !okLength || row.availableLength_mm <= 0) {
        QString msg = L("⚠️ Hibás barcode vagy hossz");
        ctx.addError(ctx.currentLineNumber(), msg);

        return std::nullopt;
    }

    // const QString sourceStr = parts[2].trimmed();
    // if (sourceStr == "Optimization")
    //     row.source = LeftoverSource::Optimization;
    // else if (sourceStr == "Manual")
    //     row.source = LeftoverSource::Manual;
    // else {
    //     qWarning() << QString("⚠️ Sor %1: ismeretlen forrástípus").arg(lineIndex);
    //     return std::nullopt;
    // }

    row.source= LeftoverSourceUtils::fromString(parts[2].trimmed());
    if (row.source == Cutting::Result::LeftoverSource::Undefined) {
        QString msg = L("⚠️ Ismeretlen forrástípus");
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    if (row.source == Cutting::Result::LeftoverSource::Optimization && parts.size() > 3) {
        bool okOpt = false;
        const int optId = parts[3].trimmed().toInt(&okOpt);
        if (okOpt)
            row.optimizationId = optId;
        else {
            QString msg = L("⚠️ Hibás optimalizáció ID");
            ctx.addError(ctx.currentLineNumber(), msg);
            return std::nullopt;
        }
    }

    row.barcode = parts[4].trimmed();
    if (row.barcode.isEmpty()) {        
        QString msg = L("⚠️ Hiányzó egyedi barcode");
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    row.storageBarcode = parts[5].trimmed();
    // Ha nincs storage barcode → engedjük meg
    if (row.storageBarcode.isEmpty()) {
        // nincs storage → később storageId = QUuid()
        row.storageBarcode = "";
    }

    row.createdAtStr   = parts[6].trimmed();
    row.lastSeenAtStr  = parts[7].trimmed();
    row.statusStr      = parts[8].trimmed();

    return row;
}

std::optional<LeftoverStockEntry>
LeftoverStockRepository::buildReusableEntryFromRow(const ReusableStockRow& row, CsvReader::FileContext& ctx) {
    const auto* mat = MaterialRegistry::instance().findByBarcode(row.materialBarcode);
    if (!mat) {
        QString msg = L("⚠️ Ismeretlen anyag barcode '%1'").arg(row.materialBarcode);
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    // const auto* storage = StorageRegistry::instance().findByBarcode(row.storageBarcode);
    // if (!storage) {
    //     QString msg = L("⚠️ Ismeretlen tároló barcode '%1'").arg(row.storageBarcode);
    //     ctx.addError(ctx.currentLineNumber(), msg);
    //     return std::nullopt;
    // }

    LeftoverStockEntry entry;
    entry.materialId         = mat->id;
    entry.availableLength_mm = row.availableLength_mm;
    entry.barcode            = row.barcode;
    entry.source             = row.source;
    entry.optimizationId     = row.optimizationId;
    //entry.storageId = storage->id;

    // ⭐ Storage NEM kötelező
    if (!row.storageBarcode.isEmpty()) {
        const auto* storage = StorageRegistry::instance().findByBarcode(row.storageBarcode);
        if (storage)
            entry.storageId = storage->id;
        else {
            // ismeretlen storage → engedjük meg, csak logoljuk
            ctx.addError(ctx.currentLineNumber(),
                         L("⚠️ Ismeretlen tároló barcode '%1' – üres storageId lesz")
                             .arg(row.storageBarcode));
            entry.storageId = QUuid();
        }
    } else {
        // nincs storage → üres ID
        entry.storageId = QUuid();
    }

    // createdAt
    entry.createdAt = QDateTime::fromString(row.createdAtStr, Qt::ISODate);
    if (!entry.createdAt.isValid()) {
        ctx.addError(ctx.currentLineNumber(), L("⚠️ Hibás createdAt dátum"));
        entry.createdAt = QDateTime::currentDateTime();
    }

    // lastSeenAt
    entry.lastSeenAt = QDateTime::fromString(row.lastSeenAtStr, Qt::ISODate);
    if (!entry.lastSeenAt.isValid()) {
        ctx.addError(ctx.currentLineNumber(), L("⚠️ Hibás lastSeenAt dátum"));
        entry.lastSeenAt = entry.createdAt;
    }

    // status
    entry.status = LeftoverStatusUtils::fromString(row.statusStr);

    // zInfo(QString("LOAD REUSABLE LEFTOVER: entryId=%1, length=%2, material=%3, storage=%4")
    //            .arg(entry.entryId.toString())
    //            .arg(entry.availableLength_mm)
    //            .arg(entry.materialId.toString())
    //            .arg(entry.storageId.toString()));

    return entry;
}

std::optional<LeftoverStockEntry>
LeftoverStockRepository::convertRowToReusableEntry(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    const auto rowOpt = convertRowToReusableRow(parts, ctx);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildReusableEntryFromRow(rowOpt.value(), ctx);
}

bool LeftoverStockRepository::saveToCSV(const LeftoverStockRegistry& registry,
                                        const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a leftover stock fájlt írásra:" << filePath;
        return false;
    }

    QTextStream out(&file);

    // Új fejléc
    out << "materialBarCode;availableLength_mm;source;optimizationId;barcode;storageBarcode;"
           "createdAt;lastSeenAt;status\n";

    for (const auto& entry : registry.readAll()) {
        if (entry.availableLength_mm <= 0 || entry.barcode.trimmed().isEmpty())
            continue;

        QString materialCode = entry.materialBarcode();
        QString sourceStr = LeftoverSourceUtils::toString(entry.source);

        QString optIdStr;
        if (entry.source == Cutting::Result::LeftoverSource::Optimization &&
            entry.optimizationId.has_value())
        {
            optIdStr = QString::number(entry.optimizationId.value());
        }

        const auto* storage = StorageRegistry::instance().findById(entry.storageId);
        QString storageBarcode = storage ? storage->barcode : "";

        QString createdStr = entry.createdAt.toString(Qt::ISODate);
        QString seenStr    = entry.lastSeenAt.toString(Qt::ISODate);
        QString statusStr  = LeftoverStatusUtils::toString(entry.status).toLower();

        out << materialCode << ";"
            << entry.availableLength_mm << ";"
            << sourceStr << ";"
            << optIdStr << ";"
            << entry.barcode << ";"
            << storageBarcode << ";"
            << createdStr << ";"
            << seenStr << ";"
            << statusStr << "\n";
    }

    return true;
}


/*
materialBarCode;availableLength_mm;source;optimizationId;barcode;storageBarcode
TE-H-32;2190;Manual;;RSM-171;RACK66
TE-H-32;1234;Manual;;RSM-012;WH04
TE-R-23;3150;Manual;;RSM-170;RACK66
TE-H-32;3500;Manual;;RSM-172;RACK66
TE-H-18;2000;Manual;;RSM-173;RACK66
TE-H-18;5500;Manual;;RSM-174;RACK66
TE-S-18;2000;Manual;;RSM-175;RACK66
TE-H-38;410;Manual;;RSM-176;RACK66
TE-H-32;2090;Manual;;RSM-177;RACK66
TE-R-23;1780;Manual;;RSM-179;RACK66
TE-R-23;1290;Manual;;RSM-180;RACK66
TE-R-23;1220;Manual;;RSM-181;RACK66
TE-R-23;1220;Manual;;RSM-182;RACK66
ROL-P;1200;Manual;;RSM-184;RACK66
ROL-P;1200;Manual;;RSM-185;RACK66
ROL-P;630;Manual;;RSM-187;RACK66
ROL-P;1750;Manual;;RSM-188;RACK66
NP-CLT-9010;1753;Manual;;RSM-152;RACK66
NP-SL-9010;2500;Manual;;RSM-156;RACK66
NP-SL-9010;2500;Manual;;RSM-157;RACK66
NP-SL-9010;1400;Manual;;RSM-158;RACK66
NP-T-9010;2130;Manual;;RSM-160;RACK66
NP-SL-9010;1320;Manual;;RSM-162;RACK66
NP-CZ-9010;1430;Manual;;RSM-164;RACK66
NP-CZ-9010;950;Manual;;RSM-167;RACK66
NP-CL-7016;1570;Manual;;RSM-141;RACK66
NP-CLT-7016;1570;Manual;;RSM-142;RACK66
NP-CLB;573;Manual;;RSM-143;RACK66
NP-CZ-7016;1230;Manual;;RSM-115;RACK66
NP-CZ-7016;1150;Manual;;RSM-116;RACK66
NP-CZ-7016;1220;Manual;;RSM-119;RACK66
NP-CZ-7016;750;Manual;;RSM-120;RACK66
NP-T-7016;1375;Manual;;RSM-013;RACK66
NP-TF-7016;1325;Manual;;RSM-014;RACK66
NP-CZ-7016;1175;Manual;;RSM-015;RACK66
NP-CZ-7016;2500;Manual;;RSM-016;RACK66
NP-CZ-9010;2175;Manual;;RSM-018;RACK66
NP-TF-9010;3450;Manual;;RSM-095;RACK66
NP-T-9010;1125;Manual;;RSM-096;RACK66
NP-TF-9010;1125;Manual;;RSM-097;RACK66
NP-TF-9010;1025;Manual;;RSM-098;RACK66
NP-T-9010;900;Manual;;RSM-101;RACK66
NP-T-9010;900;Manual;;RSM-102;RACK66
NP-TF-9010;875;Manual;;RSM-103;RACK66
NP-TF-9010;850;Manual;;RSM-104;RACK66
NP-T-9010;3475;Manual;;RSM-105;RACK66
NP-CZ-9010;575;Manual;;RSM-106;RACK66
NP-TF;1260;Manual;;RSM-107;RACK66
NP-TF-7016;1125;Manual;;RSM-109;RACK66
NP-SZ-7016;1325;Manual;;RSM-111;RACK66
NP-CL-7016;1975;Manual;;RSM-112;RACK66
NP-CL-7016;1875;Manual;;RSM-113;RACK66
NP-CL-7016;1975;Manual;;RSM-114;RACK66
NP-CL-7016;1875;Manual;;RSK-115;RACK66
NP-CL-7016;1925;Manual;;RSK-116;RACK66
NP-CL-7016;1450;Manual;;RSK-117;RACK66
NP-CLT-9010;4580;Manual;;RSL-117;CM2WH
NP-CL-9010;2200;Manual;;RSM-118;CM2WH
NP-CLT-9010;2200;Manual;;RSK-119;CM2WH
NP-CL-9010;2200;Manual;;RSK-120;CM2WH
NP-CLT-9010;2200;Manual;;RSM-121;CM2WH
NP-CL-9010;2250;Manual;;RSM-122;CM2WH
NP-CZ-9010;3500;Manual;;RSM-124;CM2WH
NP-TF-7016;2020;Manual;;RSM-127;CM2WH
NP-SL-9010;5001;Manual;;RSM-017;RACK66
NP-CZ-7016;3125;Manual;;RSM-129;CM2WH
NP-CL;1300;Manual;;RSM-135;CM2WH
NP-CZ-7016;975;Manual;;RSM-139;CM2WH
NP-CZ-7016;550;Manual;;RSM-140;CM2WH
NP-TF-7016;900;Manual;;RSM-146;CM2WH
NP-T-7016;925;Manual;;RSM-147;CM2WH
NP-TF-7016;1125;Manual;;RSM-149;CM2WH
NP-T-7016;1150;Manual;;RSM-150;CM2WH
NP-T-7016;1175;Manual;;RSM-151;CM2WH
NP-CZ-7016;1300;Manual;;RSM-153;CM2WH
NP-CZ-7016;3125;Manual;;RSM-154;CM2WH
NP-SZ-7016;1650;Manual;;RSM-155;CM2WH
NP-CL-7016;2000;Manual;;RSM-159;CM2WH
NP-T-7016;1125;Manual;;RSM-161;CM2WH
NP-TF-7016;1325;Manual;;RSM-163;CM2WH
NP-TF-7016;2800;Manual;;RSM-165;CM2WH
NP-T-7016;2975;Manual;;RSM-166;CM2WH
NP-CZ-7016;1200;Manual;;RSM-169;CM2WH
NP-T-7016;2650;Manual;;RSM-183;CM2WH
NP-TF-7016;2875;Manual;;RSM-186;CM2WH
NP-T-7016;2150;Manual;;RSM-189;CM2WH
NP-TF-9010;875;Manual;;RSM-190;CM2WH
NP-T-9010;975;Manual;;RSM-191;CM2WH
NP-TF-7016;2725;Manual;;RSM-192;CM2WH
NP-T-7016;2725;Manual;;RSM-193;CM2WH
NP-TF-7016;2850;Manual;;RSM-194;CM2WH
NP-T-7016;2850;Manual;;RSM-195;CM2WH
NP-T-9010;4675;Manual;;RSM-196;CM2WH
NP-TF-9010;4675;Manual;;RSM-197;CM2WH
NP-SZ-7016;1050;Manual;;RSM-198;CM2WH
NP-CL-7016;1250;Manual;;RSM-199;CM2WH
NP-T;3550;Manual;;RSM-201;CM2WH
NP-CZ;5880;Manual;;RSM-202;CM2WH
NP-SZ;4670;Manual;;RSM-203;CM2WH
NP-T;1800;Manual;;RSM-204;CM2WH
NP-TF;900;Manual;;RSM-205;CM2WH
NP-CLBR;3250;Manual;;RSM-206;CM2WH
NP-CL;5280;Manual;;RSM-207;CM2WH
NP-CLBR;1000;Manual;;RSM-208;CM2WH
NP-SZ;650;Manual;;RSM-210;CM2WH
NP-T;2175;Manual;;RSM-211;CM2WH
NP-CZ;1000;Manual;;RSM-212;CM2WH
NP-CZ;900;Manual;;RSM-213;CM2WH
NP-CZ;950;Manual;;RSM-214;CM2WH
NP-CZ;880;Manual;;RSM-215;CM2WH
NP-TF-7016;2935;Manual;;RSM-216;CM2WH
NP-T-7016;925;Manual;;RSM-217;CM2WH
NP-TF-7016;1030;Manual;;RSM-218;CM2WH
NP-T-7016;1050;Manual;;RSM-219;CM2WH
NP-T-7016;1050;Manual;;RSM-220;CM2WH
NP-T-7016;1210;Manual;;RSM-221;CM2WH
NP-TF-7016;1210;Manual;;RSM-222;CM2WH
NP-T-7016;1225;Manual;;RSM-223;CM2WH
NP-TF-7016;1225;Manual;;RSM-224;CM2WH
NP-T-7016;1275;Manual;;RSM-225;CM2WH
NP-TF-7016;1275;Manual;;RSM-226;CM2WH
NP-TF-7016;1350;Manual;;RSM-227;CM2WH
NP-T-7016;1400;Manual;;RSM-228;CM2WH
NP-CL-7016;1020;Manual;;RSM-229;CM2WH
NP-CL-7016;850;Manual;;RSM-230;CM2WH

*/


