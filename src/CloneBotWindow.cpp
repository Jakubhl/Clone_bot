#include "CloneBotWindow.h"
// #include "ui_CloneBotWindow.h"
#include <QColorDialog>
#include <QTextStream>
#include <QProcess>
#include <QThread>
#include <QDateTime>

CloneBotWindow::CloneBotWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    // _config.Load();
    setWindowTitle("Clonebot");
    setWindowIcon(QIcon("../../pic/robot_ico.png"));

    ui->CB_sourceBranch->addItems({"vng1","master"});
    ui->CB_issueType->addItems({"app", "com","3p","tools","vat"});
    ui->CB_releaseDebug->addItems({"Release", "Debug"});
    ui->CB_action->addItems({"Only clone",
         "Clone + (get deps + gen project)",
         "Clone + (get deps + gen project + build)",
         "Clone + (get deps + gen project + build) + open",
         "Clone + (get deps + gen project + build) + open + run"});

    connect(ui->btnSave, &QPushButton::clicked, this, &CloneBotWindow::on_btnSave_clicked);
    connect(ui->btnRefreshBranch, &QPushButton::clicked, this, &CloneBotWindow::on_btnRefresh_clicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &CloneBotWindow::CloseEvent);
    connect(ui->btnCancelRun, &QPushButton::clicked, this, &CloneBotWindow::CloseEvent);
    connect(ui->btnRun, &QPushButton::clicked, this, &CloneBotWindow::RunClone);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &CloneBotWindow::CloseEvent);
    connect(ui->LE_name, &QLineEdit::textChanged,this, &CloneBotWindow::OnProjectNameChange);

    ApplySettingsToUi();

}

CloneBotWindow::~CloneBotWindow()
{   
    // clear temp batfiles
    delete ui;
}


