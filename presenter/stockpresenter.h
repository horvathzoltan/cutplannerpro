#pragma once

#include <QObject>
#include "model/stockentry.h"

class MainWindow;

class StockPresenter : public QObject {
    Q_OBJECT

public:
    explicit StockPresenter(MainWindow* view, QObject* parent = nullptr);

    void findMaterial(const QUuid& materialId, int minLength, int maxLength);
    //void materialChosen(const StockEntry& entry);
    void filterStockByStorage(const QUuid &storageId);
    QSet<QUuid> collectSubtreeStorageIds(const QUuid &rootId);
signals:
    void highlightLeftover(const QUuid& entryId);
    void highlightStock(const QUuid& entryId);
    void showNotFoundMessage(const QString& msg);

private:
    MainWindow* view;
};
