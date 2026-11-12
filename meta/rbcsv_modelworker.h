#pragma once

#include <QHash>

#include "meta/discoverypath.h"
#include "meta/imetadata.h"
#include "meta/metafield.h"
#include "rbcsv/model/rbcsv_exportplan.h"

class RbCsvExportPlanBuilder
{
private:

    RbCsvExportPlan _plan;

public:

    RbCsvExportPlanBuilder()
    {
        _plan.root.sectionName = "root";
    }

    const RbCsvExportPlan& plan() const
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
        QString childPath =
            path.Path().isEmpty()
                ? field.name
                : path.Path() + "/" + field.name;

        RbCsvExportSection child;
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

    RbCsvExportSection& CurrentSection(
        const DiscoveryPath& path)
    {
        if(path.Depth() == 0)
            return _plan.root;

        return _plan.sections[path.Path()];
    }
};