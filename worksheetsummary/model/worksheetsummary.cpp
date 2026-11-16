#include "worksheetsummary.h"

#include "meta_binders/csv/WorkSheetPresence_CsvBinding.h"


/*
#include "meta/meta.h"

private:
    static MetaData<WorkSheetSummary> _meta;
public:
    static MetaData<WorkSheetSummary>& Meta();
    static void MetaInit();
*/

MetaData<WorkSheetSummary> WorkSheetSummary::_meta;

MetaData<WorkSheetSummary>& WorkSheetSummary::Meta(){
    if(!_meta.isInitialized())
        MetaInit();

    return _meta;
}

void WorkSheetSummary::MetaInit()
{
    if(_meta.isInitialized())
        return;

    AddMetaBase(WorkSheetSummary);

    AddMetaField(filePath);
    AddMetaField(fileName);
    AddMetaField(fileHash);
    AddMetaField(presence);
    AddMetaField(orders);

    _meta.Bind<WorkSheetPresence_CsvBinding>("presence",BindContext::CsvExport);
    //_meta.Bind<MaterialBarcodeBinding>("materialId",BindContext::CsvExport);

    _meta.setInitialized();
    _meta.RegisterSelf();
}