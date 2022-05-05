#include "hkxfile.h"
#include "hkclass.inl"
#include "hkutils.h"

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
    m_obj_ref_list.clear();
    m_obj_ref_by_list.clear();

    auto result = m_doc.load_file(path.data(), pugi::parse_default & (~pugi::parse_escapes));
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

    m_graph_data_obj     = getObj(m_graph_obj.getByName("data").text().as_string());
    m_graph_str_data_obj = getObj(m_graph_data_obj.getByName("stringData").text().as_string());
    m_var_value_obj      = getObj(m_graph_data_obj.getByName("variableInitialValues").text().as_string());
    if (!(m_graph_data_obj && m_graph_str_data_obj && m_var_value_obj))
    {
        file_logger->error("Couldn't find essential objects! (hkbBehaviorGraphData, hkbBehaviorGraphStringData or hkbVariableValueSet)");
        return;
    }

    // get manager nodes
    auto var_info_node  = m_graph_data_obj.getByName("variableInfos");
    auto var_name_node  = m_graph_str_data_obj.getByName("variableNames");
    auto var_value_node = m_var_value_obj.getByName("wordVariableValues");
    auto var_quad_node  = m_var_value_obj.getByName("quadVariableValues");

    auto evt_info_node = m_graph_data_obj.getByName("eventInfos");
    auto evt_name_node = m_graph_str_data_obj.getByName("eventNames");

    auto prop_info_node = m_graph_data_obj.getByName("characterPropertyInfos");
    auto prop_name_node = m_graph_str_data_obj.getByName("characterPropertyNames");

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
    else
        m_path = path;

    spdlog::info("Saving file: {}", path);

    m_doc.save_file(path.data(), " ", pugi::format_default | pugi::format_no_escapes);
}

void HkxFile::addRef(std::string_view id, std::string_view parent_id)
{
    if (getObj(id) && getObj(parent_id))
    {
        m_obj_ref_list.find(parent_id)->second.emplace(id);
        m_obj_ref_by_list.find(id)->second.emplace(parent_id);
    }
}
void HkxFile::deRef(std::string_view id, std::string_view parent_id)
{
    if (getObj(id) && getObj(parent_id))
    {
        auto& ref_by_list = m_obj_ref_by_list.find(id)->second;
        if (ref_by_list.contains(parent_id))
            ref_by_list.erase(std::string{parent_id});
        else
            spdlog::warn("Attempting to dereference {0} from {1} but {0} is not referenced by {1}!", id, parent_id);

        auto& ref_list = m_obj_ref_list.find(parent_id)->second;
        if (ref_list.contains(id))
            ref_list.erase(std::string{id});
        else
            spdlog::warn("Attempting to dereference {0} from {1} but {1} is not referencing {0}!", id, parent_id);
    }
}

void HkxFile::buildRefList()
{
    m_obj_ref_by_list.clear();
    m_obj_ref_list.clear();

    for (auto& [_, obj] : m_obj_list)
    {
        m_obj_ref_by_list[obj.attribute("name").as_string()] = {};
        m_obj_ref_list[obj.attribute("name").as_string()]    = {};
    }

    std::for_each(
        std::execution::par,
        m_obj_list.begin(), m_obj_list.end(),
        [=](auto entry) {
            std::string_view id = entry.second.attribute("name").as_string();
            for (auto& [parent_id, parent_obj] : m_obj_list)
            {
                if (id != parent_id && isRefBy(id, parent_obj))
                    if (getObj(id) && getObj(parent_id))
                        m_obj_ref_by_list.find(id)->second.emplace(parent_id);
            }
        }); // try to be fast here
    for (auto& [id, ref_by] : m_obj_ref_by_list)
        for (auto parent : ref_by)
            m_obj_ref_list.find(parent)->second.emplace(id);
}
void HkxFile::buildRefList(std::string_view id)
{
    auto obj = getObj(id);
    if (!obj) return;

    auto refed_objs = m_obj_ref_list.find(id)->second;
    for (auto ref_obj_id : refed_objs)
        deRef(ref_obj_id, id);

    for (auto& [ref_id, ref_obj] : m_obj_list)
        if (id != ref_id && isRefBy(ref_id, obj))
            addRef(ref_id, id);
}

