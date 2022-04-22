// Property edit ui for each of the class

#pragma once

#include <unordered_map>
#include <functional>

#include <pugixml.hpp>

namespace Haviour
{
namespace Ui
{
extern const std::unordered_map<std::string_view, std::function<void(pugi::xml_node)>> g_class_ui_map;

void showEditUi(pugi::xml_node hkobeject);
} // namespace Ui
} // namespace Haviour