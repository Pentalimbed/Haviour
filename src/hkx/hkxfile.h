#pragma once
#include "linkedmanager.h"
#include "utils.h"

#include <algorithm>
#include <deque>
#include <functional>

#include <eventpp/eventdispatcher.h>
#include <robin_hood.h>

namespace Haviour
{
namespace Hkx
{
enum HkxFileEventEnum
{
    kEventFileChanged,
    kEventAddObj,
    kEventDelObj,
    kEventTidyUp
};

// Single file
class HkxFile
{
public:
    void                    loadFile(std::string_view path);
    void                    saveFile(std::string_view path = {});
    inline bool             isFileLoaded() { return m_loaded; }
    inline std::string_view getPath() { return m_path; }

    void addRef(std::string_view id, std::string_view parent_id);
    void deRef(std::string_view id, std::string_view parent_id);
    void buildRefList();

    inline pugi::xml_node getNode(std::string_view id)
    {
        if (m_obj_list.contains(id))
            return m_obj_list.find(id)->second; // until c++23 transparent searching for [] is implemented
        return {};
    }

    inline bool isObjEssential(std::string_view id)
    {
        auto essential_obj = {m_root_obj, m_graph_obj, m_graph_data_obj, m_graph_str_data_obj, m_var_value_obj};
        return std::ranges::find(essential_obj, getNode(id)) != essential_obj.end();
    }


    AnimationEventManager    m_evt_manager;
    VariableManager          m_var_manager;
    CharacterPropertyManager m_prop_manager;

    pugi::xml_node getFirstVarRef(size_t idx, bool ret_obj = true);
    pugi::xml_node getFirstEventRef(size_t idx, bool ret_obj = true);
    pugi::xml_node getFirstPropRef(size_t idx, bool ret_obj = true);

    void reindexVariables();
    void reindexEvents();
    void reindexProperties();

    // removed unreferenced
    void cleanupVariables();
    void cleanupEvents();
    void cleanupProps();

private:
    bool m_loaded = false;

    std::string        m_path;
    pugi::xml_document m_doc;
    pugi::xml_node     m_data_node;
    pugi::xml_node     m_root_obj, m_graph_obj, m_graph_data_obj, m_graph_str_data_obj, m_var_value_obj; // Essential objects

    uint16_t m_latest_id = 0;

    StringMap<pugi::xml_node>                              m_obj_list;
    StringMap<std::vector<std::string_view>>               m_obj_class_list;
    StringMap<robin_hood::unordered_set<std::string_view>> m_ref_list;
};

// Managing files
class HkxFileManager : public eventpp::EventDispatcher<HkxFileEventEnum, void()>
{
public:
    static HkxFileManager* getSingleton();

    inline void setCurrentFile(int idx)
    {
        m_current_file = idx;
        dispatch(kEventFileChanged);
    }
    inline size_t   getCurrentFileIdx() { return m_current_file; }
    inline HkxFile& getCurrentFile() { return m_files[m_current_file]; }

    inline bool                          isFileSelected() { return m_current_file >= 0; }
    inline std::vector<std::string_view> getPathList()
    {
        std::vector<std::string_view> retval;
        retval.reserve(m_files.size());
        std::ranges::transform(m_files, std::back_inserter(retval), [](HkxFile& file) { return file.getPath(); });
        return retval;
    }

    void        newFile(std::string_view path);
    void        saveFile(std::string_view path = {});
    inline void closeCurrentFile()
    {
        if (m_current_file >= 0)
        {
            m_files.erase(m_files.begin() + m_current_file);
            m_current_file = m_files.empty() ? -1 : std::clamp(m_current_file, 0, (int)m_files.size() - 1);
            dispatch(kEventFileChanged);
        }
    }
    inline void closeAllFiles()
    {
        m_current_file = -1;
        m_files.clear();
        dispatch(kEventFileChanged);
    }

private:
    int                  m_current_file = -1;
    std::vector<HkxFile> m_files;
};

} // namespace Hkx
} // namespace Haviour