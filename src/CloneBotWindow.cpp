#include "CloneBotWindow.h"
// #include "ui_CloneBotWindow.h"
#include <QColorDialog>
#include <QTextStream>
#include <QProcess>
#include <QThread>
// #include <QSizeF>

CloneBotWindow::CloneBotWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    // _config.Load();
    setWindowTitle("Clonebot");
    setWindowIcon(QIcon("../../pic/robot_ico.png"));

    ui->CB_sourceBranch->addItems({"vng1","master"}); // dodelat aut. plnění + uložení do configu
    ui->CB_issueType->addItems({"app", "com","3p","tools","vat"}); // dodelat aut. plnění + uložení do configu
    ui->CB_releaseDebug->addItems({"Release", "Debug"}); // dodelat uložení do configu
    ui->CB_action->addItems({"Only clone", "Clone + (get deps + gen project)", "Clone + (get deps + gen project + build) + run"}); // dodelat uložení do configu

    ApplySettingsToUi();

    connect(ui->btnSave, &QPushButton::clicked, this, &CloneBotWindow::on_btnSave_clicked);
    connect(ui->btnRefreshBranch, &QPushButton::clicked, this, &CloneBotWindow::on_btnRefresh_clicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QWidget::close);
    connect(ui->btnCancelRun, &QPushButton::clicked, this, &QWidget::close);
    connect(ui->btnRun, &QPushButton::clicked, this, &CloneBotWindow::RunClone);

}

CloneBotWindow::~CloneBotWindow()
{   
    // clear temp batfiles
    delete ui;
}


