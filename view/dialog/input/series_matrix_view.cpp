#include "series_matrix_view.h"

#include "matrix_cell_delegate.h"

#include <materials/registry/material_registry.h>

#include <product/registry/bom_registry.h>
#include <product/registry/material_role_registry.h>

#include <model/registries/cuttingplanrequestregistry.h>

SeriesMatrixView::SeriesMatrixView(QWidget* parent,
                                   CuttingPresenter* presenter)
    : QWidget(nullptr)
    , _presenter(presenter)
{
    auto* layout = new QVBoxLayout(this);
    _table = new QTableWidget(this);

    _table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _table->setSelectionMode(QAbstractItemView::NoSelection);
    _table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    _table->setItemDelegate(new MatrixCellDelegate(_table));

    layout->addWidget(_table);
    setLayout(layout);

    setWindowTitle("Sorozat mátrix");
    resize(900, 600);
}

void SeriesMatrixView::updateMatrix(const ActiveSeries& active,
                                    const QVector<Cutting::Plan::Request>& fullSeries)
{
    _active = active;
    //_full = fullSeries;
    _full = CuttingPlanRequestRegistry::instance().readAll();


    rebuildMatrix();
}

void SeriesMatrixView::onSeriesContextChanged(const QString& owner,
                                              const QString& externalRefPrefix)
{
    if (!_presenter)
        return;

    // 1) Requestek lekérése
    //auto full = _presenter->seriesFor(owner, externalRefPrefix);
    //auto full = _presenter->requestsByExternalReference(externalRefPrefix);
    auto full = CuttingPlanRequestRegistry::instance().readAll();
    if (full.isEmpty())
        return;

    // 2) Új SeriesState építése
    ActiveSeries rebuilt;

    rebuilt.active = true;
    rebuilt.startRef = full.first().externalReference;

    // 3) Oszlopok (tételszámok)
    for (const auto& r : full)
        rebuilt.order.append(r.externalReference);

    // 4) BOM anyagok újragenerálása
    // 🔍 1) Minden request lekérése
    auto all = CuttingPlanRequestRegistry::instance().readAll();

    // 🔍 2) Minden productType + subtype kombináció BOM-ját összegyűjtjük
    QSet<QUuid> bomSet;

    for (const auto& r : all) {
        auto bom = generateBomMaterials(r);
        for (const auto& id : bom)
            bomSet.insert(id);
    }

    // 🔍 3) Globális BOM lista
    //rebuilt.bomMaterials = QVector<QUuid>(bomSet.begin(), bomSet.end());
    QVector<QUuid> globalOrdered;

    for (const auto& r : all) {
        auto ordered = generateBomMaterials(r);
        for (const auto& id : ordered)
            if (!globalOrdered.contains(id))
                globalOrdered << id;
    }

    rebuilt.bomMaterials = globalOrdered;


    // 5) Cellák újragenerálása
    //auto all = CuttingPlanRequestRegistry::instance().readAll();
    for (const auto& r : all)
        rebuilt.filledCells.insert({ r.externalReference, r.materialId });

    // 6) Aktuális cella
    int idx = 0;
    for (int i = 0; i < rebuilt.order.size(); ++i)
        if (rebuilt.order[i] == externalRefPrefix)
            idx = i;

    rebuilt.currentColumnIndex = idx;

    rebuilt.currentMaterialIndex = 0;

    // 7) Mátrix frissítése
    updateMatrix(rebuilt, full);
}


void SeriesMatrixView::rebuildMatrix()
{
    if (_full.isEmpty()) {
        _table->clear();
        _table->setRowCount(0);
        _table->setColumnCount(0);
        return;
    }

    buildHeaders();

    // _table->horizontalHeader()->setStyleSheet(
    //     "QHeaderView::section {"
    //     "  background-color: #f0f0f0;"
    //     "  font-weight: bold;"
    //     "  font-size: 12pt;"
    //     "  border: 1px solid #cccccc;"
    //     "}"
    //     );

    // _table->verticalHeader()->setStyleSheet(
    //     "QHeaderView::section {"
    //     "  background-color: #f0f0f0;"
    //     "  font-weight: bold;"
    //     "  font-size: 12pt;"
    //     "  border: 1px solid #cccccc;"
    //     "}"
    //     );


    buildRows();
    fillCells();
}

