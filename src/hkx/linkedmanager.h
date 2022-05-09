// For handling linked properties
// i.e. whose values are stored separately in different places and referenced by indexes
// e.g. Events, Variables, Character Property
// It's all mumbo jumbo template bullshite
// Am I overengineering?
#pragma once
#include "utils.h"

#include <array>
#include <vector>
#include <tuple>

#include <pugixml.hpp>

namespace Haviour
{
namespace Hkx
{

struct LinkedProperty
{
    virtual const char* getDefaultValue() = 0;
};

#define LINKEDPROP(class_name)                          \
    struct class_name : public LinkedProperty           \
    {                                                   \
        virtual const char* getDefaultValue() override; \
    }

LINKEDPROP(PropName);
LINKEDPROP(PropWordValue);
LINKEDPROP(PropVarInfo);
LINKEDPROP(PropEventInfo);

// this is bascially struct but worse w/ different constructor for each elements
// I don't know why I bothered to write this when it's just like 3 kinds of structs
template <class... Props>
class LinkedPropertyEntry
{
public:
    using NodeArray = typename std::array<pugi::xml_node, sizeof...(Props)>;

    bool   m_valid = false;
    size_t m_index;

private:
    template <size_t idx = 0>
    constexpr void addProp(NodeArray& containers)
    {
        if constexpr (idx < sizeof...(Props))
        {
            m_props[idx] = appendXmlString(containers[idx], std::tuple_element_t<idx, std::tuple<Props...>>().getDefaultValue());
            addProp<idx + 1>(containers);
        }
    }

    NodeArray m_props;

public:
    template <class T>
    constexpr pugi::xml_node get() { return m_props[getTypeIndex<T, Props...>()]; }

    std::enable_if_t<IsContained<PropName, Props...>::value, const char*> getName() { return get<PropName>().text().as_string(); }

    static LinkedPropertyEntry<Props...> create(NodeArray& containers)
    {
        LinkedPropertyEntry<Props...> retval;
        retval.m_valid = true;
        retval.addProp(containers);
        return retval;
    }
    static LinkedPropertyEntry<Props...> assemble(const NodeArray& nodes)
    {
        LinkedPropertyEntry<Props...> retval;
        retval.m_valid = true;
        retval.m_props = nodes;
        return retval;
    }
    void remove(NodeArray& containers)
    {
        for (size_t i = 0; i < containers.size(); ++i)
            containers[i].remove_child(m_props[i]);
    }
};

using AnimationEvent    = LinkedPropertyEntry<PropName, PropEventInfo>;
using Variable          = LinkedPropertyEntry<PropName, PropVarInfo, PropWordValue>;
using CharacterProperty = LinkedPropertyEntry<PropName, PropVarInfo>;

template <class T>
class LinkedPropertyManager
{
public:
    using Entry = typename T;

    inline size_t size() { return m_entries.size(); }
    inline T      getEntry(size_t idx)
    {
        if (idx < m_entries.size())
            return m_entries[idx];
        else
            return {};
    }
    inline std::vector<T> getEntryList() { return m_entries; }
    inline T              getEntryByName(std::string_view name)
    {
        auto res = std::ranges::find_if(m_entries, [=](auto& entry) { return !_stricmp(name.data(), entry.getName()); });
        if (res == m_entries.end())
            return {};
        else
            return *res;
    }
    inline T addEntry()
    {
        auto retval    = T::create(m_container_nodes);
        retval.m_index = m_entries.size();
        m_entries.push_back(retval);
        return retval;
    }
    inline void delEntry(size_t idx) { m_entries[idx].m_valid = false; }

    // clean up deleted variable
    // returns a idx remap map
    robin_hood::unordered_map<size_t, size_t> reindex()
    {
        size_t                                    new_idx = 0;
        robin_hood::unordered_map<size_t, size_t> remap   = {};
        for (size_t i = 0; i < m_entries.size(); i++)
        {
            if (m_entries[i].m_valid)
            {
                remap[m_entries[i].m_index] = new_idx;
                m_entries[i].m_index        = new_idx;
                ++new_idx;
            }
            else
            {
                m_entries[i].remove(m_container_nodes);
                remap.erase(m_entries[i].m_index);
            }
        }
        std::erase_if(m_entries, [](T& entry) { return !entry.m_valid; });

        for (size_t i = 0; i < m_container_nodes.size(); ++i)
            m_container_nodes[i].attribute("numelements") = size();

        return remap;
    }

protected:
    T::NodeArray   m_container_nodes;
    std::vector<T> m_entries = {};

    void buildEntryList(const typename T::NodeArray& container_nodes)
    {
        m_entries.clear();
        m_container_nodes = container_nodes;
        auto num_entries  = container_nodes[0].attribute("numelements").as_uint();

        typename T::NodeArray nodes;
        for (size_t i = 0; i < container_nodes.size(); ++i)
            nodes[i] = container_nodes[i].first_child();

        for (size_t i = 0; i < num_entries; ++i)
        {
            auto entry    = T::assemble(nodes);
            entry.m_index = m_entries.size();
            m_entries.push_back(entry);
            for (size_t j = 0; j < container_nodes.size(); ++j)
                nodes[j] = nodes[j].next_sibling();
        }
    }
};

class AnimationEventManager : public LinkedPropertyManager<AnimationEvent>
{
public:
    inline void buildEntryList(pugi::xml_node name_node, pugi::xml_node info_node) { LinkedPropertyManager<AnimationEvent>::buildEntryList({name_node, info_node}); }

private:
    using LinkedPropertyManager<AnimationEvent>::buildEntryList;
};

class CharacterPropertyManager : public LinkedPropertyManager<CharacterProperty>
{
public:
    inline void              buildEntryList(pugi::xml_node name_node, pugi::xml_node info_node) { LinkedPropertyManager<CharacterProperty>::buildEntryList({name_node, info_node}); }
    inline CharacterProperty addEntry()
    {
        auto retval = LinkedPropertyManager<CharacterProperty>::addEntry();

        retval.get<PropVarInfo>().getByName("type").text()                                  = "VARIABLE_TYPE_POINTER";
        retval.get<PropVarInfo>().getByName("role").first_child().getByName("flags").text() = "FLAG_OUTPUT|FLAG_HIDDEN|FLAG_NOT_VARIABLE|FLAG_NONE";

        return retval;
    }

private:
    using LinkedPropertyManager<CharacterProperty>::buildEntryList;
    using LinkedPropertyManager<CharacterProperty>::addEntry;
};

enum VariableTypeEnum : int8_t;

class VariableManager : public LinkedPropertyManager<Variable>
{
public:
    void                         buildEntryList(pugi::xml_node name_node, pugi::xml_node info_node, pugi::xml_node value_node, pugi::xml_node quad_node, pugi::xml_node ptr_node);
    Variable                     addEntry(VariableTypeEnum data_type);
    inline std::array<float, 4>& getQuadValue(size_t idx) { return m_quads[idx]; }
    inline std::string&          getPointerValue(size_t idx) { return m_ptrs[idx]; }

    robin_hood::unordered_map<size_t, size_t> reindex();

private:
    std::vector<std::array<float, 4>> m_quads;
    std::vector<std::string>          m_ptrs;
    pugi::xml_node                    m_quad_node;
    pugi::xml_node                    m_ptr_node;

    using LinkedPropertyManager<Variable>::buildEntryList;
    using LinkedPropertyManager<Variable>::addEntry;
    using LinkedPropertyManager<Variable>::reindex;
};

} // namespace Hkx

} // namespace Haviour