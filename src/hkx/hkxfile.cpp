#include "hkxfile.h"
#include "hkclass.inl"

#include <memory>
#include <execution>
#include <filesystem>

#include <spdlog/spdlog.h>

namespace Haviour
{
namespace Hkx
{
void HkxFile::loadFile(std::string_view path)
{
    m_path     = path;
    m_filename = std::filesystem::path(path).filename().string();

    m_loaded = false;

    auto file_logger = spdlog::default_logger()->clone(m_filename);

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

    m_latest_id = 0;

    m_obj_list.clear();
    m_obj_class_list.clear();
    m_obj_ref_list.clear();
    m_obj_ref_by_list.clear();

    // populate node list
    for (auto hkobject = m_data_node.child("hkobject"); hkobject; hkobject = hkobject.next_sibling("hkobject"))
    {
        std::string name = hkobject.attribute("name").as_string();
        if (name.empty() || !name.starts_with('#'))
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

    m_root_obj = m_data_node.find_child_by_attribute("class", "hkRootLevelContainer");
    if (!m_root_obj)
    {
        file_logger->error("Couldn't find root level object!");
        return;
    }

    reindexObjInternal(); // in case some file don't follow the 4 digit indexing

    m_loaded = true;
}

void HkxFile::saveFile(std::string_view path)
{
    if (path.empty())
        path = m_path;
    else
        m_path = path;
    m_filename = std::filesystem::path(path).filename().string();

    auto file_logger = spdlog::default_logger()->clone(m_filename);

    file_logger->info("Saving file...");

    if (!m_doc.save_file(path.data(), "    ", pugi::format_default | pugi::format_no_escapes))
        file_logger->warn("Failed to save file!");
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
    auto file_logger = spdlog::default_logger()->clone(m_filename);

    if (getObj(id) && getObj(parent_id))
    {
        auto& ref_by_list = m_obj_ref_by_list.find(id)->second;
        if (ref_by_list.contains(parent_id))
            ref_by_list.erase(std::string{parent_id});
        else
            file_logger->warn("Attempting to dereference {0} from {1} but {0} is not referenced by {1}!", id, parent_id);

        auto& ref_list = m_obj_ref_list.find(parent_id)->second;
        if (ref_list.contains(id))
            ref_list.erase(std::string{id});
        else
            file_logger->warn("Attempting to dereference {0} from {1} but {1} is not referencing {0}!", id, parent_id);
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

    for (auto& [id, obj] : m_obj_list)
        buildRefList(id);
}
void HkxFile::buildRefList(std::string_view id)
{
    auto obj = getObj(id);
    if (!obj) return;

    auto refed_objs = m_obj_ref_list.find(id)->second;
    for (auto ref_obj_id : refed_objs)
        deRef(ref_obj_id, id);

    struct Walker : pugi::xml_tree_walker
    {
        StringSet* m_refs;

        virtual bool for_each(pugi::xml_node& node)
        {
            if (node.type() == pugi::node_pcdata)
            {
                std::string text = node.value();
                size_t      pos  = text.find('#');
                while (pos != text.npos)
                {
                    if (!pos || (text[pos - 1] != '&')) // in case html entity, fuck html entities
                        m_refs->emplace(&text[pos], 5);
                    pos = text.find('#', pos + 1);
                }
                node.text() = text.c_str();
            }
            return true; // continue traversal
        }
    } walker;
    walker.m_refs = &m_obj_ref_list.find(id)->second;
    obj.traverse(walker);

    for (auto& refed_id : *walker.m_refs)
        m_obj_ref_by_list.find(refed_id)->second.emplace(id);
}

std::string_view HkxFile::addObj(std::string_view hkclass)
{
    auto file_logger = spdlog::default_logger()->clone(m_filename);

    file_logger->info("Attempting to add new {} ...", hkclass);
    if (m_latest_id >= 9999)
    {
        file_logger->warn("Exceeding object id limit (9999). Consider doing some cleaning or making a separate file for one of the branches.");
        return {};
    }

    auto& class_def_map = getClassDefaultMap();
    if (class_def_map.contains(hkclass))
    {
        auto new_obj              = appendXmlString(m_data_node, class_def_map.at(hkclass));
        auto id_str               = std::format("#{:04}", ++m_latest_id);
        new_obj.attribute("name") = id_str.c_str();

        m_obj_list[id_str] = new_obj;
        if (!m_obj_class_list.contains(hkclass))
            m_obj_class_list[std::string(hkclass)] = {};
        m_obj_class_list.find(hkclass)->second.push_back(id_str);
        m_obj_ref_by_list[id_str] = {};
        m_obj_ref_list[id_str]    = {};

        file_logger->info("Added new object {}", id_str);
        HkxFileManager::getSingleton()->dispatch(kEventObjChanged);
        return m_obj_list.find(id_str)->first;
    }
    else
    {
        file_logger->warn("Adding new object of class {} is currently not supported.", hkclass);
        return {};
    }
}
void HkxFile::delObj(std::string_view id)
{
    auto file_logger = spdlog::default_logger()->clone(m_filename);

    file_logger->info("Attempting to delete object {} ...", id);

    auto obj = getObj(id);
    if (!obj)
    {
        file_logger->warn("No object {}", id);
        return;
    }

    if (isObjEssential(id))
    {
        file_logger->warn("Object {} is essential.", id);
        return;
    }

    auto hkclass = obj.attribute("class").as_string();

    if (hasRef(id))
    {
        file_logger->warn("Object {} is still referenced by other objects.", id);
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

    file_logger->info("Object {} deleted.", id);
    HkxFileManager::getSingleton()->dispatch(kEventObjChanged);
}

void HkxFile::reindexObj(uint16_t start_id)
{
    auto file_logger = spdlog::default_logger()->clone(m_filename);

    file_logger->info("Attempting to reindex all objects...");

    reindexObjInternal(start_id);

    file_logger->info("All objects reindexed.");
    HkxFileManager::getSingleton()->dispatch(kEventObjChanged);
}

void HkxFile::reindexObjInternal(uint16_t start_id)
{
    // get the id map
    StringMap<std::string> remap = {};

    auto                     new_idx = start_id;
    std::vector<std::string> obj_list;
    getObjList(obj_list);
    std::ranges::sort(obj_list);
    for (auto& id : obj_list)
    {
        remap[id] = fmt::format("#{:04}", new_idx);
        ++new_idx;
    }
    // for (uint16_t i = 0; i <= m_latest_id; ++i)
    //     if (auto old_id_str = fmt::format("#{:04}", i); m_obj_list.contains(old_id_str))
    //     {
    //         remap[old_id_str] = fmt::format("#{:04}", new_idx);
    //         ++new_idx;
    //     }
    m_latest_id = new_idx - 1;

    // remap the lists
    {
        decltype(m_obj_list) new_list = {};
        for (auto& [key, val] : m_obj_list)
            new_list[remap[key]] = val;
        m_obj_list = new_list;
    }
    {
        decltype(m_obj_class_list) new_list = {};
        for (auto& [key, val] : m_obj_class_list)
            new_list[key] = {};
        std::for_each(std::execution::par,
                      m_obj_class_list.begin(), m_obj_class_list.end(),
                      [&](auto& pair) {
                          std::ranges::transform(pair.second, std::back_inserter(new_list[pair.first]), [&](const std::string& item) { return remap[item]; });
                      });
        m_obj_class_list = new_list;
    }
    {
        decltype(m_obj_ref_by_list) new_list = {};
        for (auto& [key, val] : m_obj_ref_by_list)
            new_list[remap[key]] = {};
        std::for_each(std::execution::par,
                      m_obj_ref_by_list.begin(), m_obj_ref_by_list.end(),
                      [&](auto& pair) {
                          auto& new_set = new_list[remap[pair.first]];
                          std::ranges::transform(pair.second, std::inserter(new_set, new_set.end()), [&](const std::string& item) { return remap[item]; });
                      });
        m_obj_ref_by_list = new_list;
    }
    {
        decltype(m_obj_ref_list) new_list = {};
        for (auto& [key, val] : m_obj_ref_list)
            new_list[remap[key]] = {};
        std::for_each(std::execution::par,
                      m_obj_ref_list.begin(), m_obj_ref_list.end(),
                      [&](auto& pair) {
                          auto& new_set = new_list[remap[pair.first]];
                          std::ranges::transform(pair.second, std::inserter(new_set, new_set.end()), [&](const std::string& item) { return remap[item]; });
                      });
        m_obj_ref_list = new_list;
    }

    // remap the xml
    struct Walker : pugi::xml_tree_walker
    {
        StringMap<std::string>* m_remap;

        virtual bool for_each(pugi::xml_node& node)
        {
            if (node.type() == pugi::node_pcdata)
            {
                std::string text = node.value();
                size_t      pos  = text.find('#');
                while (pos != text.npos)
                {
                    if (!pos || (text[pos - 1] != '&')) // in case html entity, fuck html entities
                    {
                        auto next_break = pos + 1;
                        while ((next_break != text.size()) && (text[next_break] >= '0') && (text[next_break] <= '9')) ++next_break;
                        auto rep_len = next_break - pos;
                        text.replace(pos, rep_len, m_remap->at(std::string(&text[pos], rep_len)));
                        pos = next_break;
                    }

                    pos = text.find('#', pos + 1);
                }
                node.text() = text.c_str();
            }
            return true; // continue traversal
        }
    } walker;
    walker.m_remap = &remap;
    m_data_node.traverse(walker);

    for (auto [key, obj] : m_obj_list)
        obj.attribute("name") = key.c_str();

    m_data_node.parent().attribute("toplevelobject") = m_root_obj.attribute("name").as_string();
}

//////////////////// BEHAVIOUR

void BehaviourFile::loadFile(std::string_view path)
{
    HkxFile::loadFile(path);
    if (!m_loaded)
        return;

    auto file_logger = spdlog::default_logger()->clone(m_filename);

    m_loaded = false;

    // get essential nodes
    m_graph_obj = m_data_node.find_child_by_attribute("class", "hkbBehaviorGraph");
    if (!(m_graph_obj && isRefBy(m_graph_obj.attribute("name").as_string(), m_root_obj)))
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
    auto var_ptr_node   = m_var_value_obj.getByName("variantVariableValues");

    auto evt_info_node = m_graph_data_obj.getByName("eventInfos");
    auto evt_name_node = m_graph_str_data_obj.getByName("eventNames");

    auto prop_info_node = m_graph_data_obj.getByName("characterPropertyInfos");
    auto prop_name_node = m_graph_str_data_obj.getByName("characterPropertyNames");

    if (!(var_info_node && var_name_node && var_value_node && var_quad_node && var_ptr_node &&
          evt_info_node && evt_name_node &&
          prop_info_node && prop_name_node))
    {
        file_logger->error("Couldn't find variable / animation event / character property info!");
        return;
    }

    m_var_manager.buildEntryList(var_name_node, var_info_node, var_value_node, var_quad_node, var_ptr_node);
    m_evt_manager.buildEntryList(evt_name_node, evt_info_node);
    m_prop_manager.buildEntryList(prop_name_node, prop_info_node);

    buildRefList();

    file_logger->info("File successfully loaded with {} hkobjects, {} hkclasses, {} variables, {} animation events and {} character properties",
                      m_obj_list.size(), m_obj_class_list.size(), m_var_manager.size(), m_evt_manager.size(), m_prop_manager.size());
    m_loaded = true;
}

void BehaviourFile::saveFile(std::string_view path)
{
    reindexEvents();
    reindexProps();
    reindexVariables();

    HkxFile::saveFile(path);
}

// Should've used xpath but whatever
pugi::xml_node BehaviourFile::getFirstVarRef(size_t idx, bool ret_obj)
{
    auto var_idx_node = m_data_node.find_node([=](auto node) { return isVarNode(node) && (node.text().as_ullong() == idx); });
    if (var_idx_node)
        return ret_obj ? getParentObj(var_idx_node) : var_idx_node;
    return {};
}

pugi::xml_node BehaviourFile::getFirstEventRef(size_t idx, bool ret_obj)
{
    auto evt_idx_node = m_data_node.find_node([=](auto node) { return isEvtNode(node) && (node.text().as_ullong() == idx); });
    if (evt_idx_node)
        return ret_obj ? getParentObj(evt_idx_node) : evt_idx_node;
    return {};
}
pugi::xml_node BehaviourFile::getFirstPropRef(size_t idx, bool ret_obj)
{
    auto prop_idx_node = m_data_node.find_node([=](auto node) { return isPropNode(node) && (node.text().as_ullong() == idx); });
    if (prop_idx_node)
        return ret_obj ? getParentObj(prop_idx_node) : prop_idx_node;
    return {};
}

void BehaviourFile::reindexVariables()
{
    auto remap = m_var_manager.reindex();

    struct Walker : pugi::xml_tree_walker
    {
        robin_hood::unordered_map<size_t, size_t>* m_remap;

        virtual bool for_each(pugi::xml_node& node)
        {
            if ((node.type() == pugi::node_element) && isVarNode(node) && m_remap->contains(node.text().as_llong()))
                node.text() = m_remap->at(node.text().as_llong());
            return true; // continue traversal
        }
    } walker;
    walker.m_remap = &remap;
    m_data_node.traverse(walker);
}
void BehaviourFile::reindexEvents()
{
    auto remap = m_evt_manager.reindex();

    struct Walker : pugi::xml_tree_walker
    {
        robin_hood::unordered_map<size_t, size_t>* m_remap;

        virtual bool for_each(pugi::xml_node& node)
        {
            if ((node.type() == pugi::node_element) && isEvtNode(node) && m_remap->contains(node.text().as_llong()))
                node.text() = m_remap->at(node.text().as_llong());
            return true; // continue traversal
        }
    } walker;
    walker.m_remap = &remap;
    m_data_node.traverse(walker);
}
void BehaviourFile::reindexProps()
{
    auto remap = m_prop_manager.reindex();

    struct Walker : pugi::xml_tree_walker
    {
        robin_hood::unordered_map<size_t, size_t>* m_remap;

        virtual bool for_each(pugi::xml_node& node)
        {
            if ((node.type() == pugi::node_element) && isPropNode(node) && m_remap->contains(node.text().as_llong()))
                node.text() = m_remap->at(node.text().as_llong());
            return true; // continue traversal
        }
    } walker;
    walker.m_remap = &remap;
    m_data_node.traverse(walker);
}

void BehaviourFile::cleanupVariables()
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

void BehaviourFile::cleanupEvents()
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

void BehaviourFile::cleanupProps()
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

//////////////////////    SKELLY

void SkeletonFile::loadFile(std::string_view path)
{
    HkxFile::loadFile(path);
    if (!m_loaded)
        return;
    m_loaded = false;

    auto file_logger = spdlog::default_logger()->clone(m_filename);

    auto skels = m_data_node.select_nodes("hkobject[@class='hkaSkeleton']");
    for (auto skel : skels)
    {
        if (std::string_view(skel.node().getByName("name").text().as_string()).starts_with("Ragdoll"))
            m_skel_rag_obj = skel.node();
        else
            m_skel_obj = skel.node();
    }

    if (!m_skel_obj)
    {
        file_logger->error("Couldn't find main skeleton!");
        return;
    }
    if (!m_skel_rag_obj)
        file_logger->error("Couldn't find ragdoll skeleton!");

    file_logger->info("Skeleton file loaded.");
    m_loaded = true;
}

void SkeletonFile::getBoneList(std::vector<std::string_view>& out, bool ragdoll)
{
    for (auto bone : (ragdoll ? m_skel_rag_obj : m_skel_obj).getByName("bones").children())
        out.push_back(bone.getByName("name").text().as_string());
}

//////////////////////    CHAR FILE

void CharacterFile::loadFile(std::string_view path)
{
    HkxFile::loadFile(path);
    if (!m_loaded)
        return;
    m_loaded = false;

    auto file_logger = spdlog::default_logger()->clone(m_filename);

    m_char_data_obj     = m_data_node.find_child_by_attribute("class", "hkbCharacterData");
    m_char_str_data_obj = m_data_node.find_child_by_attribute("class", "hkbCharacterStringData");
    m_var_value_obj     = m_data_node.find_child_by_attribute("class", "hkbVariableValueSet");

    if (!(m_char_data_obj && m_char_str_data_obj && m_var_value_obj))
    {
        file_logger->error("Couldn't find essential objects! (hkbBehaviorGraphData, hkbBehaviorGraphStringData or hkbVariableValueSet)");
        return;
    }

    m_anim_name_node = m_char_str_data_obj.getByName("animationNames");
    if (!m_anim_name_node)
    {
        file_logger->error("Couldn't find animationNames!");
        return;
    }

    auto var_info_node  = m_char_data_obj.getByName("characterPropertyInfos");
    auto var_name_node  = m_char_str_data_obj.getByName("characterPropertyNames");
    auto var_value_node = m_var_value_obj.getByName("wordVariableValues");
    auto var_quad_node  = m_var_value_obj.getByName("quadVariableValues");
    auto var_ptr_node   = m_var_value_obj.getByName("variantVariableValues");

    if (!(var_info_node && var_name_node && var_value_node && var_quad_node && var_ptr_node))
    {
        file_logger->error("Couldn't find character property info!");
        return;
    }

    m_prop_manager.buildEntryList(var_name_node, var_info_node, var_value_node, var_quad_node, var_ptr_node);

    file_logger->info("Character file successfully loaded with {} hkobjects, {} hkclasses, {} character properties",
                      m_obj_list.size(), m_obj_class_list.size(), m_prop_manager.size());
    m_loaded = true;
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
        m_current_file = &m_files.back();
        dispatch(kEventFileChanged);
    }
    else
        m_files.pop_back();
}

void HkxFileManager::saveFile(std::string_view path)
{
    if (!m_current_file)
        return;

    m_current_file->saveFile(path);
}
} // namespace Hkx
} // namespace Haviour