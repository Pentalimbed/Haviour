// util functions that depends on hkxfile.h
#pragma once

#include "hkxfile.h"
#include "utils.h"

#include <stack>
#include <sstream>

namespace Haviour
{
inline pugi::xml_node getParentStateMachine(pugi::xml_node hkparam, Hkx::HkxFile& file)
{
    auto state_machine = getParentObj(hkparam);
    while (strcmp(state_machine.attribute("class").as_string(), "hkbStateMachine"))
    {
        std::vector<std::string> refs = {};
        file.getObjRefs(state_machine.attribute("name").as_string(), refs);
        if (refs.empty())
            break;
        state_machine = file.getObj(refs[0]);
    }
    if (!strcmp(state_machine.attribute("class").as_string(), "hkbStateMachine"))
        return state_machine;
    return {};
}

inline pugi::xml_node getStateById(pugi::xml_node state_machine, int32_t state_id, Hkx::HkxFile& file)
{
    if (strcmp(state_machine.attribute("class").as_string(), "hkbStateMachine"))
        return {};

    auto               states_node = state_machine.getByName("states");
    size_t             num_objs    = states_node.attribute("numelements").as_ullong();
    std::istringstream states_stream(states_node.text().as_string());

    for (size_t i = 0; i < num_objs; ++i)
    {
        std::string temp_str;
        states_stream >> temp_str;

        auto state = file.getObj(temp_str);
        auto id    = state.getByName("stateId").text().as_int();

        if (id == state_id)
            return state;
    }
    return {};
}

inline int getBiggestStateId(pugi::xml_node state_machine, Hkx::HkxFile& file)
{
    if (strcmp(state_machine.attribute("class").as_string(), "hkbStateMachine"))
        return -1;

    auto               states_node = state_machine.getByName("states");
    size_t             num_objs    = states_node.attribute("numelements").as_ullong();
    std::istringstream states_stream(states_node.text().as_string());
    auto               max_id = -1;

    for (size_t i = 0; i < num_objs; ++i)
    {
        std::string temp_str;
        states_stream >> temp_str;

        auto state = file.getObj(temp_str);
        auto id    = state.getByName("stateId").text().as_int();

        max_id = std::max(max_id, id);
    }
    return max_id;
}

// get a path from a child to parent
// the return starts(idx=0) from parent to child
inline void getNavPath(const std::string& from, const std::string& to, Hkx::BehaviourFile& file, std::vector<std::string>& out)
{
    StringMap<std::string>  prev_obj;
    std::stack<std::string> objs_to_check;

    // DFS
    objs_to_check.push(from);
    prev_obj[from] = {};
    while (!objs_to_check.empty())
    {
        auto top = objs_to_check.top();
        objs_to_check.pop();

        if (top == to)
        {
            out       = {to};
            auto prev = prev_obj[to];
            while (!prev.empty())
            {
                out.push_back(prev);
                prev = prev_obj[prev];
            }
        }

        std::vector<std::string> obj_refs = {};
        file.getObjRefs(top, obj_refs);
        for (auto& ref : obj_refs)
        {
            if (!prev_obj.contains(ref))
            {
                objs_to_check.push(ref);
                prev_obj[ref] = top;
            }
        }
    }
}

inline std::string hkTriggerArray2Str(pugi::xml_node trigger_array, Hkx::BehaviourFile& file)
{
    auto               triggers = trigger_array.getByName("triggers");
    std::ostringstream stream;
    for (auto trigger : triggers.children())
        if (auto id = trigger.getByName("event").first_child().getByName("id").text(); id.as_llong() >= 0)
        {
            stream << file.m_evt_manager.getEntry(id.as_ullong()).getName() << " ";
            if (trigger.getByName("acyclic").text().as_bool())
                stream << '+';
            stream << trigger.getByName("localTime").text().as_string() << std::endl;
        }
    return stream.str();
}

} // namespace Haviour