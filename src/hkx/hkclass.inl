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

DEFENUM(e_hkbPoseMatchingGenerator_Mode)
({
    {"MODE_MATCH", "Perform pose matching. Generate the currently matched pose."},
    {"MODE_PLAY", "Play back the current animation beginning at the matched pose."},
});

DEFENUM(e_hkbExpressionData_ExpressionEventMode)
({
    {"EVENT_MODE_SEND_ONCE", "Send the event once the first time that the expression is true."},
    {"EVENT_MODE_SEND_ON_TRUE", "Send the event every frame if the expression is true."},
    {"EVENT_MODE_SEND_ON_FALSE_TO_TRUE", "Send the event every frame in which the expression becomes true after having been false on the previous frame."},
    {"EVENT_MODE_SEND_EVERY_FRAME_ONCE_TRUE", "Send the event every frame after it first becomes true."},
});

DEFENUM(e_hkbEventRangeData_EventRangeMode)
({
    {"EVENT_MODE_SEND_ON_ENTER_RANGE", "Send the event every frame in which the range is entered after having been outside the range on the previous frame."},
    {"EVENT_MODE_SEND_WHEN_IN_RANGE", "Send the event every frame if the value is in the range."},
});

DEFENUM(e_hkbWorldFromModelModeData_WorldFromModelMode)
({
    {"WORLD_FROM_MODEL_MODE_USE_OLD",
     "Return the previous worldFromModel of the character, ignoring the incoming worldFromModel.\n"
     "This causes the character's position and orientation to remain fixed while in ragdoll mode."},
    {"WORLD_FROM_MODEL_MODE_USE_INPUT",
     "Return the worldFromModel that accompanies the input pose.\n"
     "This usually only makes sense if you are keyframing some of the bones, because then the ragdoll tends to follow the motion in the animation."},
    {"WORLD_FROM_MODEL_MODE_COMPUTE",
     "Compute the worldFromModel by matching the animation to the ragdoll."},
    {"WORLD_FROM_MODEL_MODE_NONE",
     "Used to indicate that the mode should not be set."},
});

DEFENUM(e_hkbTwistModifier_SetAngleMethod)
({
    {"LINEAR"},
    {"RAMPED"},
});

DEFENUM(e_hkbTwistModifier_RotationAxisCoordinates)
({
    {"ROTATION_AXIS_IN_MODEL_COORDINATES", "Indicates that m_axisOfRotation is in model space."},
    {"ROTATION_AXIS_IN_PARENT_COORDINATES", "Indicates that m_axisOfRotation is in local parent space."},
    {"ROTATION_AXIS_IN_LOCAL_COORDINATES", "Indicates that m_axisOfRotation is in local space."},
});

//////////////////////////    DEFAULT VALUES


constexpr const char* g_def_hkx =
    R"(<?xml version="1.0" encoding="ascii"?>
<hkpackfile classversion="8" contentsversion="hk_2010.2.0-r1" toplevelobject="#0108">

	<hksection name="__data__">

		<hkobject name="#0108" class="hkRootLevelContainer" signature="0x2772c11e">
			<hkparam name="namedVariants" numelements="1">
				<hkobject>
					<hkparam name="name">hkbBehaviorGraph</hkparam>
					<hkparam name="className">hkbBehaviorGraph</hkparam>
					<hkparam name="variant">#0105</hkparam>
				</hkobject>
			</hkparam>
		</hkobject>

		<hkobject name="#0105" class="hkbBehaviorGraph" signature="0xb1218f86">
			<hkparam name="variableBindingSet">null</hkparam>
			<hkparam name="userData">0</hkparam>
			<hkparam name="name">NEW_BEHAVIOR</hkparam>
			<hkparam name="variableMode">VARIABLE_MODE_DISCARD_WHEN_INACTIVE</hkparam>
			<hkparam name="rootGenerator">null</hkparam>
			<hkparam name="data">#0111</hkparam>
		</hkobject>

		<hkobject name="#0111" class="hkbBehaviorGraphData" signature="0x95aca5d">
			<hkparam name="attributeDefaults" numelements="0"></hkparam>
			<hkparam name="variableInfos" numelements="0"></hkparam>
			<hkparam name="characterPropertyInfos" numelements="0"></hkparam>
			<hkparam name="eventInfos" numelements="0"></hkparam>
			<hkparam name="variableInitialValues">#0110</hkparam>
			<hkparam name="stringData">#0109</hkparam>
		</hkobject>

		<hkobject name="#0110" class="hkbVariableValueSet" signature="0x27812d8d">
			<hkparam name="wordVariableValues" numelements="0"></hkparam>
			<hkparam name="quadVariableValues" numelements="0"></hkparam>
			<hkparam name="variantVariableValues" numelements="0"></hkparam>
		</hkobject>

		<hkobject name="#0109" class="hkbBehaviorGraphStringData" signature="0xc713064e">
			<hkparam name="eventNames" numelements="0"></hkparam>
			<hkparam name="attributeNames" numelements="0"></hkparam>
			<hkparam name="variableNames" numelements="0"></hkparam>
			<hkparam name="characterPropertyNames" numelements="0"></hkparam>
		</hkobject>

	</hksection>

</hkpackfile>)";


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
    <hkparam name="id">-1</hkparam>
    <hkparam name="payload">null</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbStringEventPayload =
    R"(<hkobject name="#0000" class="hkbStringEventPayload" signature="0xed04256a">
    <hkparam name="data">payload</hkparam>
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

constexpr const char* g_def_hkbBoneIndexArray =
    R"(<hkobject name="#0000" class="hkbBoneIndexArray" signature="0xaa8619">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="boneIndices" numelements="0"></hkparam>
</hkobject>)";

constexpr const char* g_def_hkbClipGenerator =
    R"(<hkobject name="#0000" class="hkbClipGenerator" signature="0x333b85b9">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">ClipGenerator</hkparam>
    <hkparam name="animationName">Animations\animation.hkx</hkparam>
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
            <hkparam name="id">-1</hkparam>
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
    R"(<hkobject name="#0000" class="BSSynchronizedClipGenerator" signature="0xd83bea64">
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

