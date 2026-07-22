#include "series_matrix_view.h"

#include "matrix_cell_delegate.h"

#include <materials/registry/material_registry.h>

#include <product/registry/bom_registry.h>
#include <product/registry/material_role_registry.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

#include <model/registries/cuttingplanrequestregistry.h>

#include <materials/model/material_family_utils.h>

SeriesMatrixView::SeriesMatrixView(QWidget* parent,
                                   CuttingPresenter* presenter)
    : QWidget(nullptr)
    , _presenter(presenter)
{
    setWindowFlags(Qt::Window);

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

    //_colorless.build();
}

QVector<QUuid> SeriesMatrixView::buildSectionedBomList()
{
    QSet<QUuid> unique;
    QVector<QUuid> ordered;

    // --- 1) minden request BOM-ját összegyűjtjük ---
    for (const auto& r : _full) {
        auto bom = generateBomMaterials(r, NamedColor());

        for (const auto& id : bom) {
            if (!unique.contains(id)) {
                unique.insert(id);
                ordered << id;
            }
        }
    }

    // --- 2) család + barcode szerinti rendezés ---
    std::sort(ordered.begin(), ordered.end(),
              [](const QUuid& a, const QUuid& b) {
                  auto* ma = MaterialRegistry::instance().findById(a);
                  auto* mb = MaterialRegistry::instance().findById(b);
                  if (!ma || !mb) return false;

                  if (ma->family != mb->family)
                      return (int)ma->family < (int)mb->family;

                  return ma->barcode < mb->barcode;
              });

    return ordered;
}




void SeriesMatrixView::updateMatrix(const ActiveSeries& active,
                                    const QVector<Cutting::Plan::Request>& fullSeries)
{
    _full = fullSeries;//CuttingPlanRequestRegistry::instance().readAll();


    // ⭐ filledCells megőrzése
    auto savedFilled = _active.filledCells;

    _active = active;

    // --- 4) Fallback: ha a dialogból érkező order üres, építsük újra ---
    if (_active.order.isEmpty()) {
        for (const auto& r : _full)
            _active.order.append(r.externalReference);
    }

    // --- 0) tételszámok deduplikálása ---
    {
        QSet<QString> seen;
        QStringList dedup;

        for (const auto& ref : _active.order) {
            if (!seen.contains(ref)) {
                seen.insert(ref);
                dedup << ref;
            }
        }

        _active.order = dedup;
    }

    // ⭐ visszatöltés
    _active.filledCells = savedFilled;

    // --- BOM inicializálása hideg induláskor ---
    if (_active.bomMaterials.isEmpty()) {
        _active.bomMaterials = buildSectionedBomList();
    }


    bool bomChanged = (_active.bomMaterials.size() != _table->rowCount());
    bool refsChanged = ((_table->columnCount() - 1) != _active.order.size());

    if (bomChanged || refsChanged) {
        rebuildMatrix();
        return;
    }

    int col = _active.currentColumnIndex + 1;
    int row = _active.currentMaterialIndex;

    updateCell(row, col);
}





void SeriesMatrixView::rebuildMatrix()
{
    int oldRowCount = _table->rowCount();
    int newRowCount = _active.bomMaterials.size();

    buildHeaders();

    buildRows();
    fillCells();
}

void SeriesMatrixView::buildHeaders()
{
    QStringList cols;
    cols << "Anyag";

    for (const auto& ref : _active.order)
        cols << ref;

    _table->setColumnCount(cols.size());
    _table->setHorizontalHeaderLabels(cols);
}


