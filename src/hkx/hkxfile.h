#pragma once

#include <memory>
#include <unordered_map>
#include <set>

#include <spdlog/spdlog.h>
#include <pugixml.hpp>
#include <eventpp/eventdispatcher.h>

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
    std::string_view name;
    pugi::xml_node   value;
    pugi::xml_node   info;

    int  idx;
    bool deleted = false;
};

class HkxFile : public eventpp::EventDispatcher<HkxFileEventEnum, void()>
{
public:
    static HkxFile* getSingleton()
    {
        static HkxFile file;
        return std::addressof(file);
    }

    bool loadFile(std::string_view path);
    void rebuildRefList();
    void rebuildVariableList();

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


    inline std::vector<Variable> getVariableList() { return m_variable_list; }

private:
    std::shared_ptr<spdlog::logger> m_file_logger;

    pugi::xml_document m_doc;
    pugi::xml_node     m_data_node, m_graph_data_node, m_graph_str_data_node, m_var_value_node;
    bool               m_loaded = false;

    NodeList                                             m_node_list;
    std::unordered_map<std::string_view, pugi::xml_node> m_node_name_map;
    std::unordered_map<std::string_view, NodeList>       m_node_ref_list_map;
    std::unordered_map<std::string_view, NodeList>       m_node_class_map;

    std::vector<Variable> m_variable_list;
};

} // namespace Hkx

} // namespace Haviour
