#ifndef FILEHELPER_H
#define FILEHELPER_H

#include "common/logger.h"
#include <QTextStream>
#include <QList>
#include <QVector>
#include <QString>
#include <QMap>

class FileHelper {
public:
    // Fő CSV parser metódus: escape karakterekkel, többsoros cellákkal
    static QList<QVector<QString>> parseCSV(QTextStream *st, const QChar& separator = ';');

    static bool isCsvWithOnlyHeader(const QString &filePath);

    struct SeparatorResult {
        QChar separator;
        bool isSingleColumn;
        bool hasError = false;
        bool hasWarning = false;
        QMap<QChar, QStringList> separatorWarnings;
        QStringList globalWarnings;

        QString toString()
        {
            QString r;

            if(hasError || hasWarning)
            {
                QString a = logErrs(globalWarnings);
                if(!a.isEmpty()) r+=a;

                for (auto it = separatorWarnings.begin(); it != separatorWarnings.end(); ++it)
                {
                    const QChar sep = it.key();
                    const QStringList& ws = it.value();

                    a = logErrs(ws);
                    if(a.isEmpty()) continue;

                    r+= L("szeparátor: '%1'\n").arg(sep);;
                    r+=a;
                }
            }
            return r;
        }

    private:
        QString logErrs(const QStringList& wList){
            if(wList.isEmpty()) return {};

            return wList.join("\n");
        };
    };

    static SeparatorResult detectSeparatorSmart(QTextStream *st);
private:
    // Egyetlen cella értelmezése: escape karakterek feldolgozása
    static QString parseCell(const QString& rawCell);
};

#endif // FILEHELPER_H