constexpr const char* g_def_hkbManualSelectorGenerator =
    R"(<hkobject name="#0000" class="hkbManualSelectorGenerator" signature="0xd932fab8">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">SelectorGenerator</hkparam>
    <hkparam name="generators" numelements="0"></hkparam>
    <hkparam name="selectedGeneratorIndex">0</hkparam>
    <hkparam name="currentGeneratorIndex">0</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbModifierGenerator =
    R"(<hkobject name="#0000" class="hkbModifierGenerator" signature="0x1f81fae6">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">ModifierGenerator</hkparam>
    <hkparam name="modifier">null</hkparam>
    <hkparam name="generator">null</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbBehaviorReferenceGenerator =
    R"(<hkobject name="#0000" class="hkbBehaviorReferenceGenerator" signature="0xfcb5423">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">BehaviorReference</hkparam>
    <hkparam name="behaviorName">Behaviors\Some_Behavior.hkx</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbPoseMatchingGenerator =
    R"(<hkobject name="#0000" class="hkbPoseMatchingGenerator" signature="0x29e271b4">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">PoseMatchingGenerator</hkparam>
    <hkparam name="referencePoseWeightThreshold">0.000000</hkparam>
    <hkparam name="blendParameter">0.000000</hkparam>
    <hkparam name="minCyclicBlendParameter">0.000000</hkparam>
    <hkparam name="maxCyclicBlendParameter">1.000000</hkparam>
    <hkparam name="indexOfSyncMasterChild">0</hkparam>
    <hkparam name="flags">0</hkparam>
    <hkparam name="subtractLastChild">false</hkparam>
    <hkparam name="children" numelements="0"></hkparam>
    <hkparam name="worldFromModelRotation">(0.000000 0.000000 0.000000 1.000000)</hkparam>
    <hkparam name="blendSpeed">1.000000</hkparam>
    <hkparam name="minSpeedToSwitch">0.200000</hkparam>
    <hkparam name="minSwitchTimeNoError">0.200000</hkparam>
    <hkparam name="minSwitchTimeFullError">0.000000</hkparam>
    <hkparam name="startPlayingEventId">-1</hkparam>
    <hkparam name="startMatchingEventId">-1</hkparam>
    <hkparam name="rootBoneIndex">-1</hkparam>
    <hkparam name="otherBoneIndex">-1</hkparam>
    <hkparam name="anotherBoneIndex">-1</hkparam>
    <hkparam name="pelvisIndex">-1</hkparam>
    <hkparam name="mode">MODE_MATCH</hkparam>
</hkobject>)";

constexpr const char* g_def_BSCyclicBlendTransitionGenerator =
    R"(<hkobject name="#0000" class="BSCyclicBlendTransitionGenerator" signature="0x5119eb06">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">CyclicBlendTransitionGenerator</hkparam>
    <hkparam name="pBlenderGenerator">null</hkparam>
    <hkparam name="EventToFreezeBlendValue">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="EventToCrossBlend">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="fBlendParameter">0.000000</hkparam>
    <hkparam name="fTransitionDuration">0.200000</hkparam>
    <hkparam name="eBlendCurve">BLEND_CURVE_SMOOTH</hkparam>
</hkobject>)";

constexpr const char* g_def_BSiStateTaggingGenerator =
    R"(<hkobject name="#0000" class="BSiStateTaggingGenerator" signature="0xf0826fc1">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">iStateGen</hkparam>
    <hkparam name="pDefaultGenerator">null</hkparam>
    <hkparam name="iStateToSetAs">0</hkparam>
    <hkparam name="iPriority">0</hkparam>
</hkobject>)";

constexpr const char* g_def_BSOffsetAnimationGenerator =
    R"(<hkobject name="#0000" class="BSOffsetAnimationGenerator" signature="0xb8571122">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">OffsetAnimationGen</hkparam>
    <hkparam name="pDefaultGenerator">null</hkparam>
    <hkparam name="pOffsetClipGenerator">null</hkparam>
    <hkparam name="fOffsetVariable">0.000000</hkparam>
    <hkparam name="fOffsetRangeStart">0.000000</hkparam>
    <hkparam name="fOffsetRangeEnd">1.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbModifierList =
    R"(<hkobject name="#0000" class="hkbModifierList" signature="0xa4180ca1">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">ModifierList</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="modifiers" numelements="0"></hkparam>
</hkobject>)";

constexpr const char* g_def_hkbExpressionData =
    R"(<hkobject>
    <hkparam name="expression">expression</hkparam>
    <hkparam name="assignmentVariableIndex">-1</hkparam>
    <hkparam name="assignmentEventIndex">-1</hkparam>
    <hkparam name="eventMode">EVENT_MODE_SEND_ONCE</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbExpressionDataArray =
    R"(<hkobject name="#0000" class="hkbExpressionDataArray" signature="0x4b9ee1a2">
    <hkparam name="expressionsData" numelements="0"></hkparam>
</hkobject>)";

constexpr const char* g_def_hkbEvaluateExpressionModifier =
    R"(<hkobject name="#0000" class="hkbEvaluateExpressionModifier" signature="0xf900f6be">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">ExpressionModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="expressions">null</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbEventDrivenModifier =
    R"(<hkobject name="#0000" class="hkbEventDrivenModifier" signature="0x7ed3f44e">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">EventDrivenModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="modifier">null</hkparam>
    <hkparam name="activateEventId">-1</hkparam>
    <hkparam name="deactivateEventId">-1</hkparam>
    <hkparam name="activeByDefault">false</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbEventsFromRangeModifier =
    R"(<hkobject name="#0000" class="hkbEventsFromRangeModifier" signature="0xbc561b6e">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">EventsFromRangeMod</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="inputValue">0.000000</hkparam>
    <hkparam name="lowerBound">0.000000</hkparam>
    <hkparam name="eventRanges">nullptr</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbEventRangeData =
    R"(<hkobject>
    <hkparam name="upperBound">0.000000</hkparam>
    <hkparam name="event">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="eventMode">EVENT_MODE_SEND_ON_ENTER_RANGE</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbEventRangeDataArray =
    R"(<hkobject name="#0000" class="hkbEventRangeDataArray" signature="0x330a56ee">
    <hkparam name="eventData" numelements="0"></hkparam>
</hkobject>)";

