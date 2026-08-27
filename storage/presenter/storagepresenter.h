#pragma once

#include "storage/model/storageentry.h"
#include <QObject>
#include <QString>

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
private:
    MainWindow* _view;
};