void SeriesMatrixView::buildRows()
{
    _table->setRowCount(0);

    const auto& mats = _active.bomMaterials;

    for (int i = 0; i < mats.size(); ++i) {
        _table->insertRow(i);

        auto* mat = MaterialRegistry::instance().findById(mats[i]);
        QString name = mat ? mat->toDisplay() : "(ismeretlen anyag)";

        auto* item = new QTableWidgetItem(name);
        item->setFont(QFont("Segoe UI", 8));
        item->setForeground(QColor("#333333"));

        // --- NINCS színezés ---
        item->setBackground(Qt::white);

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

    for (int col = 1; col < _table->columnCount(); ++col) {
        for (int row = 0; row < _active.bomMaterials.size(); ++row) {
            updateCell(row, col);
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

    // 3) PIPÁK: cache-ből
    _active.filledCells = _filledCache;
}





QVector<QUuid> SeriesMatrixView::generateBomMaterials(
    const Cutting::Plan::Request& req,
    const NamedColor& effectiveColor) const
{
    auto key = qMakePair(req.productTypeId, req.productSubtypeId);

    // --- Cache hit ---
    if (_bomCache.contains(key)) {
        return _bomCache[key];
    }

    QVector<QUuid> result;

    // --- 1) BOM családok stabil sorrendben ---
    auto bomFamilies = BomRegistry::instance()
                           .bomMap(req.productTypeId, req.productSubtypeId);

    QList<MaterialFamily> famOrder = bomFamilies.keys();
    std::sort(famOrder.begin(), famOrder.end(),
              [](MaterialFamily a, MaterialFamily b) {
                  return static_cast<int>(a) < static_cast<int>(b);
              });

    // --- 2) Rolemap prefixek stabil sorrendben ---
    auto roles = MaterialRoleRegistry::instance()
                     .findRoles(req.productTypeId, req.productSubtypeId);

    std::sort(roles.begin(), roles.end(),
              [](const MaterialRole& a, const MaterialRole& b) {
                  return a.barcodePrefix < b.barcodePrefix;
              });

    // --- 3) Anyagok stabil sorrendben ---
    auto mats = MaterialRegistry::instance().readAll();
    std::sort(mats.begin(), mats.end(),
              [](const MaterialMaster& a, const MaterialMaster& b) {
                  return a.barcode < b.barcode;
              });

    // --- 4) BOM anyagok gyűjtése ---
    for (MaterialFamily fam : famOrder) {

        // prefixek gyűjtése (csillag marad!)
        QStringList famPrefixes;
        for (const auto& role : roles) {
            if (role.family == fam) {
                famPrefixes << role.barcodePrefix.trimmed();
            }
        }
        famPrefixes.sort();

        // anyagok gyűjtése
        for (const auto& prefix : famPrefixes) {
            for (const auto& mat : mats) {

                // család szűrés
                if (mat.family != fam)
                    continue;

                // prefix szűrés — helyes wildcard logika
                if (!MaterialFamilyUtils::matchPrefix(mat.barcode, prefix))
                    continue;

                // színszűrés (opcionális)
                bool materialHasColor = mat.color.isValid() &&
                                        !mat.color.code().trimmed().isEmpty();

                if (effectiveColor.isValid() &&
                    !effectiveColor.code().trimmed().isEmpty())
                {
                    if (materialHasColor &&
                        mat.color.code() != effectiveColor.code())
                        continue;

                    if (!materialHasColor)
                        continue;
                }

                result.append(mat.id);
            }
        }
    }

    // --- Cache store ---
    _bomCache[key] = result;
    return result;
}


void SeriesMatrixView::setActiveReference(const QString& ref)
{
    int col = findColumnIndex(ref);
    if (col == -1)
        return;

    _active.currentColumnIndex = col - 1;
    _active.currentMaterialIndex = 0;

    updateCell(_active.currentMaterialIndex, col);
}


void SeriesMatrixView::refreshAfterAdd(const QString& ref)
{
    _full = CuttingPlanRequestRegistry::instance().readAll();

    // ⭐ PIPÁK SZINKRONIZÁLÁSA
    _active.filledCells = _filledCache;

    // ⭐ BOM SZINKRONIZÁLÁSA
    clearBomCache();
    _active.bomMaterials = buildSectionedBomList();

    int idx = findColumnIndex(ref);

    if (_table->rowCount() == 0 || _active.bomMaterials.isEmpty()) {
        ActiveSeries s;
        s.active = true;
        s.startRef = ref;
        QSet<QString> seen;
        for (const auto& r : _full) {
            if (!seen.contains(r.externalReference)) {
                seen.insert(r.externalReference);
                s.order.append(r.externalReference);
            }
        }

        s.currentColumnIndex = s.order.indexOf(ref);
        s.currentMaterialIndex = 0;

        updateMatrix(s, _full);
        return;
    }

    if (idx == -1) {
        addColumn(ref);
        idx = _table->columnCount() - 1;
    }


    _active.currentColumnIndex = idx - 1;
    _active.currentMaterialIndex = 0;

    // --- HIÁNYZÓ LÉPÉS ---
    if (!_active.order.contains(ref)) {
        _active.order.append(ref);
    }

    // ❗ NEM egy cella, hanem az egész oszlop frissítése
    for (int row = 0; row < _active.bomMaterials.size(); ++row) {
        updateCell(row, idx);
    }
}




void SeriesMatrixView::closeEvent(QCloseEvent* e)
{
    emit matrixClosed();
    QWidget::closeEvent(e);
}


void SeriesMatrixView::clearBomCache() {
    _bomCache.clear();
}

void SeriesMatrixView::addFilledCell(const QString& ref, const QUuid& matId)
{
    _filledCache.insert({ref, matId});
}

void SeriesMatrixView::removeFilledCell(const QString& ref, const QUuid& matId)
{
    _filledCache.remove({ref, matId});
}

void SeriesMatrixView::updateFilledCell(const QString& oldRef, const QUuid& oldMat,
                                        const QString& newRef, const QUuid& newMat)
{
    // Ha a régi cella létezett → klasszikus update
    if (_filledCache.contains({oldRef, oldMat})) {
        _filledCache.remove({oldRef, oldMat});
    }

    // Ha a régi cella nem létezett → új anyag felvitele
    _filledCache.insert({newRef, newMat});
}



void SeriesMatrixView::addColumn(const QString& ref)
{
    // 1) új oszlop index
    int col = _table->columnCount();

    // 2) oszlop hozzáadása
    _table->insertColumn(col);
    _table->setHorizontalHeaderItem(col, new QTableWidgetItem(ref));

    // 3) cellák frissítése az új oszlopban
    for (int row = 0; row < _active.bomMaterials.size(); ++row) {
        updateCell(row, col);
    }
}

void SeriesMatrixView::updateCell(int row, int col)
{
    // --- hideg indulás guard ---
    if (_active.bomMaterials.isEmpty())
        return;
    if (_table->columnCount() == 0)
        return;
    if (col >= _table->columnCount())
        return;
    if (row >= _active.bomMaterials.size())
        return;

    QString ref = _table->horizontalHeaderItem(col)->text();
    QUuid matId = _active.bomMaterials[row];

    bool isFilled = _active.filledCells.contains({ref, matId});

    // qDebug() << "ref header:" << ref;
    // for (auto& p : _active.filledCells)
    //     qDebug() << "filledCells ref:" << p.first;

    const auto* req = findRequestByExternalRef(ref);
    auto effectiveColor = effectiveColorForReference(ref);
    auto bomForThisColumn = generateBomMaterials(*req, effectiveColor);
    bool isBom = bomForThisColumn.contains(matId);

    QSet<QUuid> actualForColumn;
    for (const auto& r : _full)
        if (r.externalReference == ref)
            actualForColumn.insert(r.materialId);

    bool isActual = actualForColumn.contains(matId);

    // család-szintű kielégítés
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

    QString symbol = " ";
    QColor bg = Qt::white;

    if (isFilled) {
        // Ha a cella filledCache-ben van → mindig pipa,
        // függetlenül attól, hogy BOM-tag-e
        symbol = "✔";
        bg = QColor("#c8f7c5");
    }
    else {
        if (familySatisfied) {
            // Család szinten kielégített → zöld háttér, pipa nélkül
            symbol = " ";
            bg = QColor("#c8f7c5");
        }
        else {
            if (isBom && !isActual) {
                // BOM-ban van, de még nincs tényleges request → üres checkbox
                symbol = "☐";
                bg = QColor("#e8e8ff");
            }
            if (!isBom && isActual) {
                // Nem BOM, de ténylegesen vágunk ilyen anyagot → warning
                symbol = "⚠";
                bg = QColor("#ffd6d6");
            }
        }
    }

    // ❗ NEM írjuk felül a symbol/bg-t, ha isFilled vagy familySatisfied
    // Csak akkor szürkítjük, ha sem BOM, sem actual, sem filled, sem familySatisfied
    if (!isBom && !isActual && !isFilled && !familySatisfied) {
        symbol = " ";
        bg = Qt::lightGray;
    }



    // --- item létrehozása vagy újrafelhasználása ---
    auto* item = _table->item(row, col);
    if (!item) {
        item = new QTableWidgetItem();
        _table->setItem(row, col, item);
    }

    item->setText(symbol);
    item->setTextAlignment(Qt::AlignCenter);
    item->setBackground(bg);
    item->setForeground(QColor("#222222"));

    // aktuális cella jelölése
    // aktív flag törlése minden cellán
    item->setData(Qt::UserRole, QVariant());

    // aktuális cella jelölése
    int activeRow = _active.currentMaterialIndex;
    int activeCol = _active.currentColumnIndex + 1;

    if (row == activeRow && col == activeCol)
        item->setData(Qt::UserRole, "active");

    item->setData(Qt::UserRole + 2, _active.currentMaterialIndex);
    item->setData(Qt::UserRole + 3, _active.currentColumnIndex + 1);


    if (!isBom)
        item->setData(Qt::UserRole + 1, "nonBom");
}

void SeriesMatrixView::addRow(const QUuid& matId)
{
    // 1) új sor index
    int row = _table->rowCount();
    _table->insertRow(row);

    // 2) anyag neve
    auto* mat = MaterialRegistry::instance().findById(matId);
    QString name = mat ? mat->toDisplay() : "(ismeretlen anyag)";

    // 3) sor első cellája (anyag neve)
    auto* item = new QTableWidgetItem(name);
    item->setBackground(Qt::lightGray);

    QFont f = item->font();
    f.setPointSize(8);
    item->setFont(f);
    item->setForeground(QColor("#333333"));

    _table->setItem(row, 0, item);

    // 4) cellák frissítése az új sorban
    for (int col = 1; col < _table->columnCount(); ++col) {
        updateCell(row, col);
    }
}

int SeriesMatrixView::findColumnIndex(const QString& ref) const
{
    for (int col = 1; col < _table->columnCount(); ++col) {
        if (_table->horizontalHeaderItem(col)->text() == ref)
            return col;
    }
    return -1;
}

NamedColor SeriesMatrixView::effectiveColorForReference(const QString& ref) const
{
    NamedColor color;

    for (const auto& r : _full) {
        if (r.externalReference == ref &&
            r.requiredColor.isValid() &&
            !r.requiredColor.code().trimmed().isEmpty()) {
            color = r.requiredColor;
            break;
        }
    }

    if (!color.isValid() || color.code().trimmed().isEmpty()) {
        for (const auto& p : _filledCache) {
            if (p.first != ref)
                continue;

            auto* m = MaterialRegistry::instance().findById(p.second);
            if (m && m->color.isValid() &&
                !m->color.code().trimmed().isEmpty()) {
                color = m->color;
                break;
            }
        }
    }

    return color;
}




void SeriesMatrixView::jumpToFirstReference()
{
    if (_active.order.isEmpty())
        return;

    QString first = _active.order.first();

    int col = findColumnIndex(first);
    if (col == -1)
        return;

    _active.currentColumnIndex = col - 1;
    _active.currentMaterialIndex = 0;

    // teljes oszlop frissítés
    for (int row = 0; row < _active.bomMaterials.size(); ++row)
        updateCell(row, col);
}


