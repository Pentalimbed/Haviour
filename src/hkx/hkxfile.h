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
    kEventObjChanged
};

// generic unpacked hkx
class HkxFile
{
public:
    enum HkxFileType
    {
        kUnknown,
        kBehaviour,
        kCharacter,
        kSkeleton
    };
    virtual constexpr HkxFileType getType() { return kUnknown; }

    void                    loadFile(std::string_view path);
    virtual void            saveFile(std::string_view path = {});
    inline bool             isFileLoaded() { return m_loaded; }
    inline std::string_view getPath() { return m_path; }

    void        addRef(std::string_view id, std::string_view parent_id);
    void        deRef(std::string_view id, std::string_view parent_id);
    inline bool hasRef(std::string_view id) { return m_obj_ref_by_list.contains(id) && !m_obj_ref_by_list.find(id)->second.empty(); }
    inline void getObjRefs(std::string_view id, std::vector<std::string>& out)
    {
        if (m_obj_ref_by_list.contains(id))
            for (auto& id : m_obj_ref_by_list.find(id)->second)
                out.push_back(id);
    }
    inline void getRefedObjs(std::string_view id, std::vector<std::string>& out)
    {
        auto temp_size = out.size();
        if (m_obj_ref_list.contains(id))
            for (auto& id : m_obj_ref_list.find(id)->second)
                out.push_back(id);
        std::sort(out.begin() + temp_size, out.end());
    }
    void buildRefList();
    void buildRefList(std::string_view id);

    inline pugi::xml_node getObj(std::string_view id)
    {
        if (m_obj_list.contains(id))
            return m_obj_list.find(id)->second; // until c++23 transparent searching for [] is implemented
        return {};
    }
    inline void getObjList(std::vector<std::string>& out)
    {
        for (auto& pair : m_obj_list)
            out.push_back(pair.first);
    }
    inline void getObjListByClass(std::string_view hkclass, std::vector<std::string>& out)
    {
        if (m_obj_class_list.contains(hkclass))
            for (auto& id : m_obj_class_list.find(hkclass)->second)
                out.push_back(id);
    }
    inline void getClasses(std::vector<std::string>& out)
    {
        out.push_back("All");
        for (auto& pair : m_obj_class_list)
            out.push_back(pair.first);
        std::ranges::sort(out);
    }

    std::string_view addObj(std::string_view hkclass);
    void             delObj(std::string_view id);
    void             reindexObj(uint16_t start_id = 100);

    virtual bool isObjEssential(std::string_view id) { return m_root_obj == getObj(id); };

protected:
    bool m_loaded = false;

    std::string        m_path, m_filename;
    pugi::xml_document m_doc;
    pugi::xml_node     m_data_node, m_root_obj;
    uint16_t           m_latest_id = 0;

    StringMap<pugi::xml_node>           m_obj_list;
    StringMap<std::vector<std::string>> m_obj_class_list;
    StringMap<StringSet>                m_obj_ref_list;
    StringMap<StringSet>                m_obj_ref_by_list;

    void reindexObjInternal(uint16_t start_id = 100); // reindex w/o logging & event
};

// Single behaviour file
class BehaviourFile : public HkxFile
{
public:
    virtual constexpr HkxFileType getType() override { return kBehaviour; }

    void         loadFile(std::string_view path);
    virtual void saveFile(std::string_view path = {}) override;

    inline std::string_view getRootStateMachine()
    {
        return m_graph_obj.getByName("rootGenerator").text().as_string();
    }

    inline virtual bool isObjEssential(std::string_view id) override
    {
        auto essential_obj = {m_root_obj, m_graph_obj, m_graph_data_obj, m_graph_str_data_obj, m_var_value_obj};
        return std::ranges::find(essential_obj, getObj(id)) != essential_obj.end();
    }

    AnimationEventManager    m_evt_manager;
    VariableManager          m_var_manager;
    CharacterPropertyManager m_prop_manager;