void CloneBotWindow::RunClone()
{
    // int issueNumber = ui->LE_issueNumber->text().toInt();
    QString issueNumber = ui->LE_issueNumber->text();
    QString issueType = ui->CB_issueType->currentText();
    QString projectName = ui->LE_name->text();
    QString sourceBranch = ui->CB_sourceBranch->currentText();
    QString buildOption = ui->CB_releaseDebug->currentText();
    // QString chosenOption = ui->CB_action->currentText();
    int chosenOption = ui->CB_action->currentIndex();
    AppSettings s;
    s = CollectSettingsFromUi();


    //create path
    QString issueFolderName = QString("iss%1").arg(issueNumber);
    QString issueFolderPath = QDir(s.localRootFolder)
        .filePath(issueType + "/" + projectName + "/" + sourceBranch + "/" + issueFolderName);

    QDir().mkpath(issueFolderPath);

    // qDebug() << "Složka pro issue vytvořena:" << issueFolderPath;


    // BAT soubor
    // QString batPath = QDir(issueFolderPath).filePath("run_issue.bat");
    // QString batPath = QDir().filePath(QString("Run_%1_clone_issue%2.bat").arg(projectName).arg(issueNumber));
    // QString batPath = QCoreApplication::applicationDirPath() + QString("/Run_%1_clone_issue%2.bat").arg(projectName).arg(issueNumber);

    QString batDir = QCoreApplication::applicationDirPath() + "/output";
    // QDir().mkpath(batDir);

    QDir dir(batDir);

    // Nemohu provádět cleanup když budu pouštět více tasků paralelně
    // if (dir.exists()) {
    //     QStringList batFiles = dir.entryList({"*.bat"}, QDir::Files);

    //     for (const QString& file : batFiles) {
    //         dir.remove(file);
    //     }
    // } else {
        dir.mkpath(".");
    // }
    CleanupOutputDir(&dir);

    QString batPath = batDir + QString("/Run_%1_clone_issue%2.bat").arg(projectName).arg(issueNumber);
        
    QFile batFile(batPath);
    batFile.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream out(&batFile);
    out << "@echo off\n";
    out << "setlocal\n\n";
    out << "cd /d \"" << issueFolderPath << "\"\n\n";

    out << "echo ------------------------------------------------------------------------------\n";
    out << "echo Path:" << issueFolderPath << " created \n";
    out << "echo ------------------------------------------------------------------------------\n";
    out << "echo \n";

    // git
    // QString gitFullPath = s.gitRepository + issueType + "/" + projectName + ".git";
    QString gitFullPath = QString("%1%2/%3.git").arg(s.gitRepository).arg(issueType).arg(projectName);
    out << "echo clone " << gitFullPath << "\n";
    out << "git clone \"" << gitFullPath << "\" .\n";

    out << "if errorlevel 1 (\n"; // je-li už adresář zabraný jiným repo clonem
    out << "    echo CLONE FAILED!\n";
    // out << "    pause\n";
    // out << "    exit /b 1\n";
    out << ") else (\n";
    out << "echo ------------------------------------------------------------------------------\n";
    out << "echo Project: " << projectName << " cloned from repo: " << s.gitRepository << "\n";
    out << "echo ------------------------------------------------------------------------------\n";
    out << "echo \n";
    out << ")\n";

    out << "git checkout " << sourceBranch << "\n";
    out << "git fetch --all\n";
    if (ui->CHB_createBranch->isChecked()){
        out << "git checkout " << issueFolderName << " || git checkout -b " << issueFolderName << "\n\n";
    }

    out << "echo ------------------------------------------------------------------------------\n";
    out << "echo Branch switched to: " << issueFolderName << " (HEAD: " << sourceBranch << ")\n";
    out << "echo ------------------------------------------------------------------------------\n";
    out << "echo \n";

    // generate
    if (chosenOption != ChosenOption::onlyClone) {
        out << "get_deps\n";

        out << "echo ------------------------------------------------------------------------------\n";
        out << "echo Dependencies downloaded\n";
        out << "echo ------------------------------------------------------------------------------\n";
        out << "echo \n";

        out << "gen_project " << s.year << " " << s.architecture << "\n\n";

        out << "echo ------------------------------------------------------------------------------\n";
        out << "echo Project generated\n";
        out << "echo ------------------------------------------------------------------------------\n";

        QString genProjectPath = QString("build_%1_%2").arg(s.year).arg(s.architecture);
        QString vcxprojPath = QDir(issueFolderPath + "\\" + genProjectPath).filePath(projectName + ".vcxproj");

        out << "powershell -NoProfile -ExecutionPolicy Bypass -Command \""
            << "$path='" << vcxprojPath << ".user'; "
            << "$nsUri='http://schemas.microsoft.com/developer/msbuild/2003'; "
            << "if (Test-Path $path) { "
            << "[xml]$xml=Get-Content -Raw $path "
            << "} else { "
            << "$xml=New-Object System.Xml.XmlDocument; "
            << "$decl=$xml.CreateXmlDeclaration('1.0','utf-8',$null); "
            << "$xml.AppendChild($decl) | Out-Null; "
            << "$root=$xml.CreateElement('Project',$nsUri); "
            << "$root.SetAttribute('ToolsVersion','Current'); "
            << "$xml.AppendChild($root) | Out-Null "
            << "}; "
            << "$group=$xml.Project.PropertyGroup | Where-Object { "
            << "$_.Condition -like '*" << buildOption << "|x64*' "
            << "} | Select-Object -First 1; "
            << "if ($null -eq $group) { "
            << "$group=$xml.CreateElement('PropertyGroup',$nsUri); "
            << "$condition=([char]39).ToString() + '$' + '(Configuration)|$' + '(Platform)' + ([char]39).ToString() + '==' + ([char]39).ToString() + '"
            << buildOption << "|x" << s.architecture << "' + ([char]39).ToString(); "
            << "$group.SetAttribute('Condition',$condition); "
            << "$xml.Project.AppendChild($group) | Out-Null "
            << "}; "
            << "$setXmlValue = { param($parent, $name, $value) "
            << "$node=$parent.ChildNodes | Where-Object { $_.LocalName -eq $name } | Select-Object -First 1; "
            << "if ($null -eq $node) { "
            << "$node=$xml.CreateElement($name,$nsUri); "
            << "$parent.AppendChild($node) | Out-Null "
            << "}; "
            << "$node.InnerText=$value "
            << "}; "
            // << "& $setXmlValue $group 'LocalDebuggerCommandArguments' '-id:" << projectName << " -si:RS'; "
            << "& $setXmlValue $group 'LocalDebuggerCommandArguments' '" << CheckCommandArgs(s.commandArgs) << "'; "
            << "& $setXmlValue $group 'LocalDebuggerWorkingDirectory' '" << s.workingDir << "'; "
            << "& $setXmlValue $group 'DebuggerFlavor' 'WindowsLocalDebugger'; "
            // << "& $setXmlValue $group 'OutDir' 'd:\\ERA\\SDP_0.00_olapsAll\\'; " // Visual studio to ukládá do main vcxproj
            << "$xml.Save($path); "
            << "Write-Host 'vcxproj.user updated'; "
            << "\"\n";
        
        // Úprava hlavního vcxproj - kvůli Outdir v general settings
        out << "powershell -NoProfile -ExecutionPolicy Bypass -Command \""
            << "$path='" << vcxprojPath << "'; "
            // Načtení XML
            << "if (Test-Path $path) { "
            << "[xml]$xml=Get-Content -Raw $path; "
            // Původní a nová cesta
            << "$oldPath='" << issueFolderPath.replace("/", "\\") << "\\build\\x" << s.architecture << "\\'; "
            << "$newPath='" << s.workingDir.replace("/", "\\") << "'; "
            // Vyhledání OutDir elementů
            << "$nodes=$xml.SelectNodes('//*[local-name()=''OutDir'']'); "
            // Nahrazení prvních dvou nalezených cest (Debug a Release)
            << "$count=0; "
            << "foreach ($node in $nodes) { "
            << "if ($count -ge 2) { break }; "
            << "if ($node.InnerText.Contains($oldPath)) { "
            << "$node.InnerText=$node.InnerText.Replace($oldPath,$newPath); "
            << "$count++; "
            << "} "
            << "}; "
            // Uložení XML
            << "$xml.Save($path); "
            << "Write-Host \"Replaced $count OutDir paths\"; "
            << "} else { "
            << "Write-Host 'vcxproj not found'; "
            << "}; "
            << "\"\n";
        out << "echo \n";

        if (chosenOption != ChosenOption::cloneDepsGen) {
            // build
            out << "\"" << s.msbuildPath << "\" \"" << genProjectPath << "\\" << projectName << ".sln\" /p:Configuration=" << buildOption << "\n";
            out << "echo ------------------------------------------------------------------------------\n";
            out << "echo Build completed\n";
            out << "echo ------------------------------------------------------------------------------\n";
            out << "echo \n";
            if (chosenOption != ChosenOption::cloneDepsGenBuild) {
                // open
                out << "timeout /t 3 >nul\n";
                out << "start \"\" \"" << genProjectPath << "\\" << projectName << ".sln\"\n";
                out << "timeout /t 3 >nul\n";

                if (chosenOption != ChosenOption::cloneDepsGenBuildOpen) {
                    QString suffix = "_dbg";
                    if (buildOption == "Release"){
                        suffix = "";
                    }
                    // out << "start \"\" \"" << QString("build\\x%1\\").arg(s.architecture) << projectName << "_dbg.exe\"\n";
                    out << "start \"\" \"" << QString("build\\x%1\\").arg(s.architecture) << projectName << suffix << ".exe\"\n";

                    out << "echo ------------------------------------------------------------------------------\n";
                    out << "echo Project opened\n";
                    out << "echo ------------------------------------------------------------------------------\n";
                    out << "echo \n";
                }
            }
        }
    }
    if (!s.tcmdPath.isEmpty() && ui->CHB_openTcmd->isChecked()) {
        out << QString("\"%1\" /O /T /L=\"\%CD\%\"").arg(s.tcmdPath); // opens path created
    }
    // out << "pause";

    // musím zjistit jestli je daná cesta prázdná jinak se neprovede git clone
    QProcess::startDetached("cmd.exe", {"/c", "start", "", batPath}, issueFolderPath); // zustane otevrene
    
}

