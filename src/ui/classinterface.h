// Property edit ui for each of the class

#pragma once
#include "hkx/hkxfile.h"

#include <functional>

#include <pugixml.hpp>

namespace Haviour
{
namespace Ui
{
void showEditUi(pugi::xml_node hkobject, Hkx::BehaviourFile& file);
} // namespace Ui
} // namespace Haviour