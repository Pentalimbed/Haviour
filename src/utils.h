#pragma once

#include <string>
#include <tuple>
#include <functional>
#include <mutex>

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

template <size_t N>
uint32_t str2FlagVal(std::string_view str, const std::array<EnumWrapper, N>& flags)
{
    uint32_t retval = 0;
    for (auto flag : flags)
        if (flag.val && str.contains(flag.name))
            retval |= flag.val;
    return retval;
}

template <size_t N>
std::string flagVal2Str(uint32_t value, const std::array<EnumWrapper, N>& flags)
{
    std::string value_text = {};
    for (auto flag : flags)
        if (value & flag.val)
            value_text = fmt::format("{}{}|", value_text, flag.name);
    if (value_text.ends_with('|'))
        value_text.pop_back();
    return value_text;
}

// filtering
inline bool strCaseContains(std::string_view str, std::string_view target)
{
    return !std::ranges::search(str, target, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty();
}

inline bool hasText(std::string_view str, std::string_view filter_str)
{
    return filter_str.empty() || strCaseContains(str, filter_str);
}

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

// constexpr array join
// source: https://gist.github.com/klemens-morgenstern/b75599292667a4f53007
template <typename T, std::size_t LL, std::size_t RL>
constexpr std::array<T, LL + RL> joinArray(std::array<T, LL> rhs, std::array<T, RL> lhs)
{
    std::array<T, LL + RL> ar;

    auto current = std::copy(rhs.begin(), rhs.end(), ar.begin());
    std::copy(lhs.begin(), lhs.end(), current);

    return ar;
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

// crc32
// source: https://gist.github.com/timepp/1f678e200d9e0f2a043a9ec6b3690635
// actually it's nemesis
inline uint32_t mirror_bit(uint32_t val, char num)
{
    uint32_t retval = 0;
    for (int i = 1; i <= num; i++)
    {
        if (val & 1)
            retval |= (1 << (num - i));
        val >>= 1;
    }
    return retval;
}

struct Crc32
{
    static inline uint32_t table[256];

    static inline void generate_table(uint32_t (&table)[256])
    {
        constexpr uint32_t polynomial = 0x04C11DB7; // pkzip
        for (uint32_t i = 0; i < 256; i++)
        {
            // uint32_t c = i;
            // for (size_t j = 0; j < 8; j++)
            // {
            //     if (c & 1)
            //         c = polynomial ^ (c >> 1);
            //     else
            //         c >>= 1;
            // }
            // table[i] = c;
            uint32_t c = mirror_bit(i, 8) << 24;
            for (size_t j = 0; j < 8; j++)
                c = (c << 1) ^ ((c & (1 << 31)) ? polynomial : 0);
            c = mirror_bit(c, 32);

            table[i] = c;
        }
    }

    static inline uint32_t update(const char* buf, size_t len, uint32_t initial = 0, uint32_t finalxor = 0)
    {
        static std::once_flag flag;
        std::call_once(flag, []() {
            Crc32::generate_table(Crc32::table);
        });

        // uint32_t c = initial ^ 0xFFFFFFFF;
        // const uint8_t* u = static_cast<const uint8_t*>(buf);
        // for (size_t i = 0; i < len; ++i)
        // {
        // 	c = table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
        // }
        // return c ^ 0xFFFFFFFF;

        uint32_t c = initial;
        while (len--)
            c = (c >> 8) ^ table[(c & 0xFF) ^ *(buf++)];
        return c ^ finalxor;
    }
};

// quickly define factory member & method
#define DEF_FACT_MEM(type, name, def_val) \
    type  m_##name = def_val;             \
    auto& name(type name)                 \
    {                                     \
        m_##name = name;                  \
        return *this;                     \
    }

//////////////////////////    XML HELPERS
inline pugi::xml_node appendXmlString(pugi::xml_node target, std::string_view srcString)
{
    // parse XML string as document
    pugi::xml_document doc;
    if (!doc.load_buffer(srcString.data(), srcString.length(), pugi::parse_default & (~pugi::parse_escapes)))
        return {};

    if (target.attribute("numelements"))
        target.attribute("numelements") = target.attribute("numelements").as_uint() + 1;
    return target.append_copy(doc.first_child());
}

#define getByName(name) find_child_by_attribute("name", name)

inline bool isRefBy(std::string_view id, pugi::xml_node obj)
{
    return obj.find_node([=](pugi::xml_node node) { return (node.type() == pugi::node_pcdata) && std::string_view(node.value()).contains(id); });
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

template <typename T>
inline std::string printVector(const std::vector<T>& vec)
{
    std::string retval = {};
    for (size_t i = 0; i < vec.size(); ++i)
    {
        if constexpr (std::is_same_v<T, float>)
            retval.append(fmt::format("{:.6f} {}", vec[i], ((i + 1) % 10) ? "" : "\n"));
        else
            retval.append(fmt::format("{} {}", vec[i], ((i + 1) % 10) ? "" : "\n"));
    }
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

inline pugi::xml_node getNthChild(pugi::xml_node node, size_t n)
{
    auto retval = node.first_child();
    while (n)
    {
        retval = retval.next_sibling();
        --n;
    }
    return retval;
}

inline bool isVarNode(pugi::xml_node node)
{
    auto var_name = node.attribute("name").as_string();
    return (!strcmp(var_name, "variableIndex") && !strcmp(node.parent().getByName("bindingType").text().as_string(), "BINDING_TYPE_VARIABLE")) ||
        !strcmp(var_name, "syncVariableIndex") ||     // hkbStateMachine
        !strcmp(var_name, "assignmentVariableIndex"); // hkbExpressionData
}

inline bool isEvtNode(pugi::xml_node node)
{
    auto node_name = node.attribute("name").as_string();

    auto parentparent = node.parent().parent();
    auto pp_name      = parentparent.attribute("name").as_string();
    bool is_hkbEvent =
        !strcmp(pp_name, "event") ||
        !strcmp(pp_name, "events") ||
        !strcmp(pp_name, "triggerEvent") ||                            // BSDistTriggerModifier & BSPassByTargetTriggerModifier
        !strcmp(pp_name, "contactEvent") ||                            // BSRagdollContactListenerModifier
        !strcmp(pp_name, "eventToSendWhenStateOrTransitionChanges") || // hkbStateMachine
        !strcmp(pp_name, "EventToFreezeBlendValue") ||                 // BSCyclicBlendTransitionGenerator
        !strcmp(pp_name, "EventToCrossBlend") ||
        !strcmp(pp_name, "alarmEvent") ||    // hkbTimerModifier & BSTimerModifier
        !strcmp(pp_name, "ungroundedEvent"); // hkbFootIkControlsModifier

    return !strcmp(node_name, "eventId") ||   // hkbStateMachineTransitionInfo
        !strcmp(node_name, "enterEventId") || // hkbStateMachineStateInfo/TimeInterval
        !strcmp(node_name, "exitEventId") ||
        !strcmp(node_name, "assignmentEventIndex") ||         // hkbExpressionData
        !strcmp(node_name, "returnToPreviousStateEventId") || // hkbStateMachine
        !strcmp(node_name, "randomTransitionEventId") ||
        !strcmp(node_name, "transitionToNextHigherStateEventId") ||
        !strcmp(node_name, "transitionToNextLowerStateEventId") ||
        (!strcmp(node_name, "id") && is_hkbEvent);
}

inline bool isPropNode(pugi::xml_node node)
{
    return (!strcmp(node.attribute("name").as_string(), "variableIndex") && !strcmp(node.parent().getByName("bindingType").text().as_string(), "BINDING_TYPE_CHARACTER_PROPERTY"));
}