void CloneBotWindow::RunClone()
{
    int issueNumber = ui->LE_issueNumber->text().toInt();
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
    QString batPath = QCoreApplication::applicationDirPath() + QString("/Run_%1_clone_issue%2.bat").arg(projectName).arg(issueNumber);
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

    out << "echo ------------------------------------------------------------------------------\n";
    out << "echo Project: " << projectName << " cloned from repo: " << s.gitRepository << "\n";
    out << "echo ------------------------------------------------------------------------------\n";
    out << "echo \n";

    out << "git checkout " << sourceBranch << "\n";
    out << "git fetch --all\n";
    out << "git checkout " << issueFolderName << " || git checkout -b " << issueFolderName << "\n\n";

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
        out << "echo \n";

        if (chosenOption != ChosenOption::cloneDepsGen) {
            // build
            // QString genProjectPath = "build_2019_64";
            QString genProjectPath = QString("build_%1_%2").arg(s.year).arg(s.architecture);
            // out << "msbuild " << genProjectPath << "\\" << projectName << ".sln /p:Configuration=Debug\n\n";

            // QString genProjectFullPath = QDir(issueFolderPath).filePath(genProjectPath);
            // out << "cd /d \"" << genProjectFullPath << "\"\n";
            out << "\"" << s.msbuildPath << "\" \"" << genProjectPath << "\\" << projectName << ".sln\" /p:Configuration=" << buildOption << "\n";

            out << "echo ------------------------------------------------------------------------------\n";
            out << "echo Build completed\n";
            out << "echo ------------------------------------------------------------------------------\n";
            out << "echo \n";

            // open
            out << "timeout /t 3 >nul\n";
            out << "start \"\" \"" << genProjectPath << "\\" << projectName << ".sln\"\n";
            out << "timeout /t 3 >nul\n";
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

    // out << "pause";

    // musím zjistit jestli je daná cesta prázdná jinak se neprovede git clone
    QProcess::startDetached("cmd.exe", {"/c", "start", "", batPath}, issueFolderPath); // zustane otevrene
    
    
    
    
    return;




    // git management
    // QString gitFullPath = s.gitRepository + issueType + "/" + projectName + ".git";

    QProcess gitClone;
    gitClone.start("git", {"clone", gitFullPath, issueFolderPath});
    gitClone.waitForFinished();

    if (gitClone.exitCode() != 0) {
        // qDebug() << "Chyba při klonování:" << gitClone.readAllStandardError();
    }
    else {
        // qDebug() << "Repozitář naklonován";
    }



    //checkout branch
    if (sourceBranch != "master") {
    QProcess gitCheckout;
    gitCheckout.setWorkingDirectory(issueFolderPath);

    gitCheckout.start("git", {"checkout", sourceBranch});
    gitCheckout.waitForFinished();
    }

    QProcess gitBranches;
    gitBranches.setWorkingDirectory(issueFolderPath);

    gitBranches.start("git", {"branch", "-a"});
    gitBranches.waitForFinished();

    QString output = gitBranches.readAllStandardOutput();

    QStringList branches;
    for (const QString &line : output.split('\n')) {
        QString b = line.trimmed();
        if (b.isEmpty()) continue;

        b.replace("remotes/origin/", "");
        branches << b;
    }

    // qDebug() << "Všechny větve:" << branches;





    auto switchOrCreateBranch = [&](const QString& gitPath, const QString& branchName)
    {
        QProcess checkout;
        checkout.setWorkingDirectory(gitPath);

        checkout.start("git", {"checkout", branchName});
        checkout.waitForFinished();

        if (checkout.exitCode() == 0) {
            // qDebug() << "✓ Přepnuto na" << branchName;
        }
        else {
            // qDebug() << "Větev neexistuje, vytvářím:" << branchName;

            QProcess create;
            create.setWorkingDirectory(gitPath);
            create.start("git", {"checkout", "-b", branchName});
            create.waitForFinished();

            // qDebug() << "✓ Větev vytvořena";
        }
    };

    switchOrCreateBranch(issueFolderPath, issueFolderName);





    // generate
    QProcess qProc;
    qProc.setWorkingDirectory(issueFolderPath);
    qProc.start("get_deps");
    qProc.waitForFinished();
    qProc.start("gen_project", {s.year, s.architecture});
    qProc.waitForFinished();
    // QString GEN_PROJECT_PATH = "build_" + s.year + s.architecture;
    QString GEN_PROJECT_PATH = "build_2019_64";
    QString BUILD_PROJECT_PATH = "build\x64";

    // build
    QString genProjectFullPath = QDir(issueFolderPath).filePath(GEN_PROJECT_PATH);

    QProcess msbuild;
    msbuild.setWorkingDirectory(genProjectFullPath);

    msbuild.start(s.msbuildPath, {projectName + ".sln", "/p:Configuration=Debug"});
    msbuild.waitForFinished();
    QThread::sleep(3);



    // open studio and app
    QString slnPath = QDir(issueFolderPath)
    .filePath(GEN_PROJECT_PATH + "/" + projectName + ".sln");

    QString exePath = QDir(issueFolderPath)
        .filePath(BUILD_PROJECT_PATH + "/" + projectName + "_dbg.exe");

    QProcess::startDetached("cmd.exe", {"/c", "start", "", slnPath});
    QThread::sleep(3);
    QProcess::startDetached("cmd.exe", {"/c", "start", "", exePath});

}

AppSettings CloneBotWindow::CollectSettingsFromUi() const
{
    AppSettings s;

    s.localRootFolder = ui->LE_rootFolder->text();
    s.gitRepository   = ui->LE_gitRepoPath->text();
    s.msbuildPath     = ui->LE_msbuildPath->text();
    s.year            = ui->LE_year->text();
    s.architecture    = ui->LE_architecture->text();
    s.chosenOptionIndex = ui->CB_action->currentIndex();
    s.debugReleaseIndex = ui->CB_releaseDebug->currentIndex();

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
    ui->CB_action->setCurrentIndex(s.chosenOptionIndex);
    ui->CB_releaseDebug->setCurrentIndex(s.debugReleaseIndex);

}

QStringList CloneBotWindow::GetBranchList()
{
    QStringList repoBranchList{};
    QString repoName = ui->LE_name->text();
    QString gitPath = ui->LE_gitRepoPath->text();
    QString appOrCom = ui->CB_issueType->currentText();
    if (!repoName.isEmpty() && !gitPath.isEmpty()) {

        QString gitRepo = gitPath + appOrCom + "/" + repoName + ".git";
        // std::string ahoj = gitRepo.toStdString();

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