#include "hkxfile.h"

#include <GLFW/glfw3.h>

namespace Haviour
{
extern const char* g_window_title;
extern GLFWwindow* g_window;

namespace Hkx
{
bool HkxFile::loadFile(std::string_view path)
{
    m_file_logger = spdlog::default_logger()->clone(path.data());

    m_loaded    = false;
    m_data_node = m_graph_data_node = m_graph_str_data_node = m_var_value_node = {};

    m_node_list.clear();
    m_node_name_map.clear();
    m_node_class_map.clear();
    m_node_ref_list_map.clear();
    m_variable_list.clear();

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
    if (!(m_graph_data_node && m_graph_str_data_node && m_var_value_node))
    {
        m_file_logger->error("File has incomplete variable and event info [no hkbBehaviorGraphData or hkbBehaviorGraphStringData or hkbVariableValueSet].");
        glfwSetWindowTitle(g_window, g_window_title);
        dispatch(kEventLoadFile);
        return false;
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
    rebuildVariableList();

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

void HkxFile::rebuildVariableList()
{
    m_variable_list.clear();

    auto names_node  = m_graph_str_data_node.find_child_by_attribute("name", "variableNames");
    auto infos_node  = m_graph_data_node.find_child_by_attribute("name", "variableInfos");
    auto values_node = m_var_value_node.find_child_by_attribute("name", "wordVariableValues");

    m_variable_list.reserve(names_node.attribute("numelements").as_ullong());

    auto name_node  = names_node.first_child();
    auto info_node  = infos_node.first_child();
    auto value_node = values_node.first_child();
    int  idx        = 0;
    while (name_node && infos_node && values_node)
    {
        m_variable_list.push_back({name_node.text().as_string(), info_node, value_node, idx});
        name_node  = name_node.next_sibling();
        info_node  = info_node.next_sibling();
        value_node = value_node.next_sibling();
        ++idx;
    }
}
} // namespace Hkx

} // namespace Haviour