#pragma once

#include <QHash>

#include "meta/discoverypath.h"
#include "meta/imetadata.h"
#include "meta/metafield.h"
#include "rbcsv/model/rbcsv_schema.h"

class RbCsvExportPlanBuilder
{
private:

    RbCsvSchema _plan;

public:

    RbCsvExportPlanBuilder()
    {
        _plan.root.sectionName = "root";
    }

    const RbCsvSchema& plan() const
    {
        return _plan;
    }

    void EnterType(
        IMetaData*,
        const DiscoveryPath&)
    {
    }

    void LeaveType(
        IMetaData*,
        const DiscoveryPath&)
    {
    }

    void Field(
        IMetaData*,
        const MetaField& field,
        const DiscoveryPath& path)
    {
        CurrentSection(path)
        .fields
            .append(&field);
    }

    void EnterComposition(
        IMetaData*,
        const MetaField& field,
        IMetaData*,
        const DiscoveryPath& path)
    {
        QString ps = PathString(path);

        QString childPath =
            ps.isEmpty() ? field.name : ps + "/" + field.name;

        RbCsvSchemaSection child;
        child.sectionName = field.name;

        _plan.sections.insert(
            childPath,
            child);

        auto& cs = CurrentSection(path);

        cs.children.append(childPath);
        cs.fields.append(&field);
    }

    void LeaveComposition(
        IMetaData*,
        const MetaFieldBase&,
        IMetaData*,
        const DiscoveryPath&)
    {
    }

private:

    RbCsvSchemaSection& CurrentSection(
        const DiscoveryPath& path)
    {
        if(path.Depth() == 0)
            return _plan.root;

        QString ps = PathString(path);
        return _plan.sections[ps];
    }
};