void SeriesMatrixView::buildHeaders()
{
    QStringList cols;
    cols << "Anyag";

    auto all = CuttingPlanRequestRegistry::instance().readAll();

    QSet<QString> seen;
    QStringList orderedRefs;

    for (const auto& r : all) {
        if (!seen.contains(r.externalReference)) {
            seen.insert(r.externalReference);
            orderedRefs << r.externalReference;
        }
    }

    for (const auto& ref : orderedRefs)
        cols << ref;

    _table->setColumnCount(cols.size());
    _table->setHorizontalHeaderLabels(cols);
}

// void SeriesMatrixView::buildHeaders()
// {
//     QStringList cols;
//     cols << "Anyag";

//     for (const auto& r : _full)
//         cols << r.externalReference;

//     _table->setColumnCount(cols.size());
//     _table->setHorizontalHeaderLabels(cols);
// }

// void SeriesMatrixView::buildHeaders()
// {
//     QStringList cols;
//     cols << "Anyag";

//     // 🔍 1) Minden request lekérése a registryből
//     auto all = CuttingPlanRequestRegistry::instance().readAll();

//     // 🔍 2) Egyedi tételszámok sorrendben
//     QSet<QString> seen;
//     QStringList orderedRefs;

//     for (const auto& r : all) {
//         if (!seen.contains(r.externalReference)) {
//             seen.insert(r.externalReference);
//             orderedRefs << r.externalReference;
//         }
//     }

//     // 🔍 3) Oszlopok hozzáadása
//     for (const auto& ref : orderedRefs)
//         cols << ref;

//     _table->setColumnCount(cols.size());
//     _table->setHorizontalHeaderLabels(cols);
// }



void SeriesMatrixView::buildRows()
{
    // sorok = BOM anyagok
    _table->setRowCount(_active.bomMaterials.size());

    for (int i = 0; i < _active.bomMaterials.size(); ++i) {
        QUuid matId = _active.bomMaterials[i];

        auto* mat = MaterialRegistry::instance().findById(matId);
        QString name = mat ? mat->toDisplay() : "(ismeretlen anyag)";

        auto* item = new QTableWidgetItem(name);
        item->setBackground(Qt::lightGray);

        //item->setFont(QFont("Segoe UI", 12, QFont::Bold));
        QFont f = item->font();   // platform alap font
        f.setPointSize(8);//f.pointSize() - 2);   // 2 ponttal kisebb
        item->setFont(f);
        item->setForeground(QColor("#333333"));

        _table->setItem(i, 0, item);
    }
}

const Cutting::Plan::Request* SeriesMatrixView::findRequestByExternalRef(const QString& ref) const
{
    for (const auto& r : _full)
        if (r.externalReference == ref)
            return &r;
    return nullptr;
}

