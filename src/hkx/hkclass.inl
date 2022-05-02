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
inline VariableTypeEnum getVarTypeEnum(std::string_view enumstr)
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

DEFENUM(f_hkbStateMachine_TransitionInfo_TransitionFlags)
({
    {"FLAG_USE_TRIGGER_INTERVAL", "Only allow the transition to begin if the event arrives within the interval specified by m_triggerInterval.", 0x1},
    {"FLAG_USE_INITIATE_INTERVAL", "Only allow the transition to begin within the interval specified by m_initiateInterval.", 0x2},
    {"FLAG_UNINTERRUPTIBLE_WHILE_PLAYING", "Don't allow the transition to be interrupted while it is underway.", 0x4},
    {"FLAG_UNINTERRUPTIBLE_WHILE_DELAYED", "Don't allow the transition to be interrupted while it is waiting for the initiate interval to come.", 0x8},
    {"FLAG_DELAY_STATE_CHANGE", "Change states at the end of the transition instead of the beginning.", 0x10},
    {"FLAG_DISABLED", "Disable the transition.", 0x20},
    {"FLAG_DISALLOW_RETURN_TO_PREVIOUS_STATE", "Don't use this transition to return to the previous state.", 0x40},
    {"FLAG_DISALLOW_RANDOM_TRANSITION", "Don't use this transition as a random transition.", 0x80},
    {"FLAG_DISABLE_CONDITION", "Disable the condition (effectively making it always true).", 0x100},
    {"FLAG_ALLOW_SELF_TRANSITION_BY_TRANSITION_FROM_ANY_STATE",
     "Whether or not to allow this transition to do a self-transition.\n"
     "This flag is only considered for transitions from any state (see addTransitionFromAnyState()).\n"
     "Regular transitions specify both the source and destination state, so there is no need for an additional flag.",
     0x200},
    {"FLAG_IS_GLOBAL_WILDCARD",
     "This transition is global, which means it can happen no matter what the active subgraph is.\n"
     "This flag is only considered for transitions from-any-state (wildcards).",
     0x400},
    {"FLAG_IS_LOCAL_WILDCARD",
     "If you want the transition to be used as a regular (local) wildcard, set this flag.\n"
     "This is useful if you want to have a different transition effect for a global wildcard transition than for local wildcard transition.",
     0x800},
    {"FLAG_FROM_NESTED_STATE_ID_IS_VALID", "Whether m_fromNestedStateId should be used.", 0x1000},
    {"FLAG_TO_NESTED_STATE_ID_IS_VALID", "Whether m_toNestedStateId should be used.", 0x2000},
    {"FLAG_ABUT_AT_END_OF_FROM_GENERATOR", "Whether to delay until the end of the from generator, minus the blend lead time.", 0x4000},
});

DEFENUM(e_hkbTransitionEffect_SelfTransitionMode)
({
    {"SELF_TRANSITION_MODE_CONTINUE_IF_CYCLIC_BLEND_IF_ACYCLIC",
     "If the to-generator is cyclic, it will be continued without interruption.\n"
     "Otherwise, it will be blended with itself using the echo feature."},
    {"SELF_TRANSITION_MODE_CONTINUE", "Continue the to-generator without interruption."},
    {"SELF_TRANSITION_MODE_RESET", "Reset the to-generator to the beginning."},
    {"SELF_TRANSITION_MODE_BLEND", "Reset the to-generator to the beginning using the echo feature to blend. "},
});

DEFENUM(e_hkbTransitionEffect_EventMode)
({
    {"EVENT_MODE_DEFAULT", "Apply the event mode from m_defaultEventMode."},
    {"EVENT_MODE_PROCESS_ALL", "Don't ignore the events of either the from-generator or the to-generator."},
    {"EVENT_MODE_IGNORE_FROM_GENERATOR", "Ignore all events sent by or to the from-generator."},
    {"EVENT_MODE_IGNORE_TO_GENERATOR", "Ignore all events sent by or to the to-generator."},
});

