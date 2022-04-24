#pragma once

#include <memory>
#include <unordered_map>
#include <set>
#include <array>

#include <spdlog/spdlog.h>
#include <pugixml.hpp>
#include <eventpp/eventdispatcher.h>

#include "hkclass.h"

namespace Haviour
{
namespace Hkx
{
using NodeList = std::vector<pugi::xml_node>;

enum HkxFileEventEnum
{
    kEventLoadFile,
    kEventAddObj,
    kEventDelObj,
    kEventSaveFile
};

struct Variable
{
    pugi::xml_node name;
    pugi::xml_node value; // If not word, then this stores the index to corresponding value array.
    pugi::xml_node info;

    int  idx;
    bool invalid = true;

    inline std::string_view getType() { return info.find_child_by_attribute("name", "type").text().as_string(); }
};

struct NameInfo
{
    pugi::xml_node name;
    pugi::xml_node info;

    int  idx;
    bool invalid = true;
};

// For items that has name/info stored separately and referenced by index (event/char prop etc.)
class NameInfoManger
{
public:
    void         buildList(pugi::xml_node info_node, pugi::xml_node name_node);
    virtual void clear();

    inline std::vector<NameInfo> getEntryList() { return m_entry_list; }
    virtual NameInfo             addEntry();
    inline void                  delEntry(size_t idx) { m_entry_list[idx].invalid = true; }

    inline NameInfoManger(const char* info_def, const char* name_def) :
        m_info_def(info_def), m_name_def(name_def) {}

private:
    pugi::xml_node   m_info_node, m_name_node;
    std::string_view m_info_def, m_name_def;

    std::vector<NameInfo> m_entry_list;
};

// f*ck polymorphism idc
class VariableManager
{
public:
    // info_node/name_node should directly contain info/name objects.
    // value_node should be hkbVariableSet
    void        buildVarList(pugi::xml_node info_node, pugi::xml_node name_node, pugi::xml_node value_node);
    inline void clear()
    {
        m_info_node = {}, m_name_node = {}, m_value_node = {};
        m_variable_list.clear(), m_quads_value.clear();
    }

    inline std::vector<Variable> getVariableList() { return m_variable_list; }
    Variable                     addVariable(VariableTypeEnum data_type);
    inline void                  delVariable(size_t idx) { m_variable_list[idx].invalid = true; }
    inline std::array<float, 4>& getQuadValue(size_t idx) { return m_quads_value[idx]; }

private:
    pugi::xml_node m_info_node, m_name_node, m_value_node;

    std::vector<Variable>             m_variable_list;
    std::vector<std::array<float, 4>> m_quads_value;
};


class HkxFile : public eventpp::EventDispatcher<HkxFileEventEnum, void()>
{
public:
    friend struct Variable;

    static HkxFile* getSingleton()
    {
        static HkxFile file;
        return std::addressof(file);
    }

    bool loadFile(std::string_view path);
    void rebuildRefList();
    void buildVariableList();

    inline bool isLoaded() { return m_loaded; }

    inline NodeList       getFullNodeList() { return m_node_list; }
    inline pugi::xml_node getNode(std::string_view id)
    {
        if (m_node_name_map.contains(id))
            return m_node_name_map[id];
        else
            return pugi::xml_node();
    }
    inline NodeList                      getNodeClassList(std::string_view hkclass) { return m_node_class_map[hkclass]; }
    inline std::vector<std::string_view> getClasses()
    {
        std::vector<std::string_view> retval;
        retval.reserve(m_node_class_map.size() + 1);
        retval.push_back("All");
        for (auto const& element : m_node_class_map)
            retval.push_back(element.first);
        std::ranges::sort(retval);
        return retval;
    }
    inline NodeList getNodeRefs(std::string_view id)
    {
        auto retval = std::vector(m_node_ref_list_map[id]);
        std::ranges::sort(retval, [](auto& a, auto& b) { return strcmp(a.attribute("name").as_string(), b.attribute("name").as_string()) > 0; });
        return retval;
    }

    inline void addRef(std::string_view id, pugi::xml_node obj)
    {
        if (getNode(id) && obj)
        {
            if (!m_node_ref_list_map.contains(id))
                m_node_ref_list_map[id] = {};
            if (std::ranges::find(m_node_ref_list_map[id], obj) == m_node_ref_list_map[id].end())
                m_node_ref_list_map[id].push_back(obj);
        }
    }
    inline void deRef(std::string_view id, pugi::xml_node obj)
    {
        if (getNode(id) && obj)
            if (m_node_ref_list_map.contains(id))
                if (auto pos = std::ranges::find(m_node_ref_list_map[id], obj); pos != m_node_ref_list_map[id].end())
                    m_node_ref_list_map[id].erase(pos);
                else
                    spdlog::warn("Attempting to dereference {0} from {1} but {0} is not referenced by {1}!", id, obj.attribute("name").as_string());
            else
                spdlog::warn("Attempting to dereference {0} from {1} but {0} has no reference list!", id, obj.attribute("name").as_string());
    }

    VariableManager m_var_manager;
    NameInfoManger  m_evt_manager      = {g_def_hkbEventInfo, g_def_hkStringPtr};
    NameInfoManger  m_charprop_manager = {g_def_hkbVariableInfo_characterPropertyInfo, g_def_hkStringPtr};

    pugi::xml_node getFirstVariableRef(uint32_t idx);
    pugi::xml_node getFirstEventRef(uint32_t idx);
    pugi::xml_node getFirstCharPropRef(uint32_t idx);

private:
    std::shared_ptr<spdlog::logger> m_file_logger;

    pugi::xml_document m_doc;
    pugi::xml_node     m_data_node, m_graph_data_node, m_graph_str_data_node, m_var_value_node;
    bool               m_loaded = false;

    NodeList                                             m_node_list;
    std::unordered_map<std::string_view, pugi::xml_node> m_node_name_map;
    std::unordered_map<std::string_view, NodeList>       m_node_ref_list_map;
    std::unordered_map<std::string_view, NodeList>       m_node_class_map;
};

} // namespace Hkx

} // namespace Haviour
