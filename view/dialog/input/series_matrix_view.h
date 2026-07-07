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

    void clearBomCache();

    void addFilledCell(const QString& ref, const QUuid& matId);
    void removeFilledCell(const QString& ref, const QUuid& matId);
    void updateFilledCell(const QString& oldRef, const QUuid& oldMat,
                          const QString& newRef, const QUuid& newMat);

    void addColumn(const QString& ref);
    void updateCell(int row, int col);   // a következő lépésben implementáljuk

    void addRow(const QUuid& matId);

    void setActiveSeries(const ActiveSeries& s) {_active = s;}
// public slots:
//     void onSeriesContextChanged(const QString& owner,
//                                 const QString& externalRefPrefix);

    QVector<QUuid> buildSectionedBomList();
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

    QHash<QPair<QUuid, QUuid>, QVector<QUuid>> _bomCache;
    inline QPair<QUuid, QUuid> bomKey(const Cutting::Plan::Request& req) const {
        return qMakePair(req.productTypeId, req.productSubtypeId);
    }

    QSet<QPair<QString, QUuid>> _filledCache;

    int findColumnIndex(const QString& ref) const;

};