    pugi::xml_node getFirstVarRef(size_t idx, bool ret_obj = true);
    pugi::xml_node getFirstEventRef(size_t idx, bool ret_obj = true);
    pugi::xml_node getFirstPropRef(size_t idx, bool ret_obj = true);

    void reindexVariables();
    void reindexEvents();
    void reindexProps();

    // removed unreferenced
    void cleanupVariables();
    void cleanupEvents();
    void cleanupProps();

    pugi::xml_node m_graph_obj, m_graph_data_obj, m_graph_str_data_obj, m_var_value_obj; // Essential objects
private:
};

// skeleton hkx
class SkeletonFile : public HkxFile
{
public:
    virtual constexpr HkxFileType getType() override { return kSkeleton; }

    void loadFile(std::string_view path);

    inline pugi::xml_node   getBoneNode(bool ragdoll = false) { return (ragdoll ? m_skel_rag_obj : m_skel_obj).getByName("bones"); }
    void                    getBoneList(std::vector<std::string_view>& out, bool ragdoll = false);
    inline std::string_view getBone(size_t idx, bool ragdoll = false)
    {
        return getNthChild(ragdoll ? m_skel_rag_obj.getByName("bones") : m_skel_obj.getByName("bones"), idx).getByName("name").text().as_string();
    }

    inline virtual bool isObjEssential(std::string_view) override { return true; }

private:
    pugi::xml_node m_skel_obj, m_skel_rag_obj;
};

// character hkx
class CharacterFile : public HkxFile
{
public:
    virtual constexpr HkxFileType getType() override { return kCharacter; }

    void         loadFile(std::string_view path);
    virtual void saveFile(std::string_view path = {}) override;

    inline virtual bool isObjEssential(std::string_view id) override
    {
        auto essential_obj = {/*m_root_obj, m_graph_obj,*/ m_char_data_obj, m_char_str_data_obj, m_var_value_obj};
        return std::ranges::find(essential_obj, getObj(id)) != essential_obj.end();
    }

    inline pugi::xml_node getAnimNames() { return m_anim_name_node; }

    VariableManager m_prop_manager; // naming a bit confusing but charprops are essentially variables in character files
private:
    pugi::xml_node m_anim_name_node;
    pugi::xml_node m_char_data_obj, m_char_str_data_obj, m_var_value_obj;
};

// Managing files
class HkxFileManager : public eventpp::EventDispatcher<HkxFileEventEnum, void()>
{
public:
    static HkxFileManager* getSingleton();

    void            setCurrentFile(int idx);
    void            setCurrentFile(HkxFile::HkxFileType type);
    inline HkxFile* getCurrentFile() { return m_current_file; }
    inline bool     isCurrentFileReady() { return m_current_file && m_current_file->isFileLoaded(); }

    inline std::vector<std::string_view> getPathList()
    {
        std::vector<std::string_view> retval;
        retval.reserve(m_files.size());
        std::ranges::transform(m_files, std::back_inserter(retval), [](BehaviourFile& file) { return file.getPath(); });
        return retval;
    }

    void        loadFile(std::string_view path);
    void        saveFile(std::string_view path = {});
    inline void closeCurrentFile()
    {
        if (m_current_file && m_current_file->getType() == HkxFile::kBehaviour)
        {
            auto idx = std::ranges::find_if(m_files, [=](auto& item) { return &item == m_current_file; }) - m_files.begin();
            m_files.erase(m_files.begin() + idx);
            m_current_file = m_files.empty() ? nullptr : &m_files[std::clamp(idx, (int64_t)0, (int64_t)m_files.size() - 1)];
            dispatch(kEventFileChanged);
        }
    }
    inline void closeAllFiles()
    {
        m_current_file = nullptr;
        m_files.clear();
        dispatch(kEventFileChanged);
    }

    SkeletonFile  m_skel_file;
    CharacterFile m_char_file;

private:
    HkxFile*                   m_current_file = nullptr;
    std::vector<BehaviourFile> m_files;
};

} // namespace Hkx
} // namespace Haviour