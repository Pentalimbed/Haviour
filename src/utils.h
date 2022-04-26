#pragma once

#include <string>
#include <tuple>
#include <functional>

#include <robin_hood.h>
#include <pugixml.hpp>
#include <fmt/format.h>

// for editing flags and enums
struct EnumWrapper
{
    std::string_view name;
    std::string_view hint = {};
    uint32_t         val  = 0;
};

// get index of template argument
// source https://stackoverflow.com/questions/15014096/c-index-of-type-during-variadic-template-expansion
template <typename Target, typename ListHead, typename... ListTails>
constexpr size_t getTypeIndex()
{
    if constexpr (std::is_same<Target, ListHead>::value)
        return 0;
    else
        return 1 + getTypeIndex<Target, ListTails...>();
}

// using string map without allocation
// source: https://www.cppstories.com/2021/heterogeneous-access-cpp20/
struct StringHash
{
    using is_transparent = void;
    [[nodiscard]] size_t operator()(const char* txt) const
    {
        return robin_hood::hash_bytes(txt, std::strlen(txt));
    }
    [[nodiscard]] size_t operator()(std::string_view txt) const
    {
        return robin_hood::hash_bytes(txt.data(), txt.size());
    }
    [[nodiscard]] size_t operator()(const std::string& txt) const
    {
        return robin_hood::hash_bytes(txt.data(), txt.size());
    }
};
template <typename T>
using StringMap = robin_hood::unordered_map<std::string, T, StringHash, std::equal_to<>>;
using StringSet = robin_hood::unordered_set<std::string, StringHash, std::equal_to<>>;

//////////////////////////    XML HELPERS
inline pugi::xml_node appendXmlString(pugi::xml_node target, std::string_view srcString)
{
    // parse XML string as document
    pugi::xml_document doc;
    if (!doc.load_buffer(srcString.data(), srcString.length()))
        return {};

    if (target.attribute("numelements"))
        target.attribute("numelements") = target.attribute("numelements").as_uint() + 1;
    return target.append_copy(doc.first_child());
}

inline pugi::xml_node getNthChild(pugi::xml_node target, uint32_t n)
{
    auto node = target.first_child();
    while (node && (n > 1))
        node = node.next_sibling();
    return node;
}

inline bool isRefBy(std::string_view id, pugi::xml_node obj)
{
    return obj.find_node([=](pugi::xml_node node) { return std::string_view(node.text().as_string()).contains(id); });
}

inline pugi::xml_node getParentObj(pugi::xml_node node)
{
    for (auto iter = node.parent(); iter; iter = iter.parent())
        if (iter.attribute("class"))
            return iter;
    return {};
}

inline std::string hkobj2str(pugi::xml_node obj)
{
    return fmt::format("{} [{}] - {}", obj.attribute("name").as_string(), obj.find_child_by_attribute("hkparam", "name", "name").text().as_string(), obj.attribute("class").as_string());
}

inline std::string printIdVector(std::vector<std::string> ids)
{
    std::string retval = {};
    for (size_t i = 0; i < ids.size(); ++i)
        retval.append(fmt::format("{} {}", ids[i], ((i + 1) % 10) ? "" : "\n"));
    return retval;
}