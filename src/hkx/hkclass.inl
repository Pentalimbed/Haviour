#pragma once
#include "utils.h"

#include <string>
#include <array>
#include <map>

#include <pugixml.hpp>

namespace Haviour
{
namespace Hkx
{
//////////////////////////    ENUMS & FLAGS
// hkbVariableInfo
enum VariableTypeEnum : int8_t
{
    VARIABLE_TYPE_INVALID = -1,
    VARIABLE_TYPE_BOOL    = 0,
    VARIABLE_TYPE_INT8,
    VARIABLE_TYPE_INT16,
    VARIABLE_TYPE_INT32,
    VARIABLE_TYPE_REAL,
    VARIABLE_TYPE_POINTER,
    VARIABLE_TYPE_VECTOR3,
    VARIABLE_TYPE_VECTOR4,
    VARIABLE_TYPE_QUATERNION,
};
constexpr auto e_variableType = std::to_array<std::string_view>(
    {
        "VARIABLE_TYPE_INVALID",
        "VARIABLE_TYPE_BOOL", // word-sized (at most)
        "VARIABLE_TYPE_INT8",
        "VARIABLE_TYPE_INT16",
        "VARIABLE_TYPE_INT32",
        "VARIABLE_TYPE_REAL",
        "VARIABLE_TYPE_POINTER", // this must be after the word-sized types and before the quad-sized types
        "VARIABLE_TYPE_VECTOR3", // quad-word sized
        "VARIABLE_TYPE_VECTOR4",
        "VARIABLE_TYPE_QUATERNION",
    });
constexpr VariableTypeEnum getVarTypeEnum(std::string_view enumstr)
{
    int i = 0;
    for (; i < e_variableType.size(); ++i)
        if (e_variableType[i] == enumstr)
            return VariableTypeEnum(i - 1);
    return VARIABLE_TYPE_INVALID;
}

#define DEFENUM(name) constexpr auto name = std::to_array<EnumWrapper>

// hkbVariableInfo -> hkbRoleAttribute
DEFENUM(e_hkbRoleAttribute_Role)
({
    {"ROLE_DEFAULT"},
    {"ROLE_FILE_NAME"},
    {"ROLE_BONE_INDEX", "The property is a bone index (hkInt16) or contains bone indices (hkbBoneIndexArray, hkbBoneWeightArray).\n"
                        "hkbBoneIndexArrays, hkbBoneWeightArrays, and individual bones can be given a group name using hk.Ui(group=\"groupName\").\n"
                        "HAT will use the Select Bones Dialog for groups of related bones."},
    {"ROLE_EVENT_ID", "The property is an event ID (hkInt32). HAT will allow you to choose the event by name."},
    {"ROLE_VARIABLE_INDEX", "The property is a behavior variable index (hkInt32). HAT will allow you to choose a variable by name."},
    {"ROLE_ATTRIBUTE_INDEX", "The property is a behavior attribute index (hkInt32). HAT will allow you to choose an attribute by name."},
    {"ROLE_TIME", "The property is a time in seconds (hkReal) (currently unused)."},
    {"ROLE_SCRIPT", "The property is a script"},
    {"ROLE_LOCAL_FRAME", "The property is a local frame"},
    {"ROLE_BONE_ATTACHMENT", "The property is a bone attachment"},
});

DEFENUM(f_hkbRoleAttribute_roleFlags)
({
    {"FLAG_NONE", {}, 0},
    {"FLAG_RAGDOLL",
     "The property refers to the ragdoll skeleton. Without this flag HAT defaults to the animation skeleton.\n"
     "Use this in conjunction with ROLE_BONE_INDEX, hkbBoneIndexArrays, or hkbBoneWeightArrays.",
     1},
    {"FLAG_NORMALIZED", "The property should be normalized (apply this to hkVector4) (currently unused).", 1 << 1},
    {"FLAG_NOT_VARIABLE", "HAT will not allow the property to be bound to a variable.", 1 << 2},
    {"FLAG_HIDDEN", "HAT will not show the property in the UI.", 1 << 3},
    {"FLAG_OUTPUT",
     "By default, all node properties are considered inputs, which means that behavior variable values are copied to them.\n"
     "If you want the property to serve as an output, you must use this flag.\n"
     "Note: a property cannot be both an input and an output.",
     1 << 4},
    {"FLAG_NOT_CHARACTER_PROPERTY", "HAT will not allow the property to be bound to a character property.", 1 << 5},
    {"FLAG_CHAIN",
     "HAT will interpret the property as contributing to a bone chain (no siblings allowed).\n"
     "Use this in conjunction with ROLE_BONE_INDEX or hkbBoneIndexArrays.",
     1 << 6},
}); // why is this so ugly

// hkbEventInfo -> Flags
DEFENUM(f_hkbEventInfo_Flags)
({
    {"FLAG_SILENT", "Whether or not clip generators should raise the event.", 0x1},
    {"FLAG_SYNC_POINT", "Whether or not the sync point will be", 0x2},
});

// hkbBehaviorGraph
DEFENUM(e_variableMode)
({
    {"VARIABLE_MODE_DISCARD_WHEN_INACTIVE", "Throw away the variable values and memory on deactivate().\n"
                                            "In this mode, variable memory is allocated and variable values are reset each time activate() is called."},
    {"VARIABLE_MODE_MAINTAIN_VALUES_WHEN_INACTIVE", "Don't discard the variable memory on deactivate(),\n"
                                                    "and don't reset the variable values on activate() (except the first time)."},
});

// hkbVariableBindingSet -> Binding
DEFENUM(e_bindingType)
(
    {
        {"BINDING_TYPE_VARIABLE"},
        {"BINDING_TYPE_CHARACTER_PROPERTY"},
    });

DEFENUM(e_hkbStateMachine_StartStateMode)
({
    {"START_STATE_MODE_DEFAULT", "Set the start state to m_startStateId."},
    {"START_STATE_MODE_SYNC", "Set the start state from the variable whose index is m_syncVariableIndex."},
    {"START_STATE_MODE_RANDOM", "Set the start state to a random state."},
    {"START_STATE_MODE_CHOOSER", "Set the start state using m_startStateIdSelector (fka. a chooser)."},
});

DEFENUM(e_hkbStateMachine_StateMachineSelfTransitionMode)
({
    {"SELF_TRANSITION_MODE_NO_TRANSITION", "Stay in the current state."},
    {"SELF_TRANSITION_MODE_TRANSITION_TO_START_STATE", "Transition to the start state if a transition exists between the current state and the start state."},
    {"SELF_TRANSITION_MODE_FORCE_TRANSITION_TO_START_STATE", "Transition to the start state using a transition if there is one, or otherwise by abruptly changing states."},
});

//////////////////////////    DEFAULT VALUES
constexpr const char* g_def_hkbVariableInfo =
    R"(<hkobject>
    <hkparam name="role">
        <hkobject>
            <hkparam name="role">ROLE_DEFAULT</hkparam>
            <hkparam name="flags">0</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="type">VARIABLE_TYPE_NONE</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbVariableValue =
    R"(<hkobject>
	<hkparam name="value">0</hkparam>
</hkobject>)";

constexpr const char* g_def_hkStringPtr = R"(<hkcstring>TEXT</hkcstring>)";

constexpr const char* g_def_hkbEventInfo =
    R"(<hkobject>
    <hkparam name="flags">0</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbVariableBindingSet =
    R"(<hkobject name="#0112" class="hkbVariableBindingSet" signature="0x338ad4ff">
    <hkparam name="bindings" numelements="0"></hkparam>
    <hkparam name="indexOfBindingToEnable">-1</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbVariableBindingSet_Binding =
    R"(<hkobject>
    <hkparam name="memberPath">memberPath</hkparam>
    <hkparam name="variableIndex">0</hkparam>
    <hkparam name="bitIndex">-1</hkparam>
    <hkparam name="bindingType">BINDING_TYPE_VARIABLE</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbStateMachine =
    R"(<hkobject name="#1255" class="hkbStateMachine" signature="0x816c1dcb">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">StateMachine</hkparam>
    <hkparam name="eventToSendWhenStateOrTransitionChanges">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="startStateChooser">null</hkparam>
    <hkparam name="startStateId">0</hkparam>
    <hkparam name="returnToPreviousStateEventId">-1</hkparam>
    <hkparam name="randomTransitionEventId">-1</hkparam>
    <hkparam name="transitionToNextHigherStateEventId">-1</hkparam>
    <hkparam name="transitionToNextLowerStateEventId">-1</hkparam>
    <hkparam name="syncVariableIndex">-1</hkparam>
    <hkparam name="wrapAroundStateId">false</hkparam>
    <hkparam name="maxSimultaneousTransitions">32</hkparam>
    <hkparam name="startStateMode">START_STATE_MODE_DEFAULT</hkparam>
    <hkparam name="selfTransitionMode">SELF_TRANSITION_MODE_NO_TRANSITION</hkparam>
    <hkparam name="states" numelements="0"></hkparam>
    <hkparam name="wildcardTransitions">null</hkparam>
</hkobject>)";

inline const std::map<std::string_view, const char*>& getClassDefaultMap()
{
#define DEFMAPITEM(hkclass)       \
    {                             \
#        hkclass, g_def_##hkclass \
    }

    static const std::map<std::string_view, const char*> map = {
        DEFMAPITEM(hkbVariableBindingSet),
        DEFMAPITEM(hkbStateMachine)};
    return map;
}
} // namespace Hkx
} // namespace Haviour