constexpr const char* g_def_hkbGetUpModifier =
    R"(<hkobject name="#0000" class="hkbGetUpModifier" signature="0x61cb7ac0">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">GetUpMod</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="groundNormal">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="duration">1.000000</hkparam>
    <hkparam name="alignWithGroundDuration">0.000000</hkparam>
    <hkparam name="rootBoneIndex">-1</hkparam>
    <hkparam name="otherBoneIndex">-1</hkparam>
    <hkparam name="anotherBoneIndex">-1</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbKeyframeBonesModifier =
    R"(<hkobject name="#0000" class="hkbKeyframeBonesModifier" signature="0x95f66629">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">KeyframeBonesMod</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="keyframeInfo" numelements="0"></hkparam>
    <hkparam name="keyframedBonesList">null</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbPoweredRagdollControlsModifier =
    R"(<hkobject name="#0241" class="hkbPoweredRagdollControlsModifier" signature="0x7cb54065">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">1</hkparam>
    <hkparam name="name">PoweredRagdollMatching</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="controlData">
        <hkobject>
            <hkparam name="maxForce">50.000000</hkparam>
            <hkparam name="tau">0.800000</hkparam>
            <hkparam name="damping">1.000000</hkparam>
            <hkparam name="proportionalRecoveryVelocity">2.000000</hkparam>
            <hkparam name="constantRecoveryVelocity">1.000000</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="bones">null</hkparam>
    <hkparam name="worldFromModelModeData">
        <hkobject>
            <hkparam name="poseMatchingBone0">-1</hkparam>
            <hkparam name="poseMatchingBone1">-1</hkparam>
            <hkparam name="poseMatchingBone2">-1</hkparam>
            <hkparam name="mode">WORLD_FROM_MODEL_MODE_NONE</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="boneWeights">null</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbRigidBodyRagdollControlsModifier =
    R"(<hkobject name="#0000" class="hkbRigidBodyRagdollControlsModifier" signature="0xaa87d1eb">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">1</hkparam>
    <hkparam name="name">RBMod</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="controlData">
        <hkobject>
            <hkparam name="keyFrameHierarchyControlData">
                <hkobject>
                    <hkparam name="hierarchyGain">0.170000</hkparam>
                    <hkparam name="velocityDamping">0.000000</hkparam>
                    <hkparam name="accelerationGain">1.000000</hkparam>
                    <hkparam name="velocityGain">0.600000</hkparam>
                    <hkparam name="positionGain">0.050000</hkparam>
                    <hkparam name="positionMaxLinearVelocity">1.400000</hkparam>
                    <hkparam name="positionMaxAngularVelocity">1.800000</hkparam>
                    <hkparam name="snapGain">0.100000</hkparam>
                    <hkparam name="snapMaxLinearVelocity">0.300000</hkparam>
                    <hkparam name="snapMaxAngularVelocity">0.300000</hkparam>
                    <hkparam name="snapMaxLinearDistance">0.030000</hkparam>
                    <hkparam name="snapMaxAngularDistance">0.100000</hkparam>
                </hkobject>
            </hkparam>
            <hkparam name="durationToBlend">0.000000</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="bones">null</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbRotateCharacterModifier =
    R"(<hkobject name="#0000" class="hkbRotateCharacterModifier" signature="0x877ebc0b">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name"RotateModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="degreesPerSecond">1.000000</hkparam>
    <hkparam name="speedMultiplier">1.000000</hkparam>
    <hkparam name="axisOfRotation">(1.000000 1.000000 0.000000 0.000000)</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbTimerModifier =
    R"(<hkobject name="#0000" class="hkbTimerModifier" signature="0x338b4879">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">TimerMod</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="alarmTimeSeconds">0.000000</hkparam>
    <hkparam name="alarmEvent">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
</hkobject>)";

constexpr const char* g_def_hkbFootIkControlsModifier =
    R"(<hkobject name="#0000" class="hkbFootIkControlsModifier" signature="0xe5b6f544">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">FootIKControlsModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="controlData">
        <hkobject>
            <hkparam name="gains">
                <hkobject>
                    <hkparam name="onOffGain">0.200000</hkparam>
                    <hkparam name="groundAscendingGain">1.000000</hkparam>
                    <hkparam name="groundDescendingGain">1.000000</hkparam>
                    <hkparam name="footPlantedGain">1.000000</hkparam>
                    <hkparam name="footRaisedGain">1.000000</hkparam>
                    <hkparam name="footUnlockGain">1.000000</hkparam>
                    <hkparam name="worldFromModelFeedbackGain">0.000000</hkparam>
                    <hkparam name="errorUpDownBias">1.000000</hkparam>
                    <hkparam name="alignWorldFromModelGain">0.000000</hkparam>
                    <hkparam name="hipOrientationGain">0.000000</hkparam>
                    <hkparam name="maxKneeAngleDifference">0.000000</hkparam>
                    <hkparam name="ankleOrientationGain">0.250000</hkparam>
                </hkobject>
            </hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="legs" numelements="0"></hkparam>
    <hkparam name="errorOutTranslation">(-1.000000 0.000000 1.000000 0.000000)</hkparam>
    <hkparam name="alignWithGroundRotation">(0.000000 0.000000 0.000000 1.000000)</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbFootIkControlsModifier_Leg =
    R"(<hkobject>
    <hkparam name="groundPosition">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="ungroundedEvent">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="verticalError">0.000000</hkparam>
    <hkparam name="hitSomething">false</hkparam>
    <hkparam name="isPlantedMS">false</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbTwistModifier =
    R"(<hkobject name="#0000" class="hkbTwistModifier" signature="0xb6b76b32">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">TwistModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="axisOfRotation">(1.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="twistAngle">0.000000</hkparam>
    <hkparam name="startBoneIndex">-1</hkparam>
    <hkparam name="endBoneIndex">-1</hkparam>
    <hkparam name="setAngleMethod">LINEAR</hkparam>
    <hkparam name="rotationAxisCoordinates">ROTATION_AXIS_IN_MODEL_COORDINATES</hkparam>
    <hkparam name="isAdditive">true</hkparam>
</hkobject>)";

constexpr const char* g_def_BSDirectAtModifier =
    R"(<hkobject name="#0000" class="BSDirectAtModifier" signature="0x19a005c0">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">BSDirectAtModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="directAtTarget">true</hkparam>
    <hkparam name="sourceBoneIndex">-1</hkparam>
    <hkparam name="startBoneIndex">-1</hkparam>
    <hkparam name="endBoneIndex">-1</hkparam>
    <hkparam name="limitHeadingDegrees">90.000000</hkparam>
    <hkparam name="limitPitchDegrees">90.000000</hkparam>
    <hkparam name="offsetHeadingDegrees">0.000000</hkparam>
    <hkparam name="offsetPitchDegrees">0.000000</hkparam>
    <hkparam name="onGain">0.050000</hkparam>
    <hkparam name="offGain">0.050000</hkparam>
    <hkparam name="targetLocation">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="userInfo">3</hkparam>
    <hkparam name="directAtCamera">false</hkparam>
    <hkparam name="directAtCameraX">0.000000</hkparam>
    <hkparam name="directAtCameraY">0.000000</hkparam>
    <hkparam name="directAtCameraZ">0.000000</hkparam>
    <hkparam name="active">false</hkparam>
    <hkparam name="currentHeadingOffset">0.000000</hkparam>
    <hkparam name="currentPitchOffset">0.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_BSEventEveryNEventsModifier =
    R"(<hkobject name="#0000" class="BSEventEveryNEventsModifier" signature="0x6030970c">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">BSEventEveryNEventsModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="eventToCheckFor">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="eventToSend">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="numberOfEventsBeforeSend">5</hkparam>
    <hkparam name="minimumNumberOfEventsBeforeSend">2</hkparam>
    <hkparam name="randomizeNumberOfEvents">false</hkparam>