DEFENUM(f_hkbBlendingTransitionEffect_FlagBits)
({
    {"FLAG_NONE", "No flags.", 0x0},
    {"FLAG_IGNORE_FROM_WORLD_FROM_MODEL", "Just use the worldFromModel of the generator being transitioned to.", 0x1},
    {"FLAG_SYNC", "Synchronize the cycles of the children.", 0x2},
    {"FLAG_IGNORE_TO_WORLD_FROM_MODEL", "Just use the worldFromModel of the generator being transitioned from.", 0x4},
    {"FLAG_IGNORE_TO_WORLD_FROM_MODEL_ROTATION", "Blend the to and from world from models but ignore the to generator's rotation", 0x8},
});

DEFENUM(e_hkbBlendingTransitionEffect_EndMode)
({
    {"END_MODE_NONE"},
    {"END_MODE_TRANSITION_UNTIL_END_OF_FROM_GENERATOR", "Ignore m_duration, and instead transition until the end of the \"from\" generator."},
    {"END_MODE_CAP_DURATION_AT_END_OF_FROM_GENERATOR",
     "If the transition begins closer than m_duration to the end of the \"from\" generator,\n"
     "shorten the transition duration to the time remaining so that the transition does not play the \"from\" generator beyond the end."},
});

DEFENUM(e_hkbBlendCurveUtils_BlendCurve)
({
    {"BLEND_CURVE_SMOOTH", "A cubic curve that is smooth at the endpoints: f(t) = -6 t^3 + 3 t^2."},
    {"BLEND_CURVE_LINEAR", "A linear curve: f(t) = t."},
    {"BLEND_CURVE_LINEAR_TO_SMOOTH", "A cubic curve that is abrupt at the beginning and smooth at the end: f(t) = -t^3 + t^2 + t"},
    {"BLEND_CURVE_SMOOTH_TO_LINEAR", "A cubic curve that is smooth at the beginning and abrupt at the end: f(t) = -t^3 + 2 t^2"},
});

DEFENUM(f_hkbBlenderGenerator_BlenderFlags)
({
    {"FLAG_SYNC", "Adjusts the speed of the children so that their cycles align (default: false).", 0x1},
    {"FLAG_SMOOTH_GENERATOR_WEIGHTS",
     "Filter the weights using a cubic curve.\n\n"
     "If true, the generator weights are passed through a cubic filter. The new weight w is computed as w = w^2 * (-2w + 3).\n"
     "This makes it so that you can move the weights linearly as a function of time and get a smooth result.",
     0x4},
    {"FLAG_DONT_DEACTIVATE_CHILDREN_WITH_ZERO_WEIGHTS", "If true, a child will not be deactivated when its weight is zero.", 0x8},
    {"FLAG_PARAMETRIC_BLEND", "This is a parametric blend", 0x10},
    {"FLAG_IS_PARAMETRIC_BLEND_CYCLIC", "If the blend is parametric, setting this makes it cyclic parametric blend.", 0x20},
    {"FLAG_FORCE_DENSE_POSE", "Force the output pose to be dense by filling in any missing bones with the reference pose.", 0x40},
    {"FLAG_BLEND_MOTION_OF_ADDITIVE_ANIMATIONS", "Blend the motion of additive and subtractive animations.", 0x80},
    {"FLAG_USE_VELOCITY_SYNCHRONIZATION", "Use the velocity-based synchronization", 0x100},
});

DEFENUM(e_hkbClipGenerator_PlaybackMode)
({
    {"MODE_SINGLE_PLAY", "Play the clip once from start to finish."},
    {"MODE_LOOPING", "Play the clip over and over in a loop."},
    {"MODE_USER_CONTROLLED",
     "Don't advance the clip.  Let the user control the local time.\n\n"
     "In this mode the direction of movement of the clip cursor is from old local time to the new local time.\n"
     "All the triggers within this interval are processed."},
    {"MODE_PING_PONG", "At the end of the animation, turn around backward, and then turn around again at the beginning, etc."},
    {"MODE_COUNT", "How many modes there are."},
});

