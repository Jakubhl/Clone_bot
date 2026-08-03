#include "JsonHandler.h"
#include <QStandardPaths>

bool AppConfig::SaveSettings(const AppSettings& s)
{
    QJsonObject obj;

    obj["localRootFolder"] = s.localRootFolder;
    obj["gitRepository"]   = s.gitRepository;
    obj["msbuildPath"]     = s.msbuildPath;
    obj["year"]            = s.year;
    obj["architecture"]    = s.architecture;

    QJsonDocument doc(obj);

    QFile file(GetConfigPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool AppConfig::LoadSettings(AppSettings& s)
{
    QFile file(GetConfigPath());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;

    QJsonObject obj = doc.object();

    s.localRootFolder = obj["localRootFolder"].toString();
    s.gitRepository   = obj["gitRepository"].toString();
    s.msbuildPath     = obj["msbuildPath"].toString();
    s.year            = obj["year"].toString();
    s.architecture    = obj["architecture"].toString();

    return true;
}

QString AppConfig::GetConfigPath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir); // vytvoří složku pokud neexistuje

    return dir + "/config.json";
}