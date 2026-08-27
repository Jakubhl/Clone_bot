#pragma once

#include <QDialog>
#include "JsonHandler.h"
#include "ui_CloneBotWindow.h"

class CloneBotWindow : public QDialog
{
    Q_OBJECT

public:
    explicit CloneBotWindow(QWidget *parent = nullptr);
    ~CloneBotWindow();

private:
    Ui::Dialog *ui;
    void on_btnSave_clicked();
    void on_btnRefresh_clicked();
    AppSettings CollectSettingsFromUi() const;
    void ApplySettingsToUi(/*const AppSettings& s*/);
    AppConfig* _config;
    void RunClone();
    void CloseEvent();
    void CleanupOutputDir(QDir* batDir);
    QStringList CloneBotWindow::GetBranchList();
    enum ChosenOption 
    {
        onlyClone,
        cloneDepsGen,
        cloneDepsGenBuild,
        cloneDepsGenBuildOpen,
        cloneDepsGenBuildOpenRun
    };
    // QStringList branchListVect;

};