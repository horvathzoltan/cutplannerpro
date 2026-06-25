#pragma once

#include <QHash>
#include <QString>
#include <QVector>
#include "model/cutting/plan/request.h"

struct MaterialAggregate {
    int count = 0;       // darabszám
    int totalLength = 0; // összes hossz (mm)
};

enum class NaphaloGroup {
    Lab,
    LabBetet,
    Tok,
    TokFed,
    CipzarosZaro,
    SinesZaro
};

static const inline QHash<NaphaloGroup, QVector<QString>> GROUP_PREFIXES = {
    { NaphaloGroup::Lab,          { "NP-CL", "NP-SL" } },
    { NaphaloGroup::LabBetet,     { "NP-CLB", "NP-CLBR" } },
    { NaphaloGroup::Tok,          { "NP-T" } },
    { NaphaloGroup::TokFed,       { "NP-TF" } },
    { NaphaloGroup::CipzarosZaro, { "NP-CZ" } },
    { NaphaloGroup::SinesZaro,    { "NP-SZ" } }
};


enum class NaphaloFamily {
    Tok,
    Lab,
    Zaro
};

static const inline QHash<NaphaloFamily, QVector<NaphaloGroup>> FAMILY_GROUPS = {
    { NaphaloFamily::Tok,  { NaphaloGroup::Tok, NaphaloGroup::TokFed } },
    { NaphaloFamily::Lab,  { NaphaloGroup::Lab, NaphaloGroup::LabBetet } },
    { NaphaloFamily::Zaro, { NaphaloGroup::CipzarosZaro, NaphaloGroup::SinesZaro } }
};

class NaphaloMaterialAggregator {
public:
    static QHash<QString, MaterialAggregate>
    aggregate(const QVector<Cutting::Plan::Request>& list);

    static int sumPrefix(const QHash<QString, MaterialAggregate>& map, const QString& prefix);
    static int sumGroup(const QHash<QString, MaterialAggregate> &map, NaphaloGroup group);
    static int sumFamily(const QHash<QString, MaterialAggregate> &map, NaphaloFamily family);
};
