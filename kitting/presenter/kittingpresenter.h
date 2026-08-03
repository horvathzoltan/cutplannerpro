#pragma once

#include <QObject>
#include <QVector>
#include <QUuid>

#include "kitting/model/kittinginstruction.h"
#include "model/cutting/optimizer/optimizermodel.h"   // <-- a cutting optimizer modell kell


class MainWindow; // Előre deklaráljuk, hogy ne kelljen most includolni


class KittingPresenter : public QObject {
    Q_OBJECT

public:
    explicit KittingPresenter(MainWindow* view, QObject* parent = nullptr);

    void GenerateKittingInstructions();

    // const QVector<KittingInstruction>& getInstructions() const {
    //     return _instructions;
    // }

    void ExportKittingInstructions();
private:
    MainWindow* _view;
    Cutting::Optimizer::OptimizerModel* _optimizer;

    QVector<KittingInstruction> _instructions;   // <-- csak ez kell

    QMap<QString, QVector<KittingInstruction> > groupByExternalRef(const QVector<KittingInstruction> &list);
    //QMap<QString, int> countByMaterial(const QVector<KittingInstruction> &items);

    struct MaterialSummary {
        double quantity;
        QString unit;
    };
    QMap<QString, KittingPresenter::MaterialSummary> countByMaterial(const QVector<KittingInstruction> &items);
};
