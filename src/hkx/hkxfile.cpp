#include "hkxfile.h"
#include "hkclass.h"

#include <GLFW/glfw3.h>

namespace Haviour
{
extern const char* g_window_title;
extern GLFWwindow* g_window;

namespace Hkx
{
void NameInfoManger::buildList(pugi::xml_node info_node, pugi::xml_node name_node)
{
    clear();
    m_info_node = info_node, m_name_node = name_node;

    auto name_item = name_node.first_child();
    auto info_item = info_node.first_child();
    int  idx       = 0;
    while (name_item && info_item)
    {
        m_entry_list.push_back({name_item, info_item, idx, false});
        name_item = name_item.next_sibling();
        info_item = info_item.next_sibling();
        ++idx;
    }
}

void NameInfoManger::clear()
{
    m_info_node = {}, m_name_node = {};
    m_entry_list.clear();
}

NameInfo NameInfoManger::addEntry()
{
    NameInfo retval;
    retval.name    = appendXmlString(m_name_node, m_name_def);
    retval.info    = appendXmlString(m_info_node, m_info_def);
    retval.invalid = false;

    retval.idx = m_entry_list.size();
    m_entry_list.push_back(retval);

    return retval;
}

void VariableManager::buildVarList(pugi::xml_node info_node, pugi::xml_node name_node, pugi::xml_node value_node)
{
    clear();
    m_info_node = info_node, m_name_node = name_node, m_value_node = value_node;

    auto name_item  = name_node.first_child();
    auto value_item = value_node.find_child_by_attribute("name", "wordVariableValues").first_child();
    auto info_item  = info_node.first_child();
    int  idx        = 0;
    while (name_item && value_item && info_item)
    {
        m_variable_list.push_back({name_item, value_item.find_child_by_attribute("name", "value"), info_item, idx, false});
        name_item  = name_item.next_sibling();
        value_item = value_item.next_sibling();
        info_item  = info_item.next_sibling();
        ++idx;
    }

    // get quads
    auto        quad_values_node = m_value_node.find_child_by_attribute("name", "quadVariableValues");
    auto        quad_count       = quad_values_node.attribute("numelements").as_uint();
    std::string quad_vals        = quad_values_node.text().as_string();
    std::erase_if(quad_vals, [](char& a) { return (a == '(') || (a == ')'); });
    std::istringstream quad_stream(quad_vals);
    for (size_t i = 0; i < quad_count; i++)
    {
        std::array<float, 4> array = {};
        quad_stream >> array[0] >> array[1] >> array[2] >> array[3];
        m_quads_value.push_back(array);
    }
}

Variable VariableManager::addVariable(VariableTypeEnum data_type)
{
    if ((data_type == VARIABLE_TYPE_INVALID) || (data_type == VARIABLE_TYPE_POINTER))
    {
        spdlog::warn("{} is not supported.", e_variableType[data_type + 1]);
        return {};
    }

    Variable retval;
    retval.name    = appendXmlString(m_name_node, g_def_hkStringPtr);
    retval.info    = appendXmlString(m_info_node, std::format(g_def_hkbVariableInfo, e_variableType[data_type + 1]));
    retval.value   = appendXmlString(m_value_node.find_child_by_attribute("name", "wordVariableValues"), g_def_hkbVariableValue).find_child_by_attribute("name", "value");
    retval.invalid = false;
    switch (data_type)
    {
        case VARIABLE_TYPE_POINTER:
            // NOT IMPLEMENTED
            break;
        case VARIABLE_TYPE_VECTOR3:
        case VARIABLE_TYPE_VECTOR4:
        case VARIABLE_TYPE_QUATERNION:
            retval.value.text() = m_quads_value.size();
            m_quads_value.push_back({0, 0, 0, 0});
            break;
        default:
            break;
    }

    retval.idx = m_variable_list.size();
    m_variable_list.push_back(retval);

    return retval;
}

bool HkxFile::loadFile(std::string_view path)
{
    m_file_logger = spdlog::default_logger()->clone(path.data());

    m_loaded    = false;
    m_data_node = m_graph_data_node = m_graph_str_data_node = m_var_value_node = {};

    m_node_list.clear();
    m_node_name_map.clear();
    m_node_class_map.clear();
    m_node_ref_list_map.clear();
    m_var_manager.clear();

    auto result = m_doc.load_file(path.data());
    if (!result)
    {
        m_file_logger->error("File parsed with errors.\n\tError description: {}\n\tat location {}", path, result.description(), result.offset);
        glfwSetWindowTitle(g_window, g_window_title);
        dispatch(kEventLoadFile);
        return false;
    }
    m_file_logger->info("File parsed without errors.", path);

    // get the data node
    m_data_node = m_doc.child("hkpackfile").child("hksection");
    if (!m_data_node)
    {
        m_file_logger->error("File is not a hkxpackfile [no hkpackfile or hksection].");
        glfwSetWindowTitle(g_window, g_window_title);
        dispatch(kEventLoadFile);
        return false;
    }

    // get the infos
    m_graph_data_node     = m_data_node.find_child_by_attribute("class", "hkbBehaviorGraphData");
    m_graph_str_data_node = m_data_node.find_child_by_attribute("class", "hkbBehaviorGraphStringData");
    m_var_value_node      = m_data_node.find_child_by_attribute("class", "hkbVariableValueSet");

    auto var_info_node = m_graph_data_node.find_child_by_attribute("name", "variableInfos");
    auto var_name_node = m_graph_str_data_node.find_child_by_attribute("name", "variableNames");

    auto evt_info_node = m_graph_data_node.find_child_by_attribute("name", "eventInfos");
    auto evt_name_node = m_graph_str_data_node.find_child_by_attribute("name", "eventNames");

    auto charprop_info_node = m_graph_data_node.find_child_by_attribute("name", "characterPropertyInfos");
    auto charprop_name_node = m_graph_str_data_node.find_child_by_attribute("name", "characterPropertyNames");

    if (!(m_graph_data_node && m_graph_str_data_node && m_var_value_node && var_info_node && var_name_node && evt_info_node && evt_name_node && charprop_info_node && charprop_name_node))
    {
        m_file_logger->error("File has incomplete variable/event/character property info [no valid hkbBehaviorGraphData or hkbBehaviorGraphStringData or hkbVariableValueSet].");
        glfwSetWindowTitle(g_window, g_window_title);
        dispatch(kEventLoadFile);
        return false; // Might change this in the future
    }

    // populate node list
    for (auto hkobject = m_data_node.child("hkobject"); hkobject; hkobject = hkobject.next_sibling("hkobject"))
    {
        std::string_view name = hkobject.attribute("name").as_string();
        if (name.empty())
        {
            m_file_logger->error("hkobject at location {} has no name.", hkobject.path());
            glfwSetWindowTitle(g_window, g_window_title);
            dispatch(kEventLoadFile);
            return false;
        }

        std::string_view hkclass = hkobject.attribute("class").as_string();
        if (hkclass.empty())
        {
            m_file_logger->error("hkobject at location {} has no class.", hkobject.path());
            glfwSetWindowTitle(g_window, g_window_title);
            dispatch(kEventLoadFile);
            return false;
        }

        m_node_list.push_back(hkobject);
        m_node_name_map[name] = hkobject;
        if (!m_node_class_map.contains(hkclass))
            m_node_class_map[hkclass] = NodeList();
        m_node_class_map[hkclass].push_back(hkobject);
    }

    rebuildRefList();
    m_var_manager.buildVarList(var_info_node, var_name_node, m_var_value_node);
    m_evt_manager.buildList(evt_info_node, evt_name_node);
    m_charprop_manager.buildList(charprop_info_node, charprop_name_node);

    m_file_logger->info("File successfully loaded with {} hkobjects and {} hkclasses", m_node_name_map.size(), m_node_class_map.size());
    m_loaded = true;
    glfwSetWindowTitle(g_window, std::format("{} [{}]", g_window_title, path).c_str());
    dispatch(kEventLoadFile);
    return true;
}

void HkxFile::rebuildRefList()
{
    m_node_ref_list_map.clear();

    for (auto hkobject : m_node_list)
        for (auto param = hkobject.first_child(); param; param = param.next_sibling())
            if (auto value = std::string_view(param.text().as_string()); value.starts_with('#'))
                addRef(value, hkobject);
}

#define FIND_PARAM_VAL(param, val) m_data_node.find_node([=](pugi::xml_node node) { return !strcmp(node.attribute("name").as_string(), param) && (node.text().as_int() == val); });

pugi::xml_node HkxFile::getFirstVariableRef(uint32_t idx)
{
    // bindings
    auto var_idx_node = m_data_node.find_node([=](pugi::xml_node node) { return !strcmp(node.attribute("name").as_string(), "variableIndex") &&
                                                                             (node.text().as_int() == idx) &&
                                                                             !strcmp(node.parent().find_child_by_attribute("name", "bindingType").text().as_string(), "BINDING_TYPE_VARIABLE"); });
    if (var_idx_node) return var_idx_node.parent().parent().parent();
    // hkbStateMachine
    var_idx_node = FIND_PARAM_VAL("syncVariableIndex", idx);
    if (var_idx_node) return var_idx_node.parent();
    // expressionsData
    var_idx_node = FIND_PARAM_VAL("assignmentVariableIndex", idx);
    if (var_idx_node) return var_idx_node.parent().parent().parent();
    return {};
}

pugi::xml_node HkxFile::getFirstEventRef(uint32_t idx)
{
    // Transitions
    auto evt_idx_node = FIND_PARAM_VAL("eventId", idx);
    if (evt_idx_node) return evt_idx_node.parent().parent().parent();
    evt_idx_node = FIND_PARAM_VAL("enterEventId", idx);
    if (evt_idx_node) return evt_idx_node.parent().parent().parent().parent().parent();
    evt_idx_node = FIND_PARAM_VAL("enterEventId", idx);
    if (evt_idx_node) return evt_idx_node.parent().parent().parent().parent().parent();
    // hkbClipTriggerArray
    evt_idx_node = m_data_node.find_node(
        [=](pugi::xml_node node) { 
            if(!strcmp(node.attribute("name").as_string(), "id") && (node.text().as_int() == idx))
            {
                auto parent = node.parent().parent();
                return !strcmp(node.attribute("name").as_string(), "event");
            }
            return false; });
    if (evt_idx_node) return evt_idx_node.parent().parent().parent().parent().parent();
    // hkbStateMachineEventPropertyArray
    evt_idx_node = m_data_node.find_node(
        [=](pugi::xml_node node) { 
            if(!strcmp(node.attribute("name").as_string(), "id") && (node.text().as_int() == idx))
            {
                auto parent = node.parent().parent();
                return !strcmp(node.attribute("name").as_string(), "events");
            }
            return false; });
    if (evt_idx_node) return evt_idx_node.parent().parent().parent();
    // expressionsData
    evt_idx_node = FIND_PARAM_VAL("assignmentEventIndex", idx);
    if (evt_idx_node) return evt_idx_node.parent().parent().parent();
    return {};
}
pugi::xml_node HkxFile::getFirstCharPropRef(uint32_t idx)
{
    // bindings
    auto charprop_idx_node = m_data_node.find_node([=](pugi::xml_node node) { return !strcmp(node.attribute("name").as_string(), "variableIndex") &&
                                                                                  (node.text().as_int() == idx) &&
                                                                                  !strcmp(node.parent().find_child_by_attribute("name", "bindingType").text().as_string(), "BINDING_TYPE_CHARACTER_PROPERTY"); });
    if (charprop_idx_node) return charprop_idx_node.parent().parent().parent();
    return {};
}

} // namespace Hkx

} // namespace Haviour