DEFENUM(f_hkbClipGenerator_ClipFlags)
({
    {"FLAG_CONTINUE_MOTION_AT_END",
     "In SINGLE_PLAY mode, when the clip gets to the end, it will continue the motion if this is true.\n\n"
     "If you keep playing a clip beyond the end, it will return the last pose.  This is sometimes useful as you go into a blend.\n"
     "If this property is false, the motion is just zero after the end of the clip, which means that the animation comes to a halt.\n"
     "If this property is true, the motion returned after the clip reaches the end will be the motion present at the end of the clip,\n"
     "so the animation will keep going in the direction it was going.",
     0x1},
    {"FLAG_SYNC_HALF_CYCLE_IN_PING_PONG_MODE",
     "In PING_PONG mode, if this is true, synchronization will be done on half a ping-pong cycle instead of the full cycle (back and forth).\n\n"
     "Normally in ping-pong mode, the frequency of the clip generator is reported to be half the frequency of the underlying clip.\n"
     "So for the purpose of synchronization, one cycle of this clip will include playing the clip forward and then backward again to the start.\n"
     "If you instead want it to synchronize to half of the ping-pong cycle (the animation playing through once, not twice), set this to true.",
     0x2},
    {"FLAG_MIRROR", "If this flag is set the pose is mirrored about a plane.", 0x4},
    {"FLAG_FORCE_DENSE_POSE", "If this flag is set and if the output pose would be a dense pose", 0x8},
    {"FLAG_DONT_CONVERT_ANNOTATIONS_TO_TRIGGERS", "If this flag is set then we do not convert annotation to triggers.", 0x10},
    {"FLAG_IGNORE_MOTION", "If this flag is set the motion in the animation will not be extracted.", 0x20},
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

constexpr const char* g_def_hkbEvent =
    R"(<hkobject>
    <hkparam name="id">0</hkparam>
    <hkparam name="payload">null</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbStringEventPayload =
    R"(<hkobject name="#0000" class="hkbStringEventPayload" signature="0xed04256a">
    <hkparam name="data">DLC02AnimObjectHeart</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbVariableBindingSet =
    R"(<hkobject name="#0000" class="hkbVariableBindingSet" signature="0x338ad4ff">
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
    R"(<hkobject name="#0000" class="hkbStateMachine" signature="0x816c1dcb">
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

constexpr const char* g_def_hkbStateMachineStateInfo =
    R"(<hkobject name="#0000" class="hkbStateMachineStateInfo" signature="0xed7f9d0">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="listeners" numelements="0"></hkparam>
    <hkparam name="enterNotifyEvents">null</hkparam>
    <hkparam name="exitNotifyEvents">null</hkparam>
    <hkparam name="transitions">null</hkparam>
    <hkparam name="generator">null</hkparam>
    <hkparam name="name">State</hkparam>
    <hkparam name="stateId">0</hkparam>
    <hkparam name="probability">1.000000</hkparam>
    <hkparam name="enable">true</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbStateMachine_TransitionInfo =
    R"(<hkobject>
    <hkparam name="triggerInterval">
        <hkobject>
            <hkparam name="enterEventId">-1</hkparam>
            <hkparam name="exitEventId">-1</hkparam>
            <hkparam name="enterTime">0.000000</hkparam>
            <hkparam name="exitTime">0.000000</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="initiateInterval">
        <hkobject>
            <hkparam name="enterEventId">-1</hkparam>
            <hkparam name="exitEventId">-1</hkparam>
            <hkparam name="enterTime">0.000000</hkparam>
            <hkparam name="exitTime">0.000000</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="transition">null</hkparam>
    <hkparam name="condition">null</hkparam>
    <hkparam name="eventId">0</hkparam>
    <hkparam name="toStateId">0</hkparam>
    <hkparam name="fromNestedStateId">0</hkparam>
    <hkparam name="toNestedStateId">0</hkparam>
    <hkparam name="priority">0</hkparam>
    <hkparam name="flags">0</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbStateMachineTransitionInfoArray =
    R"(<hkobject name="#0000" class="hkbStateMachineTransitionInfoArray" signature="0xe397b11e">
    <hkparam name="transitions" numelements="0"></hkparam>
</hkobject>)";

constexpr const char* g_def_hkbBlendingTransitionEffect =
    R"(<hkobject name="#0000" class="hkbBlendingTransitionEffect" signature="0xfd8584fe">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">BlendTransition</hkparam>
    <hkparam name="selfTransitionMode">SELF_TRANSITION_MODE_BLEND</hkparam>
    <hkparam name="eventMode">EVENT_MODE_DEFAULT</hkparam>
    <hkparam name="duration">0.000000</hkparam>
    <hkparam name="toGeneratorStartTimeFraction">0.000000</hkparam>
    <hkparam name="flags">0</hkparam>
    <hkparam name="endMode">END_MODE_NONE</hkparam>
    <hkparam name="blendCurve">BLEND_CURVE_SMOOTH</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbExpressionCondition =
    R"(<hkobject name="#0000" class="hkbExpressionCondition" signature="0x1c3c1045">
    <hkparam name="expression"></hkparam>
</hkobject>)";

