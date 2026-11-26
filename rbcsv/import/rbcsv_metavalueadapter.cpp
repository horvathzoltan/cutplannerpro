#include "rbcsv_metavalueadapter.h"

QList<MetaValue>
NodeToMetaValues(
    const RbCsvImportNode& node)
{
    QList<MetaValue> result;

    for(auto it = node.record.values.begin();
         it != node.record.values.end();
         ++it)
    {
        result.append(
            MetaValue(
                it.key(),           // meta field name
                QString(),          // wcode egyelőre nem ismert
                QVariant(
                    it.value()),    // QString
                RefType::None));
    }

    return result;
}