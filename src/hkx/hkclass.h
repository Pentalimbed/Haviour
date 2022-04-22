#pragma once

#include <array>
#include <string_view>

#include <pugixml.hpp>

namespace Haviour
{
inline bool appendXmlString(pugi::xml_node target, std::string_view srcString)
{
    // parse XML string as document
    pugi::xml_document doc;
    if (!doc.load_buffer(srcString.data(), srcString.length()))
        return false;

    for (pugi::xml_node child = doc.first_child(); child; child = child.next_sibling())
        target.append_copy(child);
    return true;
}

namespace Hkx
{
using namespace std;

//////////////////////////    ENUMS
// hkbBehaviorGraph
constexpr auto e_variableMode = to_array(
    {"VARIABLE_MODE_DISCARD_WHEN_INACTIVE"sv,
     "VARIABLE_MODE_MAINTAIN_VALUES_WHEN_INACTIVE"sv});

constexpr auto e_bindingType = to_array(
    {"BINDING_TYPE_VARIABLE"sv,
     "BINDING_TYPE_CHARACTER_PROPERTY"sv});


//////////////////////////    DEFAULT VALUES
constexpr const char* g_def_binding = R"(<hkobject>
        <hkparam name="memberPath">variable_name</hkparam>
        <hkparam name="variableIndex">0</hkparam>
        <hkparam name="bitIndex">-1</hkparam>
        <hkparam name="bindingType">BINDING_TYPE_VARIABLE</hkparam>
        </hkobject>)";

} // namespace Hkx

} // namespace Haviour