constexpr const char* g_def_hkbBlenderGenerator =
    R"(<hkobject name="#0000" class="hkbBlenderGenerator" signature="0x22df7147">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">BlenderGenerator</hkparam>
    <hkparam name="referencePoseWeightThreshold">0.000000</hkparam>
    <hkparam name="blendParameter">0.000000</hkparam>
    <hkparam name="minCyclicBlendParameter">0.000000</hkparam>
    <hkparam name="maxCyclicBlendParameter">1.000000</hkparam>
    <hkparam name="indexOfSyncMasterChild">0</hkparam>
    <hkparam name="flags">0</hkparam>
    <hkparam name="subtractLastChild">false</hkparam>
    <hkparam name="children" numelements="0"></hkparam>
</hkobject>)";

constexpr const char* g_def_hkbBlenderGeneratorChild =
    R"(<hkobject name="#0000" class="hkbBlenderGeneratorChild" signature="0xe2b384b0">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="generator">null</hkparam>
    <hkparam name="boneWeights">null</hkparam>
    <hkparam name="weight">0.000000</hkparam>
    <hkparam name="worldFromModelWeight">1.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_BSBoneSwitchGenerator =
    R"(<hkobject name="#0602" class="BSBoneSwitchGenerator" signature="0xf33d3eea">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">BoneSwitch</hkparam>
    <hkparam name="pDefaultGenerator">null</hkparam>
    <hkparam name="ChildrenA" numelements="0"></hkparam>
</hkobject>)";

constexpr const char* g_def_BSBoneSwitchGeneratorBoneData =
    R"(<hkobject name="#0000" class="BSBoneSwitchGeneratorBoneData" signature="0xc1215be6">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="pGenerator">null</hkparam>
    <hkparam name="spBoneWeight">null</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbBoneWeightArray =
    R"(<hkobject name="#0000" class="hkbBoneWeightArray" signature="0xcd902b77">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="boneWeights" numelements="0"></hkparam>
</hkobject>)";