</hkobject>)";

constexpr const char* g_def_BSEventOnDeactivateModifier =
    R"(<hkobject name="#0000" class="BSEventOnDeactivateModifier" signature="0x1062d993">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">OnDeactivateMod</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="event">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
</hkobject>)";

constexpr const char* g_def_BSEventOnFalseToTrueModifier =
    R"(<hkobject name="#0000" class="BSEventOnFalseToTrueModifier" signature="0x81d0777a">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">EventOnFalseToTrueMod</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="bEnableEvent1">false</hkparam>
    <hkparam name="bVariableToTest1">false</hkparam>
    <hkparam name="EventToSend1">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="bEnableEvent2">false</hkparam>
    <hkparam name="bVariableToTest2">false</hkparam>
    <hkparam name="EventToSend2">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="bEnableEvent3">false</hkparam>
    <hkparam name="bVariableToTest3">false</hkparam>
    <hkparam name="EventToSend3">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
</hkobject>)";

constexpr const char* g_def_hkbCombineTransformsModifier =
    R"(<hkobject name="#0000" class="hkbCombineTransformsModifier" signature="0xfd1f0b79">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">CombineTransformsModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="translationOut">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="rotationOut">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="leftTranslation">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="leftRotation">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="rightTranslation">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="rightRotation">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="invertLeftTransform">false</hkparam>
    <hkparam name="invertRightTransform">false</hkparam>
    <hkparam name="invertResult">false</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbComputeDirectionModifier =
    R"(<hkobject name="#0000" class="hkbComputeDirectionModifier" signature="0xdf358bd3">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">ComputeDirectionModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="pointIn">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="pointOut">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="groundAngleOut">0.000000</hkparam>
    <hkparam name="upAngleOut">0.000000</hkparam>
    <hkparam name="verticalOffset">0.000000</hkparam>
    <hkparam name="reverseGroundAngle">false</hkparam>
    <hkparam name="reverseUpAngle">false</hkparam>
    <hkparam name="projectPoint">false</hkparam>
    <hkparam name="normalizePoint">false</hkparam>
    <hkparam name="computeOnlyOnce">false</hkparam>
    <hkparam name="computedOutput">false</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbComputeRotationFromAxisAngleModifier =
    R"(<hkobject name="#0000" class="hkbComputeRotationFromAxisAngleModifier" signature="0x9b3f6936">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">ComputeRotationFromAxisAngleModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="rotationOut">(0.000000 0.000000 0.000000 1.000000)</hkparam>
    <hkparam name="axis">(1.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="angleDegrees">0.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbComputeRotationToTargetModifier =
    R"(<hkobject name="#0000" class="hkbComputeRotationToTargetModifier" signature="0x47665f1c">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">ComputeRotationToTargetModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="rotationOut">(0.000000 0.000000 0.000000 1.000000)</hkparam>
    <hkparam name="targetPosition">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="currentPosition">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="currentRotation">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="localAxisOfRotation">(0.000000 0.000000 1.000000 0.000000)</hkparam>
    <hkparam name="localFacingDirection">(1.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="resultIsDelta">false</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbDampingModifier =
    R"(<hkobject name="#0000" class="hkbDampingModifier" signature="0x9a040f03">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">1</hkparam>
    <hkparam name="name">DampingModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="kP">0.200000</hkparam>
    <hkparam name="kI">0.015000</hkparam>
    <hkparam name="kD">-0.100000</hkparam>
    <hkparam name="enableScalarDamping">true</hkparam>
    <hkparam name="enableVectorDamping">false</hkparam>
    <hkparam name="rawValue">0.000000</hkparam>
    <hkparam name="dampedValue">0.000000</hkparam>
    <hkparam name="rawVector">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="dampedVector">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="vecErrorSum">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="vecPreviousError">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="errorSum">0.000000</hkparam>
    <hkparam name="previousError">0.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbDelayedModifier =
    R"(<hkobject name="#0000" class="hkbDelayedModifier" signature="0x8e101a7a">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">DelayedModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="modifier">null</hkparam>
    <hkparam name="delaySeconds">0.000000</hkparam>
    <hkparam name="durationSeconds">0.000000</hkparam>
    <hkparam name="secondsElapsed">0.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbDetectCloseToGroundModifier =
    R"(<hkobject name="#0000" class="hkbDetectCloseToGroundModifier" signature="0x981687b2">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">DetectCloseToGroundModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="closeToGroundEvent">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="closeToGroundHeight">0.500000</hkparam>
    <hkparam name="raycastDistanceDown">0.800000</hkparam>
    <hkparam name="collisionFilterInfo">3</hkparam>
    <hkparam name="boneIndex">-1</hkparam>
    <hkparam name="animBoneIndex">-1</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbEvaluateHandleModifier =
    R"(<hkobject name="#0000" class="hkbEvaluateHandleModifier" signature="0x79757102">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">EvaluateHandleModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="handle">null</hkparam>
    <hkparam name="handlePositionOut">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="handleRotationOut">(0.000000 0.000000 0.000000 1.000000)</hkparam>
    <hkparam name="isValidOut">false</hkparam>
    <hkparam name="extrapolationTimeStep">0.000000</hkparam>
    <hkparam name="handleChangeSpeed">1.000000</hkparam>
    <hkparam name="handleChangeMode">HANDLE_CHANGE_MODE_ABRUPT</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbExtractRagdollPoseModifier =
    R"(<hkobject name="#0000" class="hkbExtractRagdollPoseModifier" signature="0x804dcbab">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">ExtractRagdollPoseModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="poseMatchingBone0">-1</hkparam>
    <hkparam name="poseMatchingBone1">-1</hkparam>
    <hkparam name="poseMatchingBone2">-1</hkparam>
    <hkparam name="enableComputeWorldFromModel">true</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbGetWorldFromModelModifier =
    R"(<hkobject name="#0000" class="hkbGetWorldFromModelModifier" signature="0x873fc6f7">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">GetWorldFromModelModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="translationOut">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="rotationOut">(0.000000 0.000000 0.000000 1.000000)</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbKeyframeBonesModifier_KeyframeInfo =
    R"(<hkobject>
    <hkparam name="keyframedPosition">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="keyframedRotation">(0.000000 0.000000 0.000000 1.000000)</hkparam>
    <hkparam name="boneIndex">-1</hkparam>
    <hkparam name="isValid">false</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbLookAtModifier =
    R"(<hkobject name="#0000" class="hkbLookAtModifier" signature="0x3d28e066">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">LookAtModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="targetWS">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="headForwardLS">(0.000000 1.000000 0.000000 0.000000)</hkparam>
    <hkparam name="neckForwardLS">(0.000000 1.000000 0.000000 0.000000)</hkparam>
    <hkparam name="neckRightLS">(1.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="eyePositionHS">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="newTargetGain">0.200000</hkparam>
    <hkparam name="onGain">0.050000</hkparam>
    <hkparam name="offGain">0.050000</hkparam>
    <hkparam name="limitAngleDegrees">45.000000</hkparam>
    <hkparam name="limitAngleLeft">90.000000</hkparam>
    <hkparam name="limitAngleRight">-90.000000</hkparam>
    <hkparam name="limitAngleUp">30.000000</hkparam>
    <hkparam name="limitAngleDown">-30.000000</hkparam>
    <hkparam name="headIndex">-1</hkparam>
    <hkparam name="neckIndex">-1</hkparam>
    <hkparam name="isOn">true</hkparam>
    <hkparam name="individualLimitsOn">false</hkparam>
    <hkparam name="isTargetInsideLimitCone">false</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbMirrorModifier =
    R"(<hkobject name="#0000" class="hkbMirrorModifier" signature="0xa9a271ea">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">MirrorModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="isAdditive">false</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbMoveCharacterModifier =
    R"(<hkobject name="#0000" class="hkbMoveCharacterModifier" signature="0x8f7492a0">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">MoveCharacterModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="offsetPerSecondMS">(0.000000 0.000000 0.000000 0.000000)</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbTransformVectorModifier =
    R"(<hkobject name="#0000" class="hkbTransformVectorModifier" signature="0xf93e0e24">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">TransformVectorModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="rotation">(0.000000 0.000000 0.000000 1.000000)</hkparam>
    <hkparam name="translation">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="vectorIn">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="vectorOut">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="rotateOnly">true</hkparam>
    <hkparam name="inverse">true</hkparam>
    <hkparam name="computeOnActivate">true</hkparam>
    <hkparam name="computeOnModify">true</hkparam>
</hkobject>)";

constexpr const char* g_def_BSComputeAddBoneAnimModifier =
    R"(<hkobject name="#0000" class="BSComputeAddBoneAnimModifier" signature="0xa67f8c46">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">ComputeAddBoneAnimModifier1</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="boneIndex">-1</hkparam>
    <hkparam name="translationLSOut">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="rotationLSOut">(0.000000 0.000000 0.000000 1.000000)</hkparam>
    <hkparam name="scaleLSOut">(0.000000 0.000000 0.000000 0.000000)</hkparam>
</hkobject>)";

constexpr const char* g_def_BSDecomposeVectorModifier =
    R"(<hkobject name="#0000" class="BSDecomposeVectorModifier" signature="0x31f6b8b6">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">DecomposeVectorModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="vector">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="x">0.000000</hkparam>
    <hkparam name="y">0.000000</hkparam>
    <hkparam name="z">0.000000</hkparam>
    <hkparam name="w">0.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_BSDistTriggerModifier =
    R"(<hkobject name="#0000" class="BSDistTriggerModifier" signature="0xb34d2bbd">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">DistTriggerModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="targetPosition">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="distance">0.000000</hkparam>
    <hkparam name="distanceTrigger">0.000000</hkparam>
    <hkparam name="triggerEvent">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
</hkobject>)";

