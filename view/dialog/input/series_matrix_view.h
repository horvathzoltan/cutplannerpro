#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>

#include "series_state.h"
#include "../../../presenter/CuttingPresenter.h"
#include "../../../model/cutting/plan/request.h"

class SeriesMatrixView : public QWidget
{
    Q_OBJECT

public:
    explicit SeriesMatrixView(QWidget* parent,
                              CuttingPresenter* presenter);

    void updateMatrix(const ActiveSeries& active,
                      const QVector<Cutting::Plan::Request>& fullSeries);

    void setActiveReference(const QString &ref);
    void refreshAfterAdd(const QString &ref);

public slots:
    void onSeriesContextChanged(const QString& owner,
                                const QString& externalRefPrefix);

signals:
    void matrixClosed();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    CuttingPresenter* _presenter = nullptr;

    ActiveSeries _active;
    QVector<Cutting::Plan::Request> _full;

    QTableWidget* _table = nullptr;

    void rebuildMatrix();
    void buildHeaders();
    void buildRows();
    void fillCells();

    QSet<QUuid> _actualMaterials;     // sorozatban ténylegesen előforduló anyagok
    QSet<QUuid> _bomMaterialsSet;     // BOM anyagok halmaza

    void computeMaterialSets();
    QVector<QUuid> generateBomMaterials(const Cutting::Plan::Request &req);
    const Cutting::Plan::Request *findRequestByExternalRef(const QString &ref) const;
    };
