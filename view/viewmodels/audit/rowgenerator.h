#pragma once

#include "common/logger.h"
#include "common/tableutils/colorlogicutils.h"
#include "common/tableutils/storageaudittable_rowstyler.h"
#include "model/storageaudit/auditcontext_text.h"
#include "view/cellhelpers/auditcellcolors.h"
#include "view/cellhelpers/auditcelltext.h"
#include "view/cellhelpers/auditcelltooltips.h"
#include "view/columnindexes/audittable_columns.h"
#include "view/viewmodels/audit/cellgenerator.h"
#include "view/viewmodels/tablerowviewmodel.h"
#include "view/viewmodels/tablecellviewmodel.h"

#include "model/storageaudit/storageauditrow.h"
#include "model/material/materialmaster.h"
#include "common/tableutils/tableutils_auditcells.h"
#include "view/cellhelpers/cellfactory.h"

#include <QSpinBox>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QWidget>
#include <QUuid>
#include <QCheckBox>

namespace Audit::ViewModel::RowGenerator {

/// 🔹 Teljes TableRowViewModel generálása egy StorageAuditRow alapján
inline TableRowViewModel generate(const StorageAuditRow& row,
                                  const MaterialMaster* mat,
                                  const QString& groupLabel,
                                  QObject* receiver) {
    TableRowViewModel vm;
    vm.rowId = row.rowId;

    // 🎨 Alapszínek a csoport alapján
    QColor baseColor = ColorLogicUtils::resolveBaseColor(mat);
    QColor fgColor = baseColor.lightness() < 128 ? Qt::white : Qt::black;

//    QColor baseColor = mat ? AuditColors::groupColor(mat->groupId) : Qt::lightGray;
//    QColor fgColor = baseColor.lightness() < 128 ? Qt::white : Qt::black;

    // 🧩 Cellák feltöltése
    vm.cells[AuditTableColumns::Material] =
        CellFactory::textCell(mat ? mat->name : "Ismeretlen",
                       mat ? mat->color.name() : "",
                       baseColor, fgColor);

    vm.cells[AuditTableColumns::Storage] =
        CellFactory::textCell(row.storageName,
            QString("Tároló: %1").arg(row.storageName),
            baseColor, fgColor);


    vm.cells[AuditTableColumns::Expected] =
        CellFactory::textCell(AuditCellText::formatExpectedQuantity(row, groupLabel),
            AuditCellTooltips::formatExpectedTooltip(row),
            baseColor, fgColor);

    vm.cells[AuditTableColumns::Missing] =
        CellFactory::textCell(AuditCellText::formatMissingQuantity(row),
            AuditCellTooltips::formatMissingTooltip(row),
            baseColor, fgColor);

    vm.cells[AuditTableColumns::Status] =
        CellFactory::textCell(TableUtils::AuditCells::statusText(row),
             AuditCellTooltips::formatStatusTooltip(row, mat),
             AuditCellColors::resolveStatusColor(row),
             Qt::black);

    vm.cells[AuditTableColumns::Barcode] =
        CellFactory::textCell(row.barcode,
            QString("Vonalkód: %1").arg(row.barcode),
            baseColor, fgColor);

    vm.cells[AuditTableColumns::Actual] =
        Audit::ViewModel::CellGenerator::createActualCell(row, receiver, baseColor, fgColor);

    return vm;
}

} // namespace AuditRowViewModelGenerator
