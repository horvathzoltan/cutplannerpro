#pragma once
#include <QFontDatabase>
#include <QStandardPaths>
#include <QFileInfo>
#include "common/logger.h"
#include "common/fontversionhelper.h"   // <-- ezt add hozzá

class FontAuditHelper
{
public:
    static QStringList audit()
    {
        QStringList warnings;

        zInfo("🎨 Emoji font audit:");
        auditFont("Segoe UI Emoji",                warnings);
        auditFont("Noto Color Emoji",      warnings);
        auditFont("Apple Color Emoji",      warnings);

        zInfo("📏 Monospaced font audit:");
        auditFont("Courier New",                        warnings);
        auditFont("Noto Sans Mono",       warnings);

        return warnings;
    }

private:
    static void auditFont(const QString& family, QStringList& warnings)
    {
        if (!QFontDatabase::hasFamily(family)) {
            zInfo(QString("  • %1: Nincs").arg(family));
            return;
        }

        QString file = FontVersionHelper::findFontFileForFamily(family);
        if (file.isEmpty()) {
            zInfo(QString("  • %1: OK (fájl nem található)").arg(family));
            warnings << QString("%1 font fájlja nem található.").arg(family);
            return;
        }

        QString raw = FontVersionHelper::getFontVersion(file);
        FontVersionHelper::FontVersionInfo vi = FontVersionHelper::parseVersionString(raw);

        QString vendorText = vi.vendor.isEmpty() ? "n/a" : vi.vendor;

        zInfo(QString("  • %1: OK (verzió: %2, vendor: %3)")
                  .arg(family)
                  .arg(vi.version)
                  .arg(vendorText));
    }

};
