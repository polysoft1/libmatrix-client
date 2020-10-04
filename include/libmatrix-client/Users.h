#ifndef __USERS_H__
#define __USERS_H__

#include <string>

namespace LibMatrix {
class User {
public:
	std::string id;
	std::string displayName;
	std::string avatarURL;
};
}; // namespace LibMatrix

#endif
