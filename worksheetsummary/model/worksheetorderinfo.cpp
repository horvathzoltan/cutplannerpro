#include "worksheetorderinfo.h"

/*
#include "meta/meta.h"

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

    AddMetaComposition(
        materials,
        WorkSheetMaterialInfo);

    _meta.setInitialized();
    _meta.RegisterSelf();
}