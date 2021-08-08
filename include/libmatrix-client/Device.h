#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <olm/olm.h>

namespace LibMatrix {
class Device {
public:
    std::string id;
    std::string userId;
    std::vector<std::string> encryptAlgos;
    std::unordered_map<std::string, std::string> idKeys;
    std::unordered_map<std::string, std::string> signatures; //Done under assumption that only the device's user will be used
    std::shared_ptr<OlmSession> currentSession;
};
}

#endif