// util functions that depends on hkxfile.h
#pragma once

#include "hkxfile.h"
#include "utils.h"

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

} // namespace Haviour