AppSettings CloneBotWindow::CollectSettingsFromUi() const
{
    AppSettings s;

    s.localRootFolder = ui->LE_rootFolder->text();
    s.gitRepository   = ui->LE_gitRepoPath->text();
    s.msbuildPath     = ui->LE_msbuildPath->text();
    s.year            = ui->LE_year->text();
    s.architecture    = ui->LE_architecture->text();
    s.tcmdPath        = ui->LE_tcmdPath->text();
    s.chosenOptionIndex = ui->CB_action->currentIndex();
    s.debugReleaseIndex = ui->CB_releaseDebug->currentIndex();
    s.chosenIssueTypeIndex = ui->CB_issueType->currentIndex();
    s.sourceBranchIndex = ui->CB_sourceBranch->currentIndex();
    s.lastTabIndex    = ui->tabWidget->currentIndex();
    s.openTcmd        = ui->CHB_openTcmd->isChecked();
    s.workingDir      = ui->LE_workDir->text();
    s.commandArgs     = ui->LE_commArgs->text();

    return s;
}

void CloneBotWindow::ApplySettingsToUi(/*const AppSettings& s*/)
{
    AppSettings s;
    _config->LoadSettings(s);
    ui->LE_rootFolder->setText(s.localRootFolder);
    ui->LE_gitRepoPath->setText(s.gitRepository);
    ui->LE_msbuildPath->setText(s.msbuildPath);
    ui->LE_year->setText(s.year);
    ui->LE_architecture->setText(s.architecture);
    ui->LE_tcmdPath->setText(s.tcmdPath);
    ui->CB_action->setCurrentIndex(s.chosenOptionIndex);
    ui->CB_releaseDebug->setCurrentIndex(s.debugReleaseIndex);
    ui->CB_issueType->setCurrentIndex(s.chosenIssueTypeIndex);
    ui->tabWidget->setCurrentIndex(s.lastTabIndex);
    ui->CB_sourceBranch->setCurrentIndex(s.sourceBranchIndex);
    ui->CHB_openTcmd->setChecked(s.openTcmd);
    ui->LE_workDir->setText(s.workingDir);
    ui->LE_commArgs->setText(s.commandArgs);

}