void SeriesMatrixView::fillCells()
{
    computeMaterialSets();

    int activeRow  = _active.currentMaterialIndex;
    int activeCol  = _active.currentColumnIndex + 1;

    for (int col = 1; col < _table->columnCount(); ++col) {

        QString ref = _table->horizontalHeaderItem(col)->text();

        for (int row = 0; row < _active.bomMaterials.size(); ++row) {

            QUuid matId = _active.bomMaterials[row];

            bool isFilled = _active.filledCells.contains({ref, matId});
            //bool isActual = _actualMaterials.contains(matId);
            //bool isBom    = _bomMaterialsSet.contains(matId);
            const auto* req = findRequestByExternalRef(ref);
            auto bomForThisColumn = generateBomMaterials(*req);

            bool isBom = bomForThisColumn.contains(matId);

            QSet<QUuid> actualForColumn;
            for (const auto& r : _full)
                if (r.externalReference == ref)
                    actualForColumn.insert(r.materialId);

            bool isActual = actualForColumn.contains(matId);

            // --- Család-szintű kielégítés (alternatív BOM elemek) ---
            bool familySatisfied = false;

            auto* mat = MaterialRegistry::instance().findById(matId);
            MaterialFamily fam = mat ? mat->family : MaterialFamily::Unknown;

            for (const auto& r : _full) {
                if (r.externalReference == ref) {
                    auto* m2 = MaterialRegistry::instance().findById(r.materialId);
                    if (m2 && m2->family == fam) {
                        familySatisfied = true;
                        break;
                    }
                }
            }
            QString symbol = "";
            QColor bg = Qt::white;

            // Ha a család már teljesítve, és ez a konkrét anyag nincs felvéve,
            // akkor NE legyen ☐, NE legyen ⚠ → legyen semleges szürke.
            // 1) ✔ kitöltött cella
            if (isFilled) {
                symbol = "✔";
                bg = QColor("#c8f7c5");
            }
            else {

                // 2) Család-szintű kielégítés
                if (familySatisfied) {
                    symbol = "";
                    bg = QColor("#c8f7c5");   // pipanélküli zöld
                }
                else {

                    // 3) ☐ hiányzó BOM-anyag
                    if (isBom && !isActual) {
                        symbol = "☐";
                        bg = QColor("#e8e8ff");
                    }

                    // 4) ⚠ felesleges anyag
                    if (!isBom && isActual) {
                        symbol = "⚠";
                        bg = QColor("#ffd6d6");
                    }
                }
            }

            // --- NEM BOM cella jelölése (áthúzás + szürke háttér) ---
            if (!isBom) {
                bg = Qt::lightGray;
            }

            auto* item = new QTableWidgetItem(symbol);
            item->setTextAlignment(Qt::AlignCenter);
            item->setBackground(bg);
            item->setForeground(QColor("#222222"));

            // --- Aktuális cella jelölése (keret) ---
            if (row == activeRow && col == activeCol) {
                item->setData(Qt::UserRole, "active");
            }

            // --- Nem BOM cella jelölése (áthúzás) ---
            if (!isBom) {
                item->setData(Qt::UserRole + 1, "nonBom");
            }

            _table->setItem(row, col, item);
        }
    }
}




void SeriesMatrixView::computeMaterialSets()
{
    _actualMaterials.clear();
    _bomMaterialsSet.clear();

    // 1) BOM anyagok halmaza
    for (const QUuid& id : _active.bomMaterials)
        _bomMaterialsSet.insert(id);

    // 2) Sorozatban ténylegesen előforduló anyagok
    for (const auto& r : _full)
        _actualMaterials.insert(r.materialId);

    // 🔍 3) PIPÁK: teljes registryből kell építeni
    auto all = CuttingPlanRequestRegistry::instance().readAll();
    for (const auto& r : all) {
        _active.filledCells.insert({ r.externalReference, r.materialId });
    }
}


// QVector<QUuid> SeriesMatrixView::generateBomMaterials(const Cutting::Plan::Request& req)
// {
//     QVector<QUuid> result;

//     // 1) BOM családok lekérése
//     auto bomFamilies = BomRegistry::instance()
//                            .bomMap(req.productTypeId, req.productSubtypeId);

//     // 2) MaterialRoleMap prefixek lekérése
//     auto roles = MaterialRoleRegistry::instance()
//                      .findRoles(req.productTypeId, req.productSubtypeId);

//     NamedColor reqColor = req.requiredColor;

//     for (const auto& role : roles) {

//         QString prefix = role.barcodePrefix.trimmed();
//         if (prefix.endsWith("*"))
//             prefix.chop(1);

//         MaterialFamily fam = role.family;

//         // 3) Csak a BOM-ban szereplő családok
//         if (!bomFamilies.contains(fam))
//             continue;

//         // 4) Csak az adott prefixhez tartozó anyagok
//         for (const auto& mat : MaterialRegistry::instance().readAll()) {

//             if (!mat.barcode.startsWith(prefix))
//                 continue;

//             // 5) Csak az adott család
//             if (mat.family != fam)
//                 continue;

//             // 6) Csak az adott szín
//             // 🔍 Csak akkor színszűrünk, ha az anyagnak tényleg van színe
//             bool materialHasColor = mat.color.isValid() &&
//                                     !mat.color.code().trimmed().isEmpty();

//             if (materialHasColor) {
//                 // Színes anyag → színszűrés kötelező
//                 if (mat.color.code() != reqColor.code())
//                     continue;
//             }
//             // Színfüggetlen anyag → NINCS színszűrés


//             result.append(mat.id);
//         }
//     }


//     // --- BOM sorrend stabilizálása ---

//     // --- BOM sorrend stabilizálása ---

//     QVector<QUuid> ordered;

