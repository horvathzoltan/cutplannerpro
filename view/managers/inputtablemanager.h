#pragma once

#include <QTableWidget>
//#include <optional>
#include "model/cuttingplanrequest.h"

class InputTableManager: public QObject {  // 🔧 QObject öröklés!
    Q_OBJECT                              // ✨ Qt metaobjektum makró!

public:
    explicit InputTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const CuttingPlanRequest& request);
    void removeRowByRequestId(const QUuid &requestId);

    void updateTableFromRegistry();

    void updateRow(const CuttingPlanRequest& updated); // ⬅️ új metódus

    void clearTable();
signals:
    void deleteRequested(const QUuid& requestId);
    void editRequested(const QUuid& requestId);

private:
    QTableWidget* table;
    QWidget* parent;

    static constexpr int CuttingRequestIdRole = Qt::UserRole + 1;

public:    

    static constexpr int ColName     = 0; // Anyag neve
    static constexpr int ColLength   = 1; // Hossz
    static constexpr int ColQty      = 2; // Mennyiség
    static constexpr int ColAction   = 3; // Művelet (pl. törlés gomb)
    static constexpr int ColMetaRowSpanStart = 0; // Alsó összefoglaló sor – kiterjesztés kezdete

};