constexpr const char* g_def_BSGetTimeStepModifier =
    R"(<hkobject name="#0000" class="BSGetTimeStepModifier" signature="0xbda33bfe">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">GetTimeStepModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="timeStep">0.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_BSInterpValueModifier =
    R"(<hkobject name="#0000" class="BSInterpValueModifier" signature="0x29adc802">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">InterpValueMod</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="source">0.000000</hkparam>
    <hkparam name="target">0.000000</hkparam>
    <hkparam name="result">0.000000</hkparam>
    <hkparam name="gain">0.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_BSIsActiveModifier =
    R"(<hkobject name="#0000" class="BSIsActiveModifier" signature="0xb0fde45a">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">ActiveModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="bIsActive0">false</hkparam>
    <hkparam name="bInvertActive0">false</hkparam>
    <hkparam name="bIsActive1">false</hkparam>
    <hkparam name="bInvertActive1">false</hkparam>
    <hkparam name="bIsActive2">false</hkparam>
    <hkparam name="bInvertActive2">false</hkparam>
    <hkparam name="bIsActive3">false</hkparam>
    <hkparam name="bInvertActive3">false</hkparam>
    <hkparam name="bIsActive4">false</hkparam>
    <hkparam name="bInvertActive4">false</hkparam>
</hkobject>)";

constexpr const char* g_def_BSLimbIKModifier =
    R"(<hkobject name="#0000" class="BSLimbIKModifier" signature="0x8ea971e5">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">LimbIKModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="limitAngleDegrees">0.000000</hkparam>
    <hkparam name="startBoneIndex">-1</hkparam>
    <hkparam name="endBoneIndex">-1</hkparam>
    <hkparam name="gain">0.000000</hkparam>
    <hkparam name="boneRadius">0.000000</hkparam>
    <hkparam name="castOffset">0.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_BSLookAtModifier_Bone =
    R"(<hkobject>
    <hkparam name="index">-1</hkparam>
    <hkparam name="fwdAxisLS">(0.000000 1.000000 0.000000 0.000000)</hkparam>
    <hkparam name="limitAngleDegrees">5.000000</hkparam>
    <hkparam name="onGain">0.050000</hkparam>
    <hkparam name="offGain">0.050000</hkparam>
    <hkparam name="enabled">true</hkparam>
</hkobject>)";

constexpr const char* g_def_BSLookAtModifier =
    R"(<hkobject name="#0000" class="BSLookAtModifier" signature="0xd756fc25">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">BSLookAtModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="lookAtTarget">true</hkparam>
    <hkparam name="bones" numelements="0"></hkparam>
    <hkparam name="eyeBones" numelements="0"></hkparam>
    <hkparam name="limitAngleDegrees">85.000000</hkparam>
    <hkparam name="limitAngleThresholdDegrees">10.000000</hkparam>
    <hkparam name="continueLookOutsideOfLimit">true</hkparam>
    <hkparam name="onGain">0.050000</hkparam>
    <hkparam name="offGain">0.050000</hkparam>
    <hkparam name="useBoneGains">true</hkparam>
    <hkparam name="targetLocation">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="targetOutsideLimits">false</hkparam>
    <hkparam name="targetOutOfLimitEvent">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="lookAtCamera">false</hkparam>
    <hkparam name="lookAtCameraX">0.000000</hkparam>
    <hkparam name="lookAtCameraY">0.000000</hkparam>
    <hkparam name="lookAtCameraZ">0.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_BSModifyOnceModifier =
    R"(<hkobject name="#0000" class="BSModifyOnceModifier" signature="0x1e20a97a">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">BSModifyOnceModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="pOnActivateModifier">null</hkparam>
    <hkparam name="pOnDeactivateModifier">null</hkparam>
