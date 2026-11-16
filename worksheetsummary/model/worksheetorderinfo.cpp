#include "worksheetorderinfo.h"

#include "meta_binders/csv/NamedColor_CsvBinding.h"
#include "meta_binders/csv/SizeCalcMode_CsvBinding.h"
#include "meta_binders/csv/SurfaceType_CsvBinding.h"

/*
#include "meta/meta.h"
#include "meta_binders/csv/NamedColor_CsvBinding.h"

private:
    static MetaData<WorkSheetSummary> _meta;
public:
    static MetaData<WorkSheetSummary>& Meta();
    static void MetaInit();
*/

MetaData<WorkSheetOrderInfo> WorkSheetOrderInfo::_meta;

MetaData<WorkSheetOrderInfo>& WorkSheetOrderInfo::Meta(){
    if(!_meta.isInitialized())
        MetaInit();

    return _meta;
}

void WorkSheetOrderInfo::MetaInit()
{
    if(_meta.isInitialized())
        return;

    AddMetaBase(WorkSheetOrderInfo);

    AddMetaField(externalReference);
    AddMetaField(ownerName);

    AddMetaField(quantity);

    AddMetaField(fullWidthMm);
    AddMetaField(fullHeightMm);

    AddMetaField(dueDate);

    AddMetaField(productTypeId);
    AddMetaField(productSubtypeId);

    AddMetaField(calcMode);

    AddMetaField(requestCount);

    AddMetaField(completedCount);
    AddMetaField(uncompletedCount);
    AddMetaField(undefinedCount);

    AddMetaField(requiredColor);
    AddMetaField(requiredSurface);

    AddMetaField(materials);

    _meta.Bind<SizeCalcMode_CsvBinding>("calcMode",BindContext::CsvExport);
    _meta.Bind<SurfaceType_CsvBinding>("requiredSurface",BindContext::CsvExport);
    _meta.Bind<NamedColor_CsvBinding>("requiredColor",BindContext::CsvExport);

    _meta.setInitialized();
    _meta.RegisterSelf();
}