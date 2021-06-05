#ifndef __TYPE_DEFS_H__
#define __TYPE_DEFS_H__

#include <unordered_map>
#include <memory>

namespace LibMatrix {
class Room;

using RoomMap = std::unordered_map<std::string, std::shared_ptr<LibMatrix::Room>>;

}

#endif