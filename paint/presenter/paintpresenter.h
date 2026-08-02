#pragma once

#include <QObject>


class MainWindow;

class PaintPresenter : public QObject {

public:
    explicit PaintPresenter(MainWindow* view, QObject *parent = nullptr);


    void ExportPaintPlan();

private:
    MainWindow* view;
};


