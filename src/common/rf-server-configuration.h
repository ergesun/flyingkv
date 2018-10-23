/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_RFCOMMON_SERVER_CONFIGURATION_H
#define MINIKV_RFCOMMON_SERVER_CONFIGURATION_H

#include <string>
#include <map>

#include "rf-server.h"

namespace minikv {
namespace common {
class RfServerConfiguration {
public:
    RfServerConfiguration() = default;
    ~RfServerConfiguration() = default;

    bool Initialize(uint32_t myId, std::string &path);
    inline const std::map<uint32_t, RfServer>& GetOtherServers() const {
        return m_mapOtherServers;
    }

    inline const RfServer& GetSelfServer() const {
        return m_selfServer;
    }

private:
    std::map<uint32_t, RfServer>      m_mapOtherServers;
    RfServer                          m_selfServer;
};
} // namespace rfcommon
} // namespace minikv

#endif //MINIKV_RFCOMMON_SERVER_CONFIGURATION_H