std::string_view HkxFile::addObj(std::string_view hkclass)
{
    spdlog::info("Attempting to add new {}", hkclass);
    if (m_latest_id >= 9999)
    {
        spdlog::warn("Exceeding object id limit (9999). Consider doing some cleaning or making a separate file for one of the branches.");
        return {};
    }

    auto& class_def_map = getClassDefaultMap();
    if (class_def_map.contains(hkclass))
    {
        auto retval              = appendXmlString(m_data_node, class_def_map.at(hkclass));
        auto id_str              = std::format("#{:04}", ++m_latest_id);
        retval.attribute("name") = id_str.c_str();

        m_obj_list[id_str] = retval;
        if (!m_obj_class_list.contains(hkclass))
            m_obj_class_list[std::string(hkclass)] = {};
        m_obj_class_list.find(hkclass)->second.push_back(id_str);
        m_obj_ref_by_list[id_str] = {};
        m_obj_ref_list[id_str]    = {};

        spdlog::info("Added new object {}", id_str);
        HkxFileManager::getSingleton()->dispatch(kEventObjChanged);
        return m_obj_list.find(id_str)->first;
    }
    else
    {
        spdlog::warn("Adding new object of class {} is currently not supported.", hkclass);
        return {};
    }
}
void HkxFile::delObj(std::string_view id)
{
    spdlog::info("Attempting to delete object {}", id);

    auto obj = getObj(id);
    if (!obj)
    {
        spdlog::warn("No object {}", id);
        return;
    }

    if (isObjEssential(id))
    {
        spdlog::warn("Object {} is essential.", id);
        return;
    }

    auto hkclass = obj.attribute("class").as_string();

    if (hasRef(id))
    {
        spdlog::warn("Object {} is still referenced by other objects.", id);
        return;
    }

    m_data_node.remove_child(obj);

    m_obj_list.erase(m_obj_list.find(id));

    auto class_list = m_obj_class_list.find(hkclass);
    std::erase(class_list->second, id);
    if (class_list->second.empty())
        m_obj_class_list.erase(class_list);

    for (auto& parent_id : m_obj_ref_by_list.find(id)->second)
        m_obj_ref_list[parent_id].erase(m_obj_ref_list[parent_id].find(id));
    for (auto& child_id : m_obj_ref_list.find(id)->second)
        m_obj_ref_by_list[child_id].erase(m_obj_ref_by_list[child_id].find(id));

    m_obj_ref_by_list.erase(m_obj_ref_by_list.find(id));
    m_obj_ref_list.erase(m_obj_ref_list.find(id));

    spdlog::info("Object {} deleted.", id);
    HkxFileManager::getSingleton()->dispatch(kEventObjChanged);
}

// #define FIND_PARAM_VAL(param, val) m_data_node.find_node([=](pugi::xml_node node) { return !strcmp(node.attribute("name").as_string(), param) && (node.text().as_int() == val); });

static bool isVarNode(pugi::xml_node node)
{
    auto var_name = node.attribute("name").as_string();
    return (!strcmp(var_name, "variableIndex") && !strcmp(node.parent().getByName("bindingType").text().as_string(), "BINDING_TYPE_VARIABLE")) ||
        !strcmp(var_name, "syncVariableIndex") ||     // hkbStateMachine
        !strcmp(var_name, "assignmentVariableIndex"); // hkbExpressionData
}

static bool isEvtNode(pugi::xml_node node)
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

static bool isPropNode(pugi::xml_node node)
{
    return (!strcmp(node.attribute("name").as_string(), "variableIndex") && !strcmp(node.parent().getByName("bindingType").text().as_string(), "BINDING_TYPE_CHARACTER_PROPERTY"));
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
    //                                                                              !strcmp(node.parent().getByName( "bindingType").text().as_string(), "BINDING_TYPE_VARIABLE"); });
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
    //                                                                               !strcmp(node.parent().getByName( "bindingType").text().as_string(), "BINDING_TYPE_CHARACTER_PROPERTY"); });
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
            if (isVarNode(node) && m_remap->contains(node.text().as_llong()))
                node.text() = m_remap->at(node.text().as_llong());
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
            if (isEvtNode(node) && m_remap->contains(node.text().as_llong()))
                node.text() = m_remap->at(node.text().as_llong());
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
            if (isPropNode(node) && m_remap->contains(node.text().as_llong()))
                node.text() = m_remap->at(node.text().as_llong());
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

void HkxFileManager::loadFile(std::string_view path)
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

    m_files[m_current_file].saveFile(path);
}
} // namespace Hkx
} // namespace Haviour