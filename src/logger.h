#pragma once

#include <fmt/format.h>
#include <pugixml.hpp>

namespace Haviour
{
inline std::string hkobj2str(pugi::xml_node obj)
{
    return fmt::format("{} [{}] - {}", obj.attribute("name").as_string(), obj.find_child_by_attribute("hkparam", "name", "name").text().as_string(), obj.attribute("class").as_string());
}

void setupLogger();
} // namespace Haviour
