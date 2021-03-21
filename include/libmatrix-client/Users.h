#ifndef __USERS_H__
#define __USERS_H__

#include <string>
#include <memory>

#include "Device.h"

namespace LibMatrix {
class User {
public:
	std::string id;
	std::string displayName;
	std::string avatarURL;
	std::unordered_map<std::string, std::shared_ptr<Device>> devices;
};
}; // namespace LibMatrix

#endif
