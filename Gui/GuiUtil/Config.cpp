#include "Config.h"

#include <fstream>

FrelayConfig::FrelayConfig()
{
}

FrelayConfig::~FrelayConfig()
{
}

bool FrelayConfig::SaveIpPort(const QString& ipPort)
{
    auto p = GlobalData::Ins()->GetConfigPath();

    json j;

    if (existFile(p)) {
        std::ifstream ifs(p);
        ifs >> j;
        ifs.close();
    }

    CgConVec vec;
    if (j.contains("connections")) {
        vec = j["connections"].get<CgConVec>();
    }

    bool exist = false;
    for (const auto& v : vec) {
        if (v.ip == ipPort.toStdString()) {
            exist = true;
            break;
        }
    }

    if (exist) {
        return true;
    }

    vec.emplace(vec.begin(), CgConnection{ipPort.toStdString()});
    if (vec.size() >= 10) {
        vec.pop_back();
    }

    j["connections"] = vec;
    std::ofstream ofs(p);
    ofs << std::setw(4) << j << std::endl;
    return true;
}

std::vector<QString> FrelayConfig::GetIpPort()
{
    std::vector<QString> result;
    auto p = GlobalData::Ins()->GetConfigPath();
    json j;
    if (existFile(p)) {
        std::ifstream ifs(p);
        ifs >> j;
        ifs.close();
        if (j.contains("connections")) {
            auto vec = j["connections"].get<CgConVec>();
            for (const auto& v : vec) {
                result.push_back(QString::fromStdString(v.ip));
            }
        } else {
            result.push_back("127.0.0.1:9009");
        }
    } else {
        result.push_back("127.0.0.1:9009");
    }
    return result;
}

bool FrelayConfig::existFile(const std::string& path)
{
    std::ifstream ifs(path);
    return ifs.good();
}
