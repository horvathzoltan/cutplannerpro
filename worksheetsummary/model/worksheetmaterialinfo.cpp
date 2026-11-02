#include "worksheetmaterialinfo.h"


MetaData<WorkSheetMaterialInfo> WorkSheetMaterialInfo::_meta;

MetaData<WorkSheetMaterialInfo>& WorkSheetMaterialInfo::Meta(){
    if(!_meta.isInitialized())
        MetaInit();

    return _meta;
}

void WorkSheetMaterialInfo::MetaInit()
{
    if(_meta.isInitialized())
        return;

    AddMetaBase(WorkSheetMaterialInfo);

    AddMetaField(materialId);
    AddMetaField(cutLengthMm);

    _meta.setInitialized();

    _meta.RegisterSelf();
}