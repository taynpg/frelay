#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <Util.h>
#include <nlohmann/json.hpp>
#include <vector>
using json = nlohmann::json;

struct CgConnection {
    std::string ip;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CgConnection, ip);
};

using CgConVec = std::vector<CgConnection>;

class FrelayConfig
{
public:
    FrelayConfig();
    ~FrelayConfig();

public:
    bool SaveIpPort(const QString& ipPort);
    std::vector<QString> GetIpPort();

private:
    bool existFile(const std::string& path);
};

#endif   // CONFIG_H