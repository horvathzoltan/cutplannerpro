// #include "rbcsvimportdump.h"

// namespace
// {

// void DumpNode(
//     QString& out,
//     const RbCsvImportNode& node,
//     int depth)
// {
//     QString indent(
//         depth * 2,
//         ' ');

//     out +=
//         indent
//         + "["
//         + node.sectionName
//         + "]\n";

//     QString text =
//         node.record.ToString();

//     if(!text.isEmpty())
//     {
//         out +=
//             indent
//             + "  "
//             + text
//             + "\n";
//     }

//     for(auto it =
//          node.children.begin();
//          it != node.children.end();
//          ++it)
//     {
//         out +=
//             indent
//             + QString(
//                   "  <%1> count=%2\n")
//                   .arg(it.key())
//                   .arg(it.value().size());

//         for(const auto& child :
//              it.value())
//         {
//             DumpNode(
//                 out,
//                 child,
//                 depth + 2);
//         }
//     }
// }

// }

// QString RbCsvImportDump::ToString( const RbCsvImportNode& node)
// {
//     QString out;

//     DumpNode(out, node, 0);

//     return out;
// }