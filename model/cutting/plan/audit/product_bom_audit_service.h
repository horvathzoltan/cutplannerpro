#pragma once

#include <QVector>
#include <QString>
#include <QHash>
#include "model/cutting/plan/request.h"
#include "naphalo_audit_types.h"

struct ProductBomAuditMessage {
    QString ref;
    QString text;
    bool isError = false;
};

struct ProductBomAuditSummary {
    QHash<QString, CountPerType> perCustomer;
};

struct BomAuditEntry {
    QString ref;        // tételszám
    QString category;   // "HEAD", "LENGTH", "ROLE", "BOM", "EXTRA"
    QString testName;   // konkrét teszt neve
    bool passed;        // átment / elbukott
    QString expected;   // elvárt érték (szöveg)
    QString actual;     // kapott érték (szöveg)
    QString details;    // opcionális részletek
};

struct BomAuditEntries{
private:
    QVector<BomAuditEntry> entries;

public:
    void add(const QString& ref,
        const QString& category,
        const QString& testName,
        bool passed,
        const QString& expected,
        const QString& actual,
        const QString& details = QString()){
            BomAuditEntry e;
            e.ref      = ref;
            e.category = category;
            e.testName = testName;
            e.passed   = passed;
            e.expected = expected;
            e.actual   = actual;
            e.details  = details;
            entries << e;
        }

    void addAll(const QVector<BomAuditEntry>& list)
    {
        for (const auto& e : list)
            entries << e;
    }

    bool hasError_Global() const
    {
        for (const auto& e : entries)
            if (!e.passed)
                return true;
        return false;
    }

    bool hasError_ByRef(const QString& ref) const
    {
        for (const auto& e : entries)
            if (e.ref == ref && !e.passed)
                return true;
        return false;
    }

    const QVector<BomAuditEntry>& readAll() const
    {
        return entries;
    }
};

struct ProductBomAuditResult {
    BomAuditEntries entries;
    ProductBomAuditSummary summary;
};

class ProductBomAuditService {
public:
    static ProductBomAuditResult run(const QVector<Cutting::Plan::Request>& all);
    static QStringList BOM_audit();
private:
    static QHash<QString, QVector<Cutting::Plan::Request>>
                groupByExternalRef(const QVector<Cutting::Plan::Request> &all);
};


