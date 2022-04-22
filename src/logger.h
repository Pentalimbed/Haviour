#pragma once

#include <format>
#include <pugixml.hpp>

namespace Haviour
{
inline std::string hkobj2str(pugi::xml_node obj)
{
    return std::format("{} [{}] - {}", obj.attribute("name").as_string(), obj.find_child_by_attribute("hkparam", "name", "name").text().as_string(), obj.attribute("class").as_string());
}

void setupLogger();
} // namespace Haviour