constexpr const char* g_def_hkbClipGenerator =
    R"(<hkobject name="#0000" class="hkbClipGenerator" signature="0x333b85b9">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">ClipGenerator</hkparam>
    <hkparam name="animationName">animation.hkx</hkparam>
    <hkparam name="triggers">null</hkparam>
    <hkparam name="cropStartAmountLocalTime">0.000000</hkparam>
    <hkparam name="cropEndAmountLocalTime">0.000000</hkparam>
    <hkparam name="startTime">0.000000</hkparam>
    <hkparam name="playbackSpeed">1.000000</hkparam>
    <hkparam name="enforcedDuration">0.000000</hkparam>
    <hkparam name="userControlledTimeFraction">0.000000</hkparam>
    <hkparam name="animationBindingIndex">-1</hkparam>
    <hkparam name="mode">MODE_SINGLE_PLAY</hkparam>
    <hkparam name="flags">0</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbClipTrigger =
    R"(<hkobject>
    <hkparam name="localTime">0.000000</hkparam>
    <hkparam name="event">
        <hkobject>
            <hkparam name="id">0</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="relativeToEndOfClip">false</hkparam>
    <hkparam name="acyclic">false</hkparam>
    <hkparam name="isAnnotation">false</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbClipTriggerArray =
    R"(<hkobject name="#0000" class="hkbClipTriggerArray" signature="0x59c23a0f">
    <hkparam name="triggers" numelements="0"></hkparam>
</hkobject>)";

constexpr const char* g_def_BSSynchronizedClipGenerator =
    R"(<hkobject name="#0365" class="BSSynchronizedClipGenerator" signature="0xd83bea64">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">SynchronizedClipGenerator</hkparam>
    <hkparam name="pClipGenerator">null</hkparam>
    <hkparam name="SyncAnimPrefix">&#9216;</hkparam>
    <hkparam name="bSyncClipIgnoreMarkPlacement">false</hkparam>
    <hkparam name="fGetToMarkTime">0.000000</hkparam>
    <hkparam name="fMarkErrorThreshold">0.100000</hkparam>
    <hkparam name="bLeadCharacter">false</hkparam>
    <hkparam name="bReorientSupportChar">false</hkparam>
    <hkparam name="bApplyMotionFromRoot">false</hkparam>
    <hkparam name="sAnimationBindingIndex">-1</hkparam>
</hkobject>)";

inline const std::map<std::string_view, const char*>& getClassDefaultMap()
{
#define DEFMAPITEM(hkclass)       \
    {                             \
#        hkclass, g_def_##hkclass \
    }

    static const std::map<std::string_view, const char*> map = {
        DEFMAPITEM(hkbVariableBindingSet),
        DEFMAPITEM(hkbStateMachine),
        DEFMAPITEM(hkbStateMachineStateInfo),
        DEFMAPITEM(hkbStringEventPayload),
        DEFMAPITEM(hkbStateMachineTransitionInfoArray),
        DEFMAPITEM(hkbBlendingTransitionEffect),
        DEFMAPITEM(hkbExpressionCondition),
        DEFMAPITEM(hkbBlenderGenerator),
        DEFMAPITEM(hkbBlenderGeneratorChild),
        DEFMAPITEM(BSBoneSwitchGenerator),
        DEFMAPITEM(hkbBoneWeightArray),
        DEFMAPITEM(BSBoneSwitchGeneratorBoneData),
        DEFMAPITEM(hkbClipGenerator),
        DEFMAPITEM(hkbClipTriggerArray),
        DEFMAPITEM(BSSynchronizedClipGenerator)};
    return map;
}

//////////////////////////    CLASS OF CLASSES

constexpr auto g_class_generators = std::to_array<std::string_view>(
    {"hkbStateMachine",
     "hkbManualSelectorGenerator",
     "hkbModifierGenerator",
     "BSiStateTaggingGenerator",
     "BSSynchronizedClipGenerator",
     "BSOffsetAnimationGenerator",
     "BSCyclicBlendTransitionGenerator",
     "hkbPoseMatchingGenerator",
     "hkbClipGenerator",
     "hkbBehaviorReferenceGenerator",
     "hkbBlenderGenerator",
     "BSBoneSwitchGenerator"});

} // namespace Hkx
} // namespace Haviour