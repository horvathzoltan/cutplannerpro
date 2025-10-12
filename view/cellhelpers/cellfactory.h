// #pragma once

// #include "view/viewmodels/tablecellviewmodel.h"
// #include <QString>
// #include <QColor>

// namespace CellFactory {

// /**
//  * @brief Egyszerű szöveges cella létrehozása.
//  *
//  * @param text       A cellában megjelenő szöveg
//  * @param tooltip    Tooltip szöveg (opcionális)
//  * @param background Háttérszín
//  * @param foreground Betűszín
//  * @param isReadOnly Ha true → nem szerkeszthető cella
//  */
// inline TableCellViewModel textCell(const QString& text,
//                                    const QString& tooltip = {},
//                                    const QColor& background = Qt::white,
//                                    const QColor& foreground = Qt::black,
//                                    bool isReadOnly = true)
// {
//     TableCellViewModel cell;
//     cell.text = text;
//     cell.tooltip = tooltip;
//     cell.background = background;
//     cell.foreground = foreground;
//     cell.isReadOnly = isReadOnly;
//     return cell;
// }

// } // namespace CellFactory
