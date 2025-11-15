// inputtablerowgenerator.h
#include "../../../model/cutting/plan/request.h"
#include "materials/model/material_master.h"
#include "materials/view/material_cell_generator.h"
#include "../../columnindexes/inputtable_columns.h"
#include "../tablerowviewmodel.h"

#include <QPushButton>
namespace Request::ViewModel::RowGenerator {

static constexpr auto RequestId_Key = "requestId";


inline TableRowViewModel generate(const Cutting::Plan::Request& request,
                                  QObject* receiver = nullptr) {
    TableRowViewModel vm;
    //vm.rowId = request.rowId.isNull() ? QUuid::createUuid() : request.rowId;

    const MaterialMaster* mat = MaterialRegistry::instance().findById(request.materialId);
    if (!mat) {};

    vm.rowId = request.requestId;


    // 🧩 Anyag + csoprt + barcode
    auto matCell = CellGenerators::materialCell(*mat);
    // 🎨 Alapszínek a csoport alapján
    QColor baseColor = matCell.background;
    QColor fgColor = matCell.foreground;
    vm.cells[InputTableColumns::Material] = matCell;

    // Material cell
    // vm.cells[InputTableColumns::Material] =
    //     TableCellViewModel::fromText(mat.name,
    //                                  QString("Barcode: %1\nColor: %2")
    //                                      .arg(mat.barcode)
    //                                      .arg(mat.color.name()),
    //                                  mat.color.color(),
    //                                  Qt::black);

    // Owner + ExternalRef
    vm.cells[InputTableColumns::Owner] =
        TableCellViewModel::fromText(request.ownerName,"",baseColor, fgColor, true);
    vm.cells[InputTableColumns::ExternalRef] =
        TableCellViewModel::fromText(request.externalReference, "",baseColor, fgColor, true);

    // Length + Tolerance
    vm.cells[InputTableColumns::Length] =
        TableCellViewModel::fromText(QString::number(request.requiredLength),"",baseColor, fgColor, true);
    QString tolStr = request.requiredTolerance.has_value()
                         ? request.requiredTolerance->toCsvString()
                         : "";
    vm.cells[InputTableColumns::Tolerance] =
        TableCellViewModel::fromText(tolStr,"",baseColor, fgColor, true);

    QString handlerTxt = HandlerSideUtils::toString(request.handlerSide);
    vm.cells[InputTableColumns::HandlerSide] =
        TableCellViewModel::fromText(handlerTxt,"",baseColor, fgColor, true);

    // Quantity
    vm.cells[InputTableColumns::Quantity] =
        TableCellViewModel::fromText(QString::number(request.quantity), "",baseColor, fgColor, true);

    // Color
    auto colorCell = CellGenerators::requestColorCell(request.requiredColor, mat->color);
    colorCell.background = baseColor;
    colorCell.foreground = fgColor;
    vm.cells[InputTableColumns::Color] = colorCell;
        //TableCellViewModel::fromText(request.requiredColorName, "",baseColor, fgColor, true);

    // Measurement
    vm.cells[InputTableColumns::Measurement] =
        TableCellViewModel::fromText(request.isMeasurementNeeded ? "✔" : "", "",baseColor, fgColor, true);

    // Actions – gombok panelben
    QPushButton* btnUpdate = new QPushButton("✏️");
    QPushButton* btnDelete = new QPushButton("🗑️");

    //btnUpdate->setProperty(RequestId_Key, request.requestId);
    //btnDelete->setProperty(RequestId_Key, request.requestId);

    // Kapcsolások
    QObject::connect(btnUpdate, &QPushButton::clicked, receiver, [receiver, id=request.requestId]() {
        QMetaObject::invokeMethod(receiver, "editRequested", Qt::QueuedConnection,
                                  Q_ARG(QUuid, id));
    });
    QObject::connect(btnDelete, &QPushButton::clicked, receiver, [receiver, id=request.requestId]() {
        QMetaObject::invokeMethod(receiver, "deleteRequested", Qt::QueuedConnection,
                                  Q_ARG(QUuid, id));
    });

    // 🎛️ Panel + layout
    auto* actionPanel = new QWidget();
    auto* layout = new QHBoxLayout(actionPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(btnUpdate);
    layout->addWidget(btnDelete);

    // Cellába a panel kerül
    vm.cells[InputTableColumns::Actions] =
        TableCellViewModel::fromWidget(actionPanel, "Műveletek");

    // akár panelbe rakva a két gombot

    return vm;
}
}
