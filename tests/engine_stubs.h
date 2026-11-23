// Stand-ins for the engine services that the code under test calls.

#ifndef ENGINE_STUBS_H
#define ENGINE_STUBS_H

#include <map>
#include <string>
#include <vector>

namespace stubs {

// What FS_ReadFile finds and FS_WriteFile stores, by name.
std::map<std::string, std::vector<unsigned char>> &Files();

// Everything printed with Com_Printf since the last call of Reset.
std::string &Printed();

// Forgets files and printed text.
void Reset();

} // namespace stubs

#endif // ENGINE_STUBS_H
