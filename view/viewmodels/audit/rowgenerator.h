#pragma once

#include "common/logger.h"
#include "view/cellhelpers/materialcellgenerator.h"
#include "view/tableutils/colorlogicutils.h"
#include "view/tableutils/storageaudittable_rowstyler.h"

#include "view/cellhelpers/auditcelltext.h"
#include "view/cellhelpers/auditcelltooltips.h"
#include "view/columnindexes/audittable_columns.h"
#include "view/viewmodels/audit/cellgenerator.h"
#include "view/viewmodels/tablerowviewmodel.h"
#include "view/viewmodels/tablecellviewmodel.h"

#include "model/storageaudit/storageauditrow.h"
#include "model/material/materialmaster.h"
#include "view/tableutils/tableutils_auditcells.h"

#include <QSpinBox>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QWidget>
#include <QUuid>
#include <QCheckBox>

namespace Audit::ViewModel::RowGenerator {

inline TableRowViewModel generate(const StorageAuditRow& row,
                                  const MaterialMaster& mat,
                                  const QString& groupLabel,
                                  QObject* receiver) {
    TableRowViewModel vm;
    vm.rowId = row.rowId;

    // 🧩 Anyag + csoprt + barcode
    auto matCell = CellGenerators::materialCell(mat, row.barcode);
    // 🎨 Alapszínek a csoport alapján
    QColor baseColor = matCell.background;
    QColor fgColor = matCell.foreground;
    vm.cells[AuditTableColumns::Material] = matCell;

    // 🧩 Tároló
    vm.cells[AuditTableColumns::Storage] =
        TableCellViewModel::fromText(row.storageName,
                                     QString("Tároló: %1").arg(row.storageName),
                                     baseColor, fgColor);

    // 🧩 Elvárt mennyiség – mindig a contextből, ha van
    //int expected = row.context ? row.context->totalExpected : row.pickingQuantity;
    vm.cells[AuditTableColumns::Expected] =
        TableCellViewModel::fromText(AuditCellText::forExpected(row, groupLabel),
                                     AuditCellTooltips::forExpected(row),
                                     baseColor, fgColor);

    // 🧩 Hiányzó mennyiség – mindig a contextből, ha van
    int missing = row.missingQuantity();
    vm.cells[AuditTableColumns::Missing] =
        TableCellViewModel::fromText(QString::number(missing),
                                     AuditCellTooltips::forMissing(row),
                                     baseColor, fgColor);

    // 🧩 Státusz
    vm.cells[AuditTableColumns::Status] =
        TableCellViewModel::fromText(row.statusText(),
                                     AuditCellTooltips::forStatus(row, mat),
                                     row.statusType().toColor(),
                                     Qt::black);

    // 🧩 Tényleges mennyiség (interaktív cella)
    vm.cells[AuditTableColumns::Actual] =
        Audit::ViewModel::CellGenerator::createActualCell(row, receiver, baseColor, fgColor);

    return vm;
}

} // namespace AuditRowViewModelGenerator