//     // 1) BOM családok sorrendje
//     QList<MaterialFamily> famOrder = bomFamilies.keys();

//     // 2) Családonként prefixek sorrendje
//     for (MaterialFamily fam : famOrder) {

//         QStringList famPrefixes;
//         for (const auto& role : roles) {
//             if (role.family == fam) {
//                 QString prefix = role.barcodePrefix.trimmed();
//                 if (prefix.endsWith("*"))
//                     prefix.chop(1);
//                 famPrefixes << prefix;
//             }
//         }

//         // 3) Prefixek sorrendje → anyagok sorrendje
//         for (const auto& prefix : famPrefixes) {
//             for (const auto& mat : MaterialRegistry::instance().readAll()) {
//                 if (mat.family == fam && mat.barcode.startsWith(prefix)) {
//                     if (result.contains(mat.id))
//                         ordered << mat.id;
//                 }
//             }
//         }
//     }

//     return ordered;


// }

QVector<QUuid> SeriesMatrixView::generateBomMaterials(const Cutting::Plan::Request& req)
{
    QVector<QUuid> result;

    // 1) BOM családok lekérése (QHash → nem stabil sorrend!)
    auto bomFamilies = BomRegistry::instance()
                           .bomMap(req.productTypeId, req.productSubtypeId);

    // --- Stabil BOM család sorrend ---
    QList<MaterialFamily> famOrder = bomFamilies.keys();
    std::sort(famOrder.begin(), famOrder.end(),
              [](MaterialFamily a, MaterialFamily b) {
                  return static_cast<int>(a) < static_cast<int>(b);
              });

    // 2) MaterialRoleMap prefixek lekérése
    auto roles = MaterialRoleRegistry::instance()
                     .findRoles(req.productTypeId, req.productSubtypeId);

    // --- Stabil prefix sorrend ---
    std::sort(roles.begin(), roles.end(),
              [](const MaterialRole& a, const MaterialRole& b) {
                  return a.barcodePrefix < b.barcodePrefix;
              });

    NamedColor reqColor = req.requiredColor;

    // 3) MaterialRegistry stabil sorrendben
    auto mats = MaterialRegistry::instance().readAll();
    std::sort(mats.begin(), mats.end(),
              [](const MaterialMaster& a, const MaterialMaster& b) {
                  return a.barcode < b.barcode;
              });

    // 4) BOM anyagok gyűjtése stabil sorrendben
    for (MaterialFamily fam : famOrder) {

        // prefixek stabil sorrendben
        QStringList famPrefixes;
        for (const auto& role : roles) {
            if (role.family == fam) {
                QString prefix = role.barcodePrefix.trimmed();
                if (prefix.endsWith("*"))
                    prefix.chop(1);
                famPrefixes << prefix;
            }
        }
        famPrefixes.sort();

        // anyagok stabil sorrendben
        for (const auto& prefix : famPrefixes) {
            for (const auto& mat : mats) {

                if (mat.family != fam)
                    continue;

                if (!mat.barcode.startsWith(prefix))
                    continue;

                // színszűrés
                bool materialHasColor = mat.color.isValid() &&
                                        !mat.color.code().trimmed().isEmpty();

                if (materialHasColor &&
                    mat.color.code() != reqColor.code())
                    continue;

                result.append(mat.id);
            }
        }
    }

    return result;
}

void SeriesMatrixView::setActiveReference(const QString& ref)
{
    _full = CuttingPlanRequestRegistry::instance().readAll();

    // oszlop index keresése
    int idx = 0;
    for (int i = 0; i < _full.size(); ++i)
        if (_full[i].externalReference == ref)
            idx = i;

    _active.currentColumnIndex = idx;
    _active.currentMaterialIndex = 0;

    rebuildMatrix();
}

void SeriesMatrixView::refreshAfterAdd(const QString& ref)
{
    _full = CuttingPlanRequestRegistry::instance().readAll();

    // aktuális oszlop kijelölése
    int idx = 0;
    for (int i = 0; i < _full.size(); ++i)
        if (_full[i].externalReference == ref)
            idx = i;

    _active.currentColumnIndex = idx;
    _active.currentMaterialIndex = 0;

    rebuildMatrix();
}

void SeriesMatrixView::closeEvent(QCloseEvent* e)
{
    emit matrixClosed();
    QWidget::closeEvent(e);
}