</hkobject>)";

constexpr const char* g_def_BSPassByTargetTriggerModifier =
    R"(<hkobject name="#0000" class="BSPassByTargetTriggerModifier" signature="0x703d7b66">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">PassByTargetTriggerModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="targetPosition">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="radius">0.000000</hkparam>
    <hkparam name="movementDirection">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="triggerEvent">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
</hkobject>)";

constexpr const char* g_def_BSRagdollContactListenerModifier =
    R"(<hkobject name="#0260" class="BSRagdollContactListenerModifier" signature="0x8003d8ce">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">CollisionListenerMod</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="contactEvent">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="bones">null</hkparam>
</hkobject>)";

constexpr const char* g_def_BSTimerModifier =
    R"(<hkobject name="#0000" class="BSTimerModifier" signature="0x531f3292">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">TimerModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="alarmTimeSeconds">0.000000</hkparam>
    <hkparam name="alarmEvent">
        <hkobject>
            <hkparam name="id">-1</hkparam>
            <hkparam name="payload">null</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="resetAlarm">false</hkparam>
</hkobject>)";

constexpr const char* g_def_BSTweenerModifier =
    R"(<hkobject name="#0000" class="BSTweenerModifier" signature="0xd2d9a04">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">0</hkparam>
    <hkparam name="name">TweenerModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="tweenPosition">true</hkparam>
    <hkparam name="tweenRotation">true</hkparam>
    <hkparam name="useTweenDuration">true</hkparam>
    <hkparam name="tweenDuration">0.000000</hkparam>
    <hkparam name="targetPosition">(0.000000 0.000000 0.000000 0.000000)</hkparam>
    <hkparam name="targetRotation">(0.000000 0.000000 0.000000 1.000000)</hkparam>
</hkobject>)";

constexpr const char* g_def_BSSpeedSamplerModifier =
    R"(<hkobject name="#0000" class="BSSpeedSamplerModifier" signature="0xd297fda9">
    <hkparam name="variableBindingSet">null</hkparam>
    <hkparam name="userData">o</hkparam>
    <hkparam name="name">BSSpeedSamplerModifier</hkparam>
    <hkparam name="enable">true</hkparam>
    <hkparam name="state">-1</hkparam>
    <hkparam name="direction">0.000000</hkparam>
    <hkparam name="goalSpeed">0.000000</hkparam>
    <hkparam name="speedOut">0.000000</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbStateMachineEventPropertyArray =
    R"(<hkobject name="#0000" class="hkbStateMachineEventPropertyArray" signature="0xb07b4388">
    <hkparam name="events" numelements="0"></hkparam>
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
        DEFMAPITEM(hkbBoneIndexArray),
        DEFMAPITEM(BSBoneSwitchGeneratorBoneData),
        DEFMAPITEM(hkbClipGenerator),
        DEFMAPITEM(hkbClipTriggerArray),
        DEFMAPITEM(BSSynchronizedClipGenerator),
        DEFMAPITEM(hkbManualSelectorGenerator),
        DEFMAPITEM(hkbModifierGenerator),
        DEFMAPITEM(hkbBehaviorReferenceGenerator),
        DEFMAPITEM(hkbPoseMatchingGenerator),
        DEFMAPITEM(BSCyclicBlendTransitionGenerator),
        DEFMAPITEM(BSOffsetAnimationGenerator),
        DEFMAPITEM(hkbModifierList),
        DEFMAPITEM(hkbEvaluateExpressionModifier),
        DEFMAPITEM(hkbExpressionDataArray),
        DEFMAPITEM(hkbEventDrivenModifier),
        DEFMAPITEM(hkbEventsFromRangeModifier),
        DEFMAPITEM(hkbEventRangeDataArray),
        DEFMAPITEM(hkbGetUpModifier),
        DEFMAPITEM(hkbKeyframeBonesModifier),
        DEFMAPITEM(hkbPoweredRagdollControlsModifier),
        DEFMAPITEM(hkbRigidBodyRagdollControlsModifier),
        DEFMAPITEM(hkbRotateCharacterModifier),
        DEFMAPITEM(hkbTimerModifier),
        DEFMAPITEM(hkbFootIkControlsModifier),
        DEFMAPITEM(hkbTwistModifier),
        DEFMAPITEM(BSDirectAtModifier),
        DEFMAPITEM(BSEventEveryNEventsModifier),
        DEFMAPITEM(BSEventOnDeactivateModifier),
        DEFMAPITEM(BSEventOnFalseToTrueModifier),
        DEFMAPITEM(hkbCombineTransformsModifier),
        DEFMAPITEM(hkbComputeDirectionModifier),
        DEFMAPITEM(hkbComputeRotationFromAxisAngleModifier),
        DEFMAPITEM(hkbComputeRotationToTargetModifier),
        DEFMAPITEM(hkbDampingModifier),
        DEFMAPITEM(hkbDelayedModifier),
        DEFMAPITEM(hkbDetectCloseToGroundModifier),
        // DEFMAPITEM(hkbEvaluateHandleModifier),
        DEFMAPITEM(hkbExtractRagdollPoseModifier),
        DEFMAPITEM(hkbGetWorldFromModelModifier),
        DEFMAPITEM(hkbLookAtModifier),
        DEFMAPITEM(hkbMirrorModifier),
        DEFMAPITEM(hkbMoveCharacterModifier),
        DEFMAPITEM(hkbTransformVectorModifier),
        DEFMAPITEM(BSComputeAddBoneAnimModifier),
        DEFMAPITEM(BSDecomposeVectorModifier),
        DEFMAPITEM(BSDistTriggerModifier),
        DEFMAPITEM(BSGetTimeStepModifier),
        DEFMAPITEM(BSInterpValueModifier),
        DEFMAPITEM(BSIsActiveModifier),
        DEFMAPITEM(BSLimbIKModifier),
        DEFMAPITEM(BSLookAtModifier),
        DEFMAPITEM(BSModifyOnceModifier),
        DEFMAPITEM(BSPassByTargetTriggerModifier),
        DEFMAPITEM(BSRagdollContactListenerModifier),
        DEFMAPITEM(BSTimerModifier),
        DEFMAPITEM(BSTweenerModifier),
        DEFMAPITEM(BSSpeedSamplerModifier),
        DEFMAPITEM(hkbStateMachineEventPropertyArray)};
    return map;
}

//////////////////////////    CLASS OF CLASSES

constexpr auto g_class_generators = std::to_array<std::string_view>(
    {"hkbStateMachine",

     "hkbBlenderGenerator",
     "hkbBehaviorReferenceGenerator",
     "hkbClipGenerator",
     "hkbManualSelectorGenerator",
     "hkbModifierGenerator",
     "hkbPoseMatchingGenerator",

     "BSBoneSwitchGenerator",
     "BSCyclicBlendTransitionGenerator",
     "BSiStateTaggingGenerator",
     "BSOffsetAnimationGenerator",
     "BSSynchronizedClipGenerator"});

