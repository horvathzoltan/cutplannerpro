#include "rowtracker2.h"

RowTracker2::RowTracker2(
    QTableWidget* table)
    : _table(table)
{
}

QTableWidgetItem*
RowTracker2::ensureHeader(
    QTableWidget* table,
    int row)
{
    auto* hdr =
        table->verticalHeaderItem(row);

    if (!hdr)
    {
        hdr =
            new QTableWidgetItem();

        table->setVerticalHeaderItem(
            row,
            hdr);
    }

    return hdr;
}

void RowTracker2::registerRow(
    int row,
    const QUuid& id,
    const QVariant& extra)
{
    if (!_table)
        return;

    auto* hdr =
        ensureHeader(
            _table,
            row);

    hdr->setData(
        IdRole,
        id);

    hdr->setData(
        ExtraRole,
        extra);
}

std::optional<QUuid>
RowTracker2::idOfRow(
    int row) const
{
    if (!_table)
        return std::nullopt;

    auto* hdr =
        _table->verticalHeaderItem(
            row);

    if (!hdr)
        return std::nullopt;

    QUuid id =
        hdr->data(
               IdRole)
            .toUuid();

    if (id.isNull())
        return std::nullopt;

    return id;
}

std::optional<int>
RowTracker2::rowOf(
    const QUuid& id) const
{
    if (!_table)
        return std::nullopt;

    for (int row = 0;
         row < _table->rowCount();
         ++row)
    {
        auto rowId =
            idOfRow(row);

        if (rowId &&
            *rowId == id)
        {
            return row;
        }
    }

    return std::nullopt;
}

std::optional<QVariant>
RowTracker2::extraOfRow(
    int row) const
{
    if (!_table)
        return std::nullopt;

    auto* hdr =
        _table->verticalHeaderItem(
            row);

    if (!hdr)
        return std::nullopt;

    QVariant v =
        hdr->data(
            ExtraRole);

    if (!v.isValid())
        return std::nullopt;

    return v;
}

std::optional<QVariant>
RowTracker2::extraOfId(
    const QUuid& id) const
{
    auto row =
        rowOf(id);

    if (!row)
        return std::nullopt;

    return extraOfRow(*row);
}

bool RowTracker2::contains(
    const QUuid& id) const
{
    return rowOf(id).has_value();
}
