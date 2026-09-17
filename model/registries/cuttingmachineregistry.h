#pragma once

#include <QVector>
#include <QString>
#include "../cutting/cuttingmachine.h"

class CuttingMachineRegistry {
private:
    CuttingMachineRegistry() = default;
    CuttingMachineRegistry(const CuttingMachine&) = delete;

    QVector<CuttingMachine> _data; // 📦 Betöltött vágógépek listája

public:
    static CuttingMachineRegistry& instance();

    bool registerData(const CuttingMachine& machine);  // név alapján egyediség

    void setData(const QVector<CuttingMachine>& v) { _data = v;}
    const QVector<CuttingMachine>& readAll() const { return _data; }
    const CuttingMachine* findByName(const QString& name) const;
    const CuttingMachine* findByBarcode(const QString& barcode) const;

    bool isBarcodeUnique(const QString& barcode) const;

    bool contains(const QString& name) const;

    bool isEmpty() const { return _data.isEmpty(); }
    void clear();  // teszteléshez
    const CuttingMachine *findById(const QUuid &id) const;

    int size() const { return _data.size(); }
};
