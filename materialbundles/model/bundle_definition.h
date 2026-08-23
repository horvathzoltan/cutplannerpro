#pragma once
#include <QUuid>
#include <QString>
#include <QVector>

struct BundleComponent {
    QUuid materialId;
    int count = 1;
};

struct BundleDefinition {
    QUuid id;               ///< A bundle saját UUID-je
    QString code;           ///< Emberi azonosító (CSV-ből)
    QString name;           ///< Emberi név
    QVector<BundleComponent> components;

    std::optional<double> computedLength_mm() const;
};
