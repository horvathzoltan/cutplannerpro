// #include "worksheetsummary/model/worksheetsummary.h"
// template<typename T>
// struct EnumTraits;

// template<>
// struct EnumTraits<WorkSheetPresence>
// {
//     static QString ToString(
//         WorkSheetPresence v)
//     {
//         return
//             WorkSheetPresenceUtils::
//             toCsv(v);
//     }

//     static WorkSheetPresence Parse(
//         const QString& s)
//     {
//         return
//             WorkSheetPresenceUtils::
//             fromCsv(s);
//     }
// };

// template<typename EnumType>
// void RegisterEnumConverters()
// {
//     static bool done = false;

//     if(done)
//         return;

//     done = true;

//     QMetaType::registerConverter<
//         EnumType,
//         QString>(
//         [](EnumType v)
//         {
//             return EnumTraits<
//                 EnumType>::ToString(v);
//         });

//     QMetaType::registerConverter<
//         QString,
//         EnumType>(
//         [](const QString& s)
//         {
//             return EnumTraits<
//                 EnumType>::Parse(s);
//         });
// }