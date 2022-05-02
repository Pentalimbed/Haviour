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
// source: https://stackoverflow.com/questions/15014096/c-index-of-type-during-variadic-template-expansion
template <typename Target, typename ListHead, typename... ListTails>
constexpr size_t getTypeIndex()
{
    if constexpr (std::is_same<Target, ListHead>::value)
        return 0;
    else
        return 1 + getTypeIndex<Target, ListTails...>();
}

// check template arguments contains stuff
// source: https://stackoverflow.com/questions/34122177/ensuring-that-a-variadic-template-contains-no-duplicates
template <typename T, typename... List>
struct IsContained;

template <typename T, typename Head, typename... Tail>
struct IsContained<T, Head, Tail...>
{
    enum
    {
        value = std::is_same<T, Head>::value || IsContained<T, Tail...>::value
    };
};

template <typename T>
struct IsContained<T>
{
    enum
    {
        value = false
    };
};

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

#define getByName(name) find_child_by_attribute("name", name)

inline bool isRefBy(std::string_view id, pugi::xml_node obj)
{
    return obj.find_node([=](pugi::xml_node node) { return std::string_view(node.text().as_string()).contains(id); });
}

inline pugi::xml_node getParentObj(pugi::xml_node node)
{
    for (auto iter = node; iter; iter = iter.parent())
        if (iter.attribute("class"))
            return iter;
    return {};
}

inline const char* getObjContextName(pugi::xml_node obj)
{
    auto hkclass = obj.attribute("class").as_string();
    if (!strcmp(hkclass, "hkbStringEventPayload"))
        return obj.getByName("data").text().as_string();
    else if (!strcmp(hkclass, "hkbExpressionCondition"))
        return obj.getByName("expression").text().as_string();
    else
        return obj.getByName("name").text().as_string();
}

inline std::string hkobj2str(pugi::xml_node obj)
{
    return fmt::format("{} [{}] - {}", obj.attribute("name").as_string(), getObjContextName(obj), obj.attribute("class").as_string());
}

inline std::string printIdVector(std::vector<std::string> ids)
{
    std::string retval = {};
    for (size_t i = 0; i < ids.size(); ++i)
        retval.append(fmt::format("{} {}", ids[i], ((i + 1) % 10) ? "" : "\n"));
    return retval;
}

inline uint32_t getChildIndex(pugi::xml_node hkobject)
{
    uint32_t result = 0;
    while (hkobject.previous_sibling())
    {
        hkobject = hkobject.previous_sibling();
        ++result;
    }
    return result;
}

inline std::string getParamPath(pugi::xml_node hkparam)
{
    std::string retval = "";
    while (hkparam && !hkparam.attribute("class"))
    {
        if (hkparam.attribute("name"))
            retval = fmt::format("/{}{}", hkparam.attribute("name").as_string(), retval);
        else
            retval = fmt::format(":{}{}", getChildIndex(hkparam), retval);
        hkparam = hkparam.parent();
    }
    return {retval.begin() + 1, retval.end()};
}