constexpr auto g_class_modifiers = std::to_array<std::string_view>(
    {"hkbModifierList",

     "hkbCombineTransformsModifier",
     "hkbComputeDirectionModifier",
     "hkbComputeRotationFromAxisAngleModifier",
     "hkbComputeRotationToTargetModifier",
     "hkbDampingModifier",
     "hkbDelayedModifier",
     "hkbDetectCloseToGroundModifier",
     "hkbEvaluateExpressionModifier",
     "hkbEvaluateHandleModifier",
     "hkbEventDrivenModifier",
     "hkbEventsFromRangeModifier",
     "hkbExtractRagdollPoseModifier",
     "hkbFootIkControlsModifier",
     "hkbGetHandleOnBoneModifier",
     "hkbGetUpModifier",
     "hkbGetWorldFromModelModifier",
     "hkbHandIkControlsModifier",
     "hkbKeyframeBonesModifier",
     "hkbLookAtModifier",
     "hkbMirrorModifier",
     "hkbMoveCharacterModifier",
     "hkbPoweredRagdollControlsModifier",
     "hkbRigidBodyRagdollControlsModifier",
     "hkbRotateCharacterModifier",
     "hkbSenseHandleModifier",
     "hkbTimerModifier",
     "hkbTransformVectorModifier",
     "hkbTwistModifier",

     "BSComputeAddBoneAnimModifier",
     "BSDecomposeVectorModifier",
     "BSDirectAtModifier",
     "BSDistTriggerModifier",
     "BSEventEveryNEventsModifier",
     "BSEventOnDeactivateModifier",
     "BSEventOnFalseToTrueModifier",
     "BSGetTimeStepModifier",
     "BSInterpValueModifier",
     "BSIsActiveModifier",
     "BSLimbIKModifier",
     "BSLookAtModifier",
     "BSModifyOnceModifier",
     "BSPassByTargetTriggerModifier",
     "BSRagdollContactListenerModifier",
     "BSTimerModifier",
     "BSTweenerModifier",
     "BSSpeedSamplerModifier"});

constexpr auto g_class_payload          = std::to_array<std::string_view>({"hkbStringEventPayload"});
constexpr auto g_class_transition       = std::to_array<std::string_view>({"hkbBlendingTransitionEffect"});
constexpr auto g_class_expression       = std::to_array<std::string_view>({"hkbExpressionCondition"});
constexpr auto g_class_expr_array       = std::to_array<std::string_view>({"hkbExpressionDataArray"});
constexpr auto g_class_binding          = std::to_array<std::string_view>({"hkbVariableBindingSet"});
constexpr auto g_class_graph_data       = std::to_array<std::string_view>({"hkbBehaviorGraphData"});
constexpr auto g_class_state_chooser    = std::to_array<std::string_view>({"hkbStateChooser"});
constexpr auto g_class_transition_array = std::to_array<std::string_view>({"hkbStateMachineTransitionInfoArray"});
constexpr auto g_class_events_array     = std::to_array<std::string_view>({"hkbStateMachineEventPropertyArray"});
constexpr auto g_class_bone_weights     = std::to_array<std::string_view>({"hkbBoneWeightArray"});
constexpr auto g_class_bone_indices     = std::to_array<std::string_view>({"hkbBoneIndexArray"});
constexpr auto g_class_boneswitch_data  = std::to_array<std::string_view>({"BSBoneSwitchGeneratorBoneData"});
constexpr auto g_class_triggers         = std::to_array<std::string_view>({"hkbClipTriggerArray"});
constexpr auto g_class_clip             = std::to_array<std::string_view>({"hkbClipGenerator"});
constexpr auto g_class_blender_gens     = std::to_array<std::string_view>({"hkbBlenderGenerator", "hkbPoseMatchingGenerator"});
constexpr auto g_class_event_ranges     = std::to_array<std::string_view>({"hkbEventRangeDataArray"});
constexpr auto g_class_key_bones        = std::to_array<std::string_view>({"keyframedBonesList"});

//////////////////////////    CLASS INFO

