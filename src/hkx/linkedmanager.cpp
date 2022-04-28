// just to avoid including hkclass.inl everywhere

#include "linkedmanager.h"
#include "hkclass.inl"

#include <sstream>
#include <fmt/format.h>

namespace Haviour
{
namespace Hkx
{
#define LINKEDPROPDEF(class_name, def_prop_str) \
    const char* class_name::getDefaultValue() { return def_prop_str; }

LINKEDPROPDEF(PropName, g_def_hkStringPtr);
LINKEDPROPDEF(PropWordValue, g_def_hkbVariableValue);
LINKEDPROPDEF(PropVarInfo, g_def_hkbVariableInfo);
/// why can't linker detect this?
LINKEDPROPDEF(PropEventInfo, g_def_hkbEventInfo);

void VariableManager::buildEntryList(pugi::xml_node name_node, pugi::xml_node info_node, pugi::xml_node value_node, pugi::xml_node quad_node)
{
    LinkedPropertyManager<Variable>::buildEntryList({name_node, info_node, value_node});
    // get quads
    m_quad_node            = quad_node;
    auto        quad_count = quad_node.attribute("numelements").as_uint();
    std::string quad_vals  = quad_node.text().as_string();
    std::erase_if(quad_vals, [](char& a) { return (a == '(') || (a == ')'); });
    std::istringstream quad_stream(quad_vals);
    for (size_t i = 0; i < quad_count; i++)
    {
        std::array<float, 4> array = {};
        quad_stream >> array[0] >> array[1] >> array[2] >> array[3];
        m_quads.push_back(array);
    }
}

Variable VariableManager::addEntry(VariableTypeEnum data_type)
{
    auto retval = addEntry();

    retval.get<PropVarInfo>().getByName("type").text() = e_variableType[data_type + 1].data();

    switch (data_type)
    {
        // case VARIABLE_TYPE_POINTER:
        //     // NOT IMPLEMENTED
        //     break;
        case VARIABLE_TYPE_VECTOR3:
        case VARIABLE_TYPE_VECTOR4:
        case VARIABLE_TYPE_QUATERNION:
            retval.get<PropWordValue>().first_child().text() = m_quads.size();
            m_quads.push_back({0, 0, 0, 0});
            break;
        default:
            break;
    }

    return retval;
}

robin_hood::unordered_map<size_t, size_t> VariableManager::reindex()
{
    auto remap = LinkedPropertyManager<Variable>::reindex();

    std::vector<std::vector<Variable*>> quad_users(m_quads.size());
    for (auto& entry : m_entries)
    {
        auto data_type = getVarTypeEnum(entry.get<PropVarInfo>().getByName("type").text().as_string());
        switch (data_type)
        {
            // case VARIABLE_TYPE_POINTER:
            //     // NOT IMPLEMENTED
            //     break;
            case VARIABLE_TYPE_VECTOR3:
            case VARIABLE_TYPE_VECTOR4:
            case VARIABLE_TYPE_QUATERNION:
                quad_users[entry.get<PropWordValue>().first_child().text().as_ullong()].push_back(&entry);
                break;
            default:
                break;
        }
    }
    // Clean quads
    std::vector<std::array<float, 4>> new_quads;
    std::string                       quad_text = {};
    for (size_t i = 0; i < m_quads.size(); ++i)
    {
        if (!quad_users[i].empty())
        {
            new_quads.push_back(m_quads[i]);
            quad_text += fmt::format("\n({},{},{},{})", m_quads[i][0], m_quads[i][1], m_quads[i][2], m_quads[i][3]);
        }
        for (auto user : quad_users[i])
            user->get<PropWordValue>().first_child().text() = new_quads.size() - 1;
    }
    m_quads = new_quads;
    // Rewrite xml
    m_quad_node.remove_children();
    m_quad_node.attribute("numelements") = m_quads.size();
    m_quad_node.text()                   = quad_text.c_str();
    return remap;
}
} // namespace Hkx
} // namespace Haviour