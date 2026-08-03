#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QDir>
#include <QJsonDocument>
#include <QFile>

struct AppSettings
{
    QString localRootFolder;
    QString gitRepository;
    QString msbuildPath;
    QString year;
    QString architecture;
    int debugReleaseIndex = 0;
    int chosenOptionIndex = 0;
};

class AppConfig {
    public:
        bool SaveSettings(const AppSettings& s);
        bool LoadSettings(AppSettings& s);
    private:
        QString GetConfigPath();

};