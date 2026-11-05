#pragma once

#include <QString>
#include <QStringList>

#include "meta/imetadata.h"
#include "meta/metafield.h"
#include "meta/discoverypath.h"

/*
- sororientált
- ugyanazt a szerkezetet követi mint az adat
- a mezőnevek az igazságforrásból (Meta) jönnek
- a composition mezők helyőrzőként jelennek meg
- ugyana

A gyerek szakítja félbe a szülőt. A gyerek késleltetve kerül kiírásra.zok a szekcióhatárok lesznek benne mint az adatban
A rekord mindig teljes egészében kiíródik. A compositionök csak helyőrzőként jelennek meg benne. A hozzájuk tartozó szekciók a rekord után, alárendelten kerülnek kiírásra.

*/

class RbCsvSchemaWorker
{
private:
    QString _text;
    QStringList _currentRow;

public:

    const QString& text() const
    {
        return _text;
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
        FlushRow();
    }

    void Field(
        IMetaData*,
        const MetaFieldBase& field,
        const DiscoveryPath&)
    {
        AppendField(field.name);
    }

    void EnterComposition(
        IMetaData*,
        const MetaFieldBase& field,
        IMetaData*,
        const DiscoveryPath&)
    {
        AppendField("[" + field.name + "]");

        FlushRow();

        _text += "\n";
        _text += field.name;
        _text += "\n";
    }

    void LeaveComposition(
        IMetaData*,
        const MetaFieldBase& field,
        IMetaData*,
        const DiscoveryPath&)
    {
        FlushRow();

        _text += "\nEND # ";
        _text += field.name;
        _text += "\n";
    }

private:

    void AppendField(const QString& value)
    {
        _currentRow.append(value);
    }

    void FlushRow()
    {
        if(_currentRow.isEmpty())
            return;

        _text += _currentRow.join(';');
        _text += "\n";

        _currentRow.clear();
    }
};