inline const std::map<std::string_view, std::string_view>& getClassInfo()
{
    static const std::map<std::string_view, std::string_view> map = {
        {"hkbVariableBindingSet", "A set of bindings from variables to object properties."},
        {"hkbStateMachine",
         "A state machine that transitions between generators.\n\n"
         "An hkbStateMachine organizes a set of states, each of which has an hkbGenerator. The state machine is itself an hkbGenerator.\n"
         "When not in a transition, it simply generates a pose by calling the generator corresponding to the current state.\n"
         "During a transition, it generates poses using an hkbTransitionEffect to generate the pose\n"
         "(typically by blending the generator being transitioned from with the one being transitioned to)."},
        {"hkbStateMachineStateInfo", "Information regarding a state of the state machine."},
        {"hkbStringEventPayload", "An event payload that contains a string."},
        {"hkbStateMachineTransitionInfoArray",
         "An array of TransitionInfos wrapped for shared access.\n\n"
         "A TransitionInfo stores information regarding a transition between states."},
        {"hkbBlendingTransitionEffect", "A transition effect that does a linear blend between two generators."},
        {"hkbExpressionCondition", "A condition that is described by an expression string."},
        {"hkbBlenderGenerator",
         "A generator which performs a blend between an arbitrary number of children.\n\n"
         "Each child has a blend weight between 0 and 1. Each child can also have a blend weight for each bone.\n"
         "The two weights are multiplied to get the per-child per-bone weight. For each bone, the weights across all of the children are normalized to sum to 1.\n"
         "If the sum of the weights falls below the threshold specified in m_referencePoseWeightThreshold, the reference pose is blended in.\n\n"
         "When the FLAG_PARAMETRIC_BLEND flag is set the blender generator works as a parametric blender.\n"
         "The parametric blend can be cyclic or acyclic. Cyclic and acyclic differs in the way the boundary conditions are handled.\n\n"
         "During a parametric blend the child generator weights represents the sample points on a line.\n"
         "Based on the value of m_blendParameter an interval is computed such that the parametric blend lies within this interval\n"
         "and the animations that bound the interval are blended. There can be multiple animations with the same weight.\n"
         "If that happens then we have a convention the interval is chosen such that the highest child generator index bounds it on the left side.\n\n"
         "In case of a acyclic parametric blend\n"
         "whenever the value of m_blendParameter is less than the value of first child then the interval between the first and second child is chosen\n"
         "and when it is greater then the value of its last child then the interval between the second last child and the last child is chosen.\n"
         "The same thing happens in cyclic mode when the first child weight is equal to the m_minBlendParameter and the last child weight is equal to m_maxBlendParameter.\n"
         "If that is not the case then when the m_blendParameter goes beyond the first child and m_minBlendParameter is less than the first child weight\n"
         "or m_blendParameter goes beyond the last child weight and m_maxBlendParameter is greater than the last child weight\n"
         "the interval is between the first and the last children."},
        {"hkbBlenderGeneratorChild", "An internal class representing the child of an hkbBlenderGenerator."},
        {"hkbBoneWeightArray", "An array of bone weights used when blending."},
        {"hkbBoneIndexArray", "An array of bone indices that can be bound to behavior variables and character properties."},
        {"hkbClipGenerator", "Generates an animation clip."},
        {"hkbClipTriggerArray",
         "An array of triggers wrapped for shared access.\n\n"
         "A trigger is an event that is attached to the local timeline of an hkbClipGenerator.\n"
         "When the clip passes the time of a trigger, the event associated with the trigger is sent."},
        {"hkbManualSelectorGenerator", "An example custom generator for selecting among a set of child generators."},
        {"hkbModifierGenerator",
         "A generator that wraps another generator and applies a modifier to the results.\n\n"
         "When a method of hkbModifierGenerator is called, it first calls the same method on the child generator.\n"
         "It then calls the analogous method on the stored modifier."},
        {"hkbBehaviorReferenceGenerator",
         "This generator is a wrapper for an hkbBehaviorGraph.\n"
         "It does not save the behavior when serialized. Only the name is saved.\n"
         "The convention is that the user must provide a behavior at runtime, using the name to identify which behavior to use.\n\n"
         "You need to make sure to go through the linking process\n"
         "so that the referencing behavior graph and the referenced behavior graph can communicate events, attributes, and variables.\n"
         "See hkbBehaviorLinkingUtils::linkBehavior() and hkbBehaviorLinkingUtils::linkBehaviors()."},
        {"hkbPoseMatchingGenerator",
         "Matches the ragdoll to the first pose of each of the child generators.\n\n"
         "This generator has two modes.\n"
         "In MODE_MATCH, the current ragdoll pose is matched with the first pose of each of the child generators.\n"
         "The best matching pose is produced as output (gradually).\n"
         "When a new child is chosen to have the best matching pose, that child is blended in over time and the other children are blended out.\n"
         "In MODE_PLAY, the best matching child is played."},
        {"hkbModifierList",
         "Applies a sequence of modifiers as a single modifier.\n\n"
         "This class is especially useful if a behavior graph shares a sequence of modifiers among several branches.\n"
         "You can create the sequence once and then just add it to each branch instead of having to add all of the modifiers individually."},
        {"hkbEvaluateExpressionModifier", "A modifier for evaluating expressions."},
        {"hkbExpressionDataArray", "An array of expressions wrapped for shared access."},
        {"hkbEventDrivenModifier", "A modifier that activates/deactivates its child modifier on receiving events"},
        {"hkbEventsFromRangeModifier", "A modifier for firing events, given a variable mapped onto a set of ranges."},
        {"hkbEventRangeDataArray", "An array of expressions wrapped for shared access."},
        {"hkbGetUpModifier",
         "Gradually aligns the up-vector of the character's world-from-model transform with the world up-vector.\n"
         "This is useful when the character is getting up after having been a ragdoll.\n"
         "When using the hkbPoweredRagdollModifier the world-from-model of the character is typically allowed to rotate freely.\n"
         "But when you want the character to stand up again, the world-from-model needs to be realigned with the world so that the character stands up straight."},
        {"hkbKeyframeBonesModifier",
         "This modifier allows you to specify which bones are keyframed. One float per bone is placed into the track data.\n"
         "This data is passed through the behavior graph and can be blended by blend nodes and during transitions.\n"
         "The track data is used by hkbPoweredRagdollModifier and hkbRigidBodyRagdollModifier to decide which bones should be keyframed."},
        {"hkbPoweredRagdollControlsModifier",
         "Produces control data used by hkbPoweredRagdollModifier and puts it into the tracks of the output when modify() is called.\n"
         "This data is passed through the behavior tree and can be blended by blend nodes and during transitions."},
        {"hkbRigidBodyRagdollControlsModifier",
         "Produces control data used by hkbRigidBodyRagdollModifier and puts it into the tracks of the output when modify() is called.\n"
         "This data is passed through the behavior tree and can be blended by blend nodes and during transitions."},
        {"hkbRotateCharacterModifier", "A modifier that rotates the character worldFromModel in response to events."},
        {"hkbTimerModifier", "A modifier for sending an event after a time lapse."},
        {"hkbFootIkControlsModifier",
         "Places foot IK control data into the track data to be blended in a behavior graph.\n\n"
         "The control data is consumed by the hkbFootIkModifier, which should be placed rootward of this node in the behavior graph."},
        {"hkbTwistModifier", "A modifier to twist a chain of bones (such as the spine of the character) around a given axis."},
        {"hkbCombineTransformsModifier",
         "This modifier multiplies two input transforms and produces an output transform.\n"
         "The inputs and outputs are in the form of a translation vector and a quaternion.\n"
         "You can optionally request inversion of the inputs and output."},
        {"hkbComputeDirectionModifier", "Given an input point, this modifier outputs the point in the character's coordinate frame, and also a vertical angle and ground angle."},
        {"hkbComputeRotationFromAxisAngleModifier", "This modifier converts an axis and angle into a quaternion."},
        {"hkbComputeRotationToTargetModifier", "Computes the rotation about a given axis that will face an object toward a given target."},
        {"hkbDampingModifier", "This is a modifier that uses a PID controller to provide variable damping."},
        {"hkbDelayedModifier", "A modifier that applies another modifier after a delay, and optionally for a finite duration."},
        {"hkbDetectCloseToGroundModifier", "A modifier that sends an event when the character is close to the ground."},
        {"hkbExtractRagdollPoseModifier", "This modifier allows you to extract the current pose of the ragdoll"},
        {"hkbGetWorldFromModelModifier", "A modifier that gets the worldFromModel."},
        {"hkbLookAtModifier", "A modifier that looks at a target point."},
        {"hkbMirrorModifier", "This simply mirrors the animation"},
        {"hkbMoveCharacterModifier", "A modifier that moves the character position by a constant offset per unit time."},
        {"hkbTransformVectorModifier", "Transforms a given vector by a given matrix."},
        {"hkbStateMachineEventPropertyArray",
         "An array of EventProperties wrapped for shared access.\n\n"
         "An EventProperty is an event that is used as a node property.\n"
         "It differs from hkbEvent in that the payload is reference counted and it doesn't have a sender."},
    };
    return map;
}

} // namespace Hkx
} // namespace Haviour