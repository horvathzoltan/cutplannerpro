#pragma once

#include "storage/model/storageentry.h"
#include <QObject>
#include <QString>
#include <stock/utils/stocklistform_utils.h>

class MainWindow;

class StoragePresenter : public QObject {
    Q_OBJECT
public:
    explicit StoragePresenter(MainWindow* view, QObject* parent = nullptr);

    void exportStorageLabelPdf(const QUuid& storageId);
    void exportMultipleLabels(const QList<StorageEntry*>& entries);

    void exportStockIntakeForm(const QUuid &storageId);
    void exportStockListPdf(const QUuid &storageId);
    void exportMaterialBarcodeList();
    void exportStorageBarcodeList();
    void exportGlobalStockListPdf();
private:
    MainWindow* _view;
    static QSet<QUuid> findCommonMaterials(const QList<StockListFormUtils::AggregatedMaterial> &mats);
    static QList<StockListFormUtils::AggregatedMaterial> buildGroupedList(const QList<StockListFormUtils::AggregatedMaterial> &mats);
};
