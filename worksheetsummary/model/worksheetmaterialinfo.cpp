#include "worksheetmaterialinfo.h"

#include <meta_binders/csv/Material_BarcodeBinding.h>


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

    _meta.Bind<Material_BarcodeBinding>("materialId",BindContext::CsvExport);

    _meta.setInitialized();
    _meta.RegisterSelf();
}