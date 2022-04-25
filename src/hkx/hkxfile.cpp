#include "hkxfile.h"

#include <memory>
#include <execution>
#include <spdlog/spdlog.h>

namespace Haviour
{
namespace Hkx
{
void HkxFile::loadFile(std::string_view path)
{
    auto file_logger  = spdlog::default_logger()->clone(path.data());
    auto file_manager = HkxFileManager::getSingleton();

    m_path = path;

    m_loaded    = false;
    m_latest_id = 0;

    m_obj_list.clear();
    m_obj_class_list.clear();
    m_ref_list.clear();

    auto result = m_doc.load_file(path.data());
    if (!result)
    {
        file_logger->error("File parsed with errors.\n\tError description: {}\n\tat location {}", path, result.description(), result.offset);
        return;
    }
    file_logger->info("File parsed without errors.", path);

    // get the data node
    m_data_node = m_doc.child("hkpackfile").child("hksection");
    if (!m_data_node)
    {
        file_logger->error("File is not a hkxpackfile [no hkpackfile or hksection].");
        return;
    }

    // populate node list
    for (auto hkobject = m_data_node.child("hkobject"); hkobject; hkobject = hkobject.next_sibling("hkobject"))
    {
        std::string name = hkobject.attribute("name").as_string();
        if (name.empty() || (name.length() != 5) || !name.starts_with('#'))
        {
            file_logger->error("hkobject at location {} has no valid name.", hkobject.path());
            return;
        }

        std::string hkclass = hkobject.attribute("class").as_string();
        if (hkclass.empty())
        {
            file_logger->error("hkobject {} has no class.", name);
            return;
        }

        m_obj_list[name] = hkobject;
        if (!m_obj_class_list.contains(hkclass))
            m_obj_class_list[hkclass] = {};
        m_obj_class_list[hkclass].push_back(name);

        uint16_t id = std::atoi(name.data() + 1); // There's a potential crash here...
        m_latest_id = std::max(id, m_latest_id);
    }

    // get essential nodes
    m_root_obj  = m_data_node.find_child_by_attribute("class", "hkRootLevelContainer");
    m_graph_obj = m_data_node.find_child_by_attribute("class", "hkbBehaviorGraph");
    if (!(m_root_obj && m_graph_obj && isRefBy(m_graph_obj.attribute("name").as_string(), m_root_obj)))
    {
        file_logger->error("Couldn't find behavior graph!");
        return;
    }

    m_graph_data_obj     = getNode(m_graph_obj.find_child_by_attribute("name", "data").text().as_string());
    m_graph_str_data_obj = getNode(m_graph_data_obj.find_child_by_attribute("name", "stringData").text().as_string());
    m_var_value_obj      = getNode(m_graph_data_obj.find_child_by_attribute("name", "variableInitialValues").text().as_string());
    if (!(m_graph_data_obj && m_graph_str_data_obj && m_var_value_obj))
    {
        file_logger->error("Couldn't find essential objects! (hkbBehaviorGraphData, hkbBehaviorGraphStringData or hkbVariableValueSet)");
        return;
    }

    // get manager nodes
    auto var_info_node  = m_graph_data_obj.find_child_by_attribute("name", "variableInfos");
    auto var_name_node  = m_graph_str_data_obj.find_child_by_attribute("name", "variableNames");
    auto var_value_node = m_var_value_obj.find_child_by_attribute("name", "wordVariableValues");
    auto var_quad_node  = m_var_value_obj.find_child_by_attribute("name", "quadVariableValues");

    auto evt_info_node = m_graph_data_obj.find_child_by_attribute("name", "eventInfos");
    auto evt_name_node = m_graph_str_data_obj.find_child_by_attribute("name", "eventNames");

    auto prop_info_node = m_graph_data_obj.find_child_by_attribute("name", "characterPropertyInfos");
    auto prop_name_node = m_graph_str_data_obj.find_child_by_attribute("name", "characterPropertyNames");

    if (!(var_info_node && var_name_node && var_value_node && var_quad_node &&
          evt_info_node && evt_name_node &&
          prop_info_node && prop_name_node))
    {
        file_logger->error("Couldn't find variable / animation event / character property info!");
        return;
    }

    m_var_manager.buildEntryList(var_name_node, var_info_node, var_value_node, var_quad_node);
    m_evt_manager.buildEntryList(evt_name_node, evt_info_node);
    m_prop_manager.buildEntryList(prop_name_node, prop_info_node);

    buildRefList();

    file_logger->info("File successfully loaded with {} hkobjects, {} hkclasses, {} variables, {} animation events and {} character properties",
                      m_obj_list.size(), m_obj_class_list.size(), m_var_manager.size(), m_evt_manager.size(), m_prop_manager.size());
    m_loaded = true;
}

void HkxFile::saveFile(std::string_view path)
{
    reindexEvents();
    reindexProperties();
    reindexVariables();

    if (path.empty())
        path = m_path;

    m_doc.save_file(path.data());
}

void HkxFile::addRef(std::string_view id, std::string_view parent_id)
{
    if (getNode(id) && getNode(parent_id))
        m_ref_list.find(id)->second.emplace(parent_id);
}
void HkxFile::deRef(std::string_view id, std::string_view parent_id)
{
    if (getNode(id) && getNode(parent_id))
    {
        if (m_ref_list.find(id)->second.contains(parent_id))
            m_ref_list.find(id)->second.erase(parent_id);
        else
            spdlog::warn("Attempting to dereference {0} from {1} but {0} is not referenced by {1}!", id, parent_id);
    }
}

void HkxFile::buildRefList()
{
    m_ref_list.clear();

    for (auto& [_, obj] : m_obj_list)
        m_ref_list[obj.attribute("name").as_string()] = {};

    std::for_each(
        std::execution::par,
        m_obj_list.begin(), m_obj_list.end(),
        [=](auto entry) {
            std::string_view id = entry.second.attribute("name").as_string();
            for (auto& [_, parent_obj] : m_obj_list)
            {
                std::string_view parent_id = parent_obj.attribute("name").as_string();
                if (id != parent_id && isRefBy(id, parent_obj))
                    addRef(id, parent_id);
            }
        }); // try to be fast here
}

// #define FIND_PARAM_VAL(param, val) m_data_node.find_node([=](pugi::xml_node node) { return !strcmp(node.attribute("name").as_string(), param) && (node.text().as_int() == val); });

static bool isVarNode(pugi::xml_node node)
{
    return (!strcmp(node.attribute("name").as_string(), "variableIndex") && !strcmp(node.parent().find_child_by_attribute("name", "bindingType").text().as_string(), "BINDING_TYPE_VARIABLE")) ||
        !strcmp(node.attribute("name").as_string(), "syncVariableIndex") ||
        !strcmp(node.attribute("name").as_string(), "assignmentVariableIndex");
}

static bool isEvtNode(pugi::xml_node node)
{
    return !strcmp(node.attribute("name").as_string(), "eventId") ||
        !strcmp(node.attribute("name").as_string(), "enterEventId") ||
        !strcmp(node.attribute("name").as_string(), "exitEventId") ||
        (!strcmp(node.attribute("name").as_string(), "id") && (!strcmp(node.parent().parent().attribute("name").as_string(), "event") || !strcmp(node.parent().parent().attribute("name").as_string(), "events"))) ||
        !strcmp(node.attribute("name").as_string(), "assignmentEventIndex");
}

static bool isPropNode(pugi::xml_node node)
{
    return (!strcmp(node.attribute("name").as_string(), "variableIndex") && !strcmp(node.parent().find_child_by_attribute("name", "bindingType").text().as_string(), "BINDING_TYPE_CHARACTER_PROPERTY"));
}

// Should've used xpath but whatever
pugi::xml_node HkxFile::getFirstVarRef(size_t idx, bool ret_obj)
{
    auto var_idx_node = m_data_node.find_node([=](auto node) { return isVarNode(node) && (node.text().as_ullong() == idx); });
    if (var_idx_node)
        return ret_obj ? getParentObj(var_idx_node) : var_idx_node;
    return {};
    // {
    //     // bindings
    //     auto var_idx_node = m_data_node.find_node([=](pugi::xml_node node) { return !strcmp(node.attribute("name").as_string(), "variableIndex") &&
    //                                                                              (node.text().as_int() == idx) &&
    //                                                                              !strcmp(node.parent().find_child_by_attribute("name", "bindingType").text().as_string(), "BINDING_TYPE_VARIABLE"); });
    //     if (var_idx_node) return ret_obj ? var_idx_node.parent().parent().parent() : var_idx_node;
    // }
    // // hkbStateMachine
    // auto var_idx_node = FIND_PARAM_VAL("syncVariableIndex", idx);
    // if (var_idx_node) return ret_obj ? var_idx_node.parent() : var_idx_node;
    // // expressionsData
    // var_idx_node = FIND_PARAM_VAL("assignmentVariableIndex", idx);
    // if (var_idx_node) return ret_obj ? var_idx_node.parent().parent().parent() : var_idx_node;
    // return {};
}

pugi::xml_node HkxFile::getFirstEventRef(size_t idx, bool ret_obj)
{
    auto evt_idx_node = m_data_node.find_node([=](auto node) { return isEvtNode(node) && (node.text().as_ullong() == idx); });
    if (evt_idx_node)
        return ret_obj ? getParentObj(evt_idx_node) : evt_idx_node;
    return {};
    // // Transitions
    // auto evt_idx_node = FIND_PARAM_VAL("eventId", idx);
    // if (evt_idx_node) return evt_idx_node.parent().parent().parent();
    // evt_idx_node = FIND_PARAM_VAL("enterEventId", idx);
    // if (evt_idx_node) return evt_idx_node.parent().parent().parent().parent().parent();
    // evt_idx_node = FIND_PARAM_VAL("exitEventId", idx);
    // if (evt_idx_node) return evt_idx_node.parent().parent().parent().parent().parent();
    // // hkbClipTriggerArray
    // evt_idx_node = m_data_node.find_node(
    //     [=](pugi::xml_node node) {
    //         if(!strcmp(node.attribute("name").as_string(), "id") && (node.text().as_int() == idx))
    //         {
    //             auto parent = node.parent().parent();
    //             return !strcmp(parent.attribute("name").as_string(), "event");
    //         }
    //         return false; });
    // if (evt_idx_node) return evt_idx_node.parent().parent().parent().parent().parent();
    // // hkbStateMachineEventPropertyArray
    // evt_idx_node = m_data_node.find_node(
    //     [=](pugi::xml_node node) {
    //         if(!strcmp(node.attribute("name").as_string(), "id") && (node.text().as_int() == idx))
    //         {
    //             auto parent = node.parent().parent();
    //             return !strcmp(parent.attribute("name").as_string(), "events");
    //         }
    //         return false; });
    // if (evt_idx_node) return evt_idx_node.parent().parent().parent();
    // // expressionsData
    // evt_idx_node = FIND_PARAM_VAL("assignmentEventIndex", idx);
    // if (evt_idx_node) return evt_idx_node.parent().parent().parent();
    // return {};
}
pugi::xml_node HkxFile::getFirstPropRef(size_t idx, bool ret_obj)
{
    auto prop_idx_node = m_data_node.find_node([=](auto node) { return isPropNode(node) && (node.text().as_ullong() == idx); });
    if (prop_idx_node)
        return ret_obj ? getParentObj(prop_idx_node) : prop_idx_node;
    return {};
    // // bindings
    // auto charprop_idx_node = m_data_node.find_node([=](pugi::xml_node node) { return !strcmp(node.attribute("name").as_string(), "variableIndex") &&
    //                                                                               (node.text().as_int() == idx) &&
    //                                                                               !strcmp(node.parent().find_child_by_attribute("name", "bindingType").text().as_string(), "BINDING_TYPE_CHARACTER_PROPERTY"); });
    // if (charprop_idx_node) return charprop_idx_node.parent().parent().parent();
    // return {};
}

void HkxFile::reindexVariables()
{
    auto remap = m_var_manager.reindex();

    struct Walker : pugi::xml_tree_walker
    {
        robin_hood::unordered_map<size_t, size_t>* m_remap;

        virtual bool for_each(pugi::xml_node& node)
        {
            if (isVarNode(node))
                node.text() = m_remap->at(node.text().as_ullong());
            return true; // continue traversal
        }
    } walker;
    walker.m_remap = &remap;
    m_data_node.traverse(walker);
}
void HkxFile::reindexEvents()
{
    auto remap = m_evt_manager.reindex();

    struct Walker : pugi::xml_tree_walker
    {
        robin_hood::unordered_map<size_t, size_t>* m_remap;

        virtual bool for_each(pugi::xml_node& node)
        {
            if (isEvtNode(node))
                node.text() = m_remap->at(node.text().as_ullong());
            return true; // continue traversal
        }
    } walker;
    walker.m_remap = &remap;
    m_data_node.traverse(walker);
}
void HkxFile::reindexProperties()
{
    auto remap = m_prop_manager.reindex();

    struct Walker : pugi::xml_tree_walker
    {
        robin_hood::unordered_map<size_t, size_t>* m_remap;

        virtual bool for_each(pugi::xml_node& node)
        {
            if (isPropNode(node))
                node.text() = m_remap->at(node.text().as_ullong());
            return true; // continue traversal
        }
    } walker;
    walker.m_remap = &remap;
    m_data_node.traverse(walker);
}

void HkxFile::cleanupVariables()
{
    robin_hood::unordered_map<size_t, bool> refmap;
    for (size_t i = 0; i < m_var_manager.size(); ++i)
        refmap[i] = false;

    struct Walker : pugi::xml_tree_walker
    {
        robin_hood::unordered_map<size_t, bool>* m_refmap;

        virtual bool for_each(pugi::xml_node& node)
        {
            if (isVarNode(node))
                (*m_refmap)[node.text().as_ullong()] = true;
            return true; // continue traversal
        }
    } walker;
    walker.m_refmap = &refmap;
    m_data_node.traverse(walker);

    for (auto [idx, is_refed] : refmap)
        if (!is_refed)
            m_var_manager.delEntry(idx);
}

void HkxFile::cleanupEvents()
{
    robin_hood::unordered_map<size_t, bool> refmap;
    for (size_t i = 0; i < m_evt_manager.size(); ++i)
        refmap[i] = false;

    struct Walker : pugi::xml_tree_walker
    {
        robin_hood::unordered_map<size_t, bool>* m_refmap;

        virtual bool for_each(pugi::xml_node& node)
        {
            if (isEvtNode(node))
                (*m_refmap)[node.text().as_ullong()] = true;
            return true; // continue traversal
        }
    } walker;
    walker.m_refmap = &refmap;
    m_data_node.traverse(walker);

    for (auto [idx, is_refed] : refmap)
        if (!is_refed)
            m_evt_manager.delEntry(idx);
}

void HkxFile::cleanupProps()
{
    robin_hood::unordered_map<size_t, bool> refmap;
    for (size_t i = 0; i < m_prop_manager.size(); ++i)
        refmap[i] = false;

    struct Walker : pugi::xml_tree_walker
    {
        robin_hood::unordered_map<size_t, bool>* m_refmap;

        virtual bool for_each(pugi::xml_node& node)
        {
            if (isPropNode(node))
                (*m_refmap)[node.text().as_ullong()] = true;
            return true; // continue traversal
        }
    } walker;
    walker.m_refmap = &refmap;
    m_data_node.traverse(walker);

    for (auto [idx, is_refed] : refmap)
        if (!is_refed)
            m_prop_manager.delEntry(idx);
}

//////////////////////    FILE MANAGER
HkxFileManager* HkxFileManager::getSingleton()
{
    static HkxFileManager manager;
    return std::addressof(manager);
}

void HkxFileManager::newFile(std::string_view path)
{
    spdlog::info("Loading file: {}", path);

    m_files.push_back({});
    auto& file = m_files.back();
    file.loadFile(path);
    if (file.isFileLoaded())
    {
        m_current_file = m_files.size() - 1;
        dispatch(kEventFileChanged);
    }
    else
        m_files.pop_back();
}

void HkxFileManager::saveFile(std::string_view path)
{
    if (m_current_file < 0)
        return;

    spdlog::info("Saving file: {}", path);

    m_files[m_current_file].saveFile(path);
}
} // namespace Hkx
} // namespace Haviour