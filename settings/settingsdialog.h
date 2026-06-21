#pragma once
#include "settings/settingsmeta.h"
#include <QDialog>
#include <QMap>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

private:
    void accept() override;
    void reject() override;

private:
    Ui::SettingsDialog* ui;

    // minden meta.key → widget pointer
    QMap<QString, QWidget*> editors;

    void buildTabs();
    void buildCategoryTab(SettingCategory category, QWidget* tab);
    void handleAction(const QString &key);
    void resetHeaders();
    void resetWindow();
    void resetCounters();
};
