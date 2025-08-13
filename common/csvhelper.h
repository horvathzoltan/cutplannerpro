#pragma once

#include <QString>

namespace CsvHelper {

inline QString escape(const QString& field, QChar delimiter = ';') {
    QString f = field;
    bool mustQuote = f.contains(delimiter) || f.contains(',')
                     || f.contains('"') || f.contains(' ')
                     || f.contains('\n') || f.contains('\r');
    f.replace("\"", "\"\""); // idézőjel duplázás
    return mustQuote ? "\"" + f + "\"" : f;
}

inline QString escape2(const QString& s) {
        QString v = s;
        v.replace("\"", "\"\"");     // idézőjelek duplázása
        return "\"" + v + "\"";      // teljes mező idézőjelezése
    };

} // namespace CsvHelper