QStringList CloneBotWindow::GetBranchList()
{
    QStringList repoBranchList{};
    QString repoName = ui->LE_name->text();
    QString gitPath = ui->LE_gitRepoPath->text();
    QString appOrCom = ui->CB_issueType->currentText();
    if (!repoName.isEmpty() && !gitPath.isEmpty()) {

        QString gitRepo = gitPath + appOrCom + "/" + repoName + ".git";
        QProcess process;
        process.start("git", {"ls-remote", "--heads", gitRepo});
        process.waitForFinished();

        if (process.exitCode() != 0)
            return {};

        QString output = process.readAllStandardOutput();

        for (const QString& line : output.split('\n', QString::SkipEmptyParts))
        {
            QStringList parts = line.split('\t');
            if (parts.size() != 2)
                continue;

            QString ref = parts[1];
            if (ref.startsWith("refs/heads/"))
                repoBranchList.append(ref.mid(QString("refs/heads/").length()));
        }
    }

    return repoBranchList;
}

void CloneBotWindow::on_btnSave_clicked()
{
    AppSettings s = CollectSettingsFromUi();
    _config->SaveSettings(s);
}

void CloneBotWindow::CloseEvent()
{
    on_btnSave_clicked();
    close();
}

void CloneBotWindow::on_btnRefresh_clicked()
{
    QStringList repoBranchList = GetBranchList();
    if (!repoBranchList.isEmpty()){
        ui->CB_sourceBranch->clear();
        ui->CB_sourceBranch->addItems(repoBranchList);
    } else {
        ui->CB_sourceBranch->clear();
        ui->CB_sourceBranch->addItems({"vng1","master"});
    }
}

void CloneBotWindow::CleanupOutputDir(QDir* batDir)
{
    const QDateTime limit = QDateTime::currentDateTime().addDays(-2); // older then 2 days
    const QStringList batFiles = batDir->entryList({"*.bat"}, QDir::Files);
    for (const QString& file : batFiles)
    {
        QFileInfo fileInfo(batDir->filePath(file));

        if (fileInfo.lastModified() < limit)
        {
            QFile::remove(fileInfo.filePath());
        }
    }
}

QString CloneBotWindow::CheckCommandArgs(QString inputArgs)
{
    inputArgs.remove(" ");
    QStringList arguments = inputArgs.split('-', QString::SkipEmptyParts);
    QString newArgs = "";
    for (const QString &arg : arguments) {
        QStringList parts = arg.split(':');
        if (parts.size() != 2) {
            continue;
        }
        
        QString name = parts[0];
        QString value = parts[1];
        newArgs = newArgs + "-" + name + ":" + value + " ";
    }
    ui->LE_commArgs->setText(newArgs); // případnou opravu zapíšu

    return newArgs;
}

void CloneBotWindow::OnProjectNameChange(const QString &text)
{
    QString currentInput = ui->LE_commArgs->text();
    currentInput.remove(" ");
    QStringList arguments = currentInput.split('-', QString::SkipEmptyParts);
    QString newIdInput = "";
    for (const QString &arg : arguments) {
        QStringList parts = arg.split(':');
        if (parts.size() != 2) {
            continue;
        }
        
        QString name = parts[0];
        QString value = parts[1];
        if (name == "id"){
            value = ui->LE_name->text();
        }
        newIdInput = newIdInput + "-" + name + ":" + value + " ";
    }

    if (newIdInput.isEmpty()){
        newIdInput = newIdInput + "-id:" + ui->LE_name->text() + " -si:RS";
        ui->LE_commArgs->setText(newIdInput);
    } else {
        // ještě odstranit mezeru na konci
        ui->LE_commArgs->setText(newIdInput.remove(QRegularExpression("\\s+$")));
    }

}