#include "classinterface.h"

#include "widgets.h"
#include "hkx/hkclass.inl"

#include <robin_hood.h>
#include <imgui.h>
#include <extern/font_awesome_5.h>

namespace Haviour
{
namespace Ui
{

constexpr auto EditAreaTableFlag = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;

#define UICLASS(hkclass) void show_##hkclass(pugi::xml_node obj, Hkx::HkxFile& file)

///////////////////////    NOT REGISTERED BUT COMMONLY USED

UICLASS(hkbStringEventPayload)
{
    stringEdit(obj.getByName("data"));
}

UICLASS(hkbExpressionCondition)
{
    stringEdit(obj.getByName("expression"));
}

UICLASS(hkbEvent)
{
    linkedPropPickerEdit<Hkx::AnimationEvent>(obj.getByName("id"), file.m_evt_manager);
    refEdit(obj.getByName("payload"), {"hkbStringEventPayload"}, file);
    if (auto payload = Hkx::HkxFileManager::getSingleton()->getCurrentFile().getObj(obj.getByName("payload").text().as_string()); payload)
        show_hkbStringEventPayload(payload, file);
}
void hkbEventSelectable(pugi::xml_node obj, Hkx::HkxFile& file, const char* hint, const char* text)
{
    ImGui::PushID(text);

    ImGui::TableNextColumn();
    if (ImGui::Button(ICON_FA_PEN_SQUARE))
        ImGui::OpenPopup(text);
    ImGui::SameLine();
    if (ImGui::Selectable(text))
        ImGui::OpenPopup(text);
    if (hint)
        addTooltip(hint);
    if (ImGui::BeginPopup(text, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::BeginTable("hkbBehaviorGraph", 2, ImGuiTableFlags_SizingStretchProp, {400.0f, -FLT_MIN}))
        {
            show_hkbEvent(obj, file);
            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }
    ImGui::TableNextColumn();

    ImGui::PopID();
}

UICLASS(hkbStateMachineEventPropertyArray)
{
    auto events_node = obj.getByName("events");

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("events"), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        appendXmlString(events_node, Hkx::g_def_hkbEvent);
    addTooltip("Add new event");
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(events_node.attribute("numelements").as_string());

    constexpr auto table_flag = ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
    if (ImGui::BeginTable("events", 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        pugi::xml_node mark_delete = {};
        bool           do_delete   = false;
        auto           numelements = events_node.attribute("numelements").as_ullong();
        size_t         i           = 0;
        for (auto event : events_node.children())
        {
            ImGui::PushID(i);
            hkbEventSelectable(event, file, {},
                               file.m_evt_manager.getEntry(event.getByName("id").text().as_ullong()).getName());
            if (ImGui::Button(ICON_FA_MINUS_CIRCLE))
            {
                do_delete   = true;
                mark_delete = event;
            }
            addTooltip("Remove");
            ImGui::PopID();

            ++i;
        }
        if (do_delete)
        {
            events_node.remove_child(mark_delete);
            events_node.attribute("numelements") = events_node.attribute("numelements").as_int() - 1;
        }

        ImGui::EndTable();
    }
}

UICLASS(hkbStateMachine_TimeInterval)
{
    linkedPropPickerEdit<Hkx::AnimationEvent>(obj.getByName("enterEventId"), file.m_evt_manager,
                                              "The event ID that bounds the beginning of the interval (set to EVENT_ID_NULL to use m_enterTime instead).");
    linkedPropPickerEdit<Hkx::AnimationEvent>(obj.getByName("exitEventId"), file.m_evt_manager,
                                              "The event ID that bounds the end of the interval (set to EVENT_ID_NULL to use m_exitTime instead).");
    floatEdit(obj.getByName("enterTime"),
              "The time at which the interval is entered (used if both m_enterEventId and m_exitEventId are set to EVENT_ID_NULL).");
    floatEdit(obj.getByName("exitTime"),
              "The time at which the interval is exited (used if both m_enterEventId and m_exitEventId are set to EVENT_ID_NULL).\n"
              "Setting this to 0.0f means infinity; the inverval will not end.");
}

UICLASS(hkbStateMachine_TransitionInfo)
{
    ImGui::TableNextColumn();
    bool open_tree_node = ImGui::TreeNode("triggerInterval");
    addTooltip("The interval in which the event must be received for the transition to occur.\n\n"
               "This is only used if (m_flags & FLAG_USE_TRIGGER_INTERVAL) is true.\n"
               "You should make sure that the interval is longer than your timestep (eg, 1/30 sec), or else the interval may be missed.");
    if (open_tree_node)
    {
        if (ImGui::BeginTable("interval", 2, EditAreaTableFlag))
        {
            show_hkbStateMachine_TimeInterval(obj.getByName("triggerInterval").first_child(), file);
            ImGui::EndTable();
        }
        ImGui::TreePop();
    }
    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    open_tree_node = ImGui::TreeNode("initiateInterval");
    addTooltip("The interval in which the transition may begin.\n\n"
               "This is only used if (m_flags & FLAG_USE_BEGIN_INTERVAL) is true.\n"
               "If the transition is activated outside of this interval, the transition will be delayed until the interval begins.\n"
               "You should make sure that the interval is longer than your timestep (eg, 1/30 sec), or else the interval may be missed.");
    if (open_tree_node)
    {
        if (ImGui::BeginTable("interval", 2, EditAreaTableFlag))
        {
            show_hkbStateMachine_TimeInterval(obj.getByName("triggerInterval").first_child(), file);
            ImGui::EndTable();
        }
        ImGui::TreePop();
    }
    ImGui::TableNextRow();

    refEdit(obj.getByName("transition"), {"hkbBlendingTransitionEffect"}, file,
            "The transition to apply.\n\n"
            "This is public but should be handled with care since it is reference counted.\n"
            "It's best to change this only by calling methods of hkbStateMachine.");
    refEdit(obj.getByName("condition"), {"hkbExpressionCondition"}, file,
            "A condition that determines whether the transition should occur.\n\n"
            "The transition only occurs if this condition evaluates to true.\n"
            "If m_event == hkbEvent::EVENT_ID_NULL then the transition occurs as soon as the condition becomes true.\n"
            "Otherwise, the transition occurs when the event is received and the condition is also true.");
    auto condition = file.getObj(obj.getByName("condition").text().as_string());
    if (condition)
    {
        ImGui::Indent(10.0F);
        show_hkbExpressionCondition(condition, file);
        ImGui::Indent(-10.0F);
    }

    linkedPropPickerEdit<Hkx::AnimationEvent>(obj.getByName("eventId"), file.m_evt_manager);
    fullTableSeparator();
    stateEdit(obj.getByName("toStateId"), file,
              "The state to which the transition goes.");
    stateEdit(obj.getByName("fromNestedStateId"), file,
              "The nested state (a state within the state machine that is inside the from-state) from which this transition must be done.");
    stateEdit(obj.getByName("toNestedStateId"), file,
              "A nested state to which this transitions goes.");
    fullTableSeparator();
    intScalarEdit(obj.getByName("priority"), ImGuiDataType_S16,
                  "Each transition has a priority.");
    flagEdit(obj.getByName("flags"), Hkx::f_hkbStateMachine_TransitionInfo_TransitionFlags,
             "Flags indicating specialized behavior (see TransitionInfoTransitionFlagBits).");
}

///////////////////////    REGISTERED INTERFACES

UICLASS(hkbBehaviorGraph)
{
    if (ImGui::BeginTable("hkbBehaviorGraph", 4, EditAreaTableFlag))
    {
        stringEdit(obj.getByName("name"));
        intScalarEdit(obj.getByName("userData"), ImGuiDataType_U32);
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
        enumEdit(obj.getByName("variableMode"), Hkx::e_variableMode,
                 "How do deal with variables when the behavior is inactive.");
        refEdit(obj.getByName("rootGenerator"), {"hkbStateMachine"}, file,
                "The root node of the behavior graph.");
        refEdit(obj.getByName("data"), {"hkbBehaviorGraphData"}, file,
                "The constant data associated with the behavior.");
        ImGui::EndTable();
    }
}

UICLASS(hkbVariableBindingSet)
{
    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_binding = {};
    if (obj_cache != obj)
        edit_binding = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    auto bindings = obj.getByName("bindings");
    if (ImGui::BeginTable("hkbBehaviorGraph1", 2, ImGuiTableFlags_SizingStretchProp))
    {
        intScalarEdit(obj.getByName("indexOfBindingToEnable"), ImGuiDataType_S32,
                      "If there is a binding to the member hkbModifier::m_enable then we store its index here.");
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbBehaviorGraph2", 2))
    {
        ImGui::TableSetupColumn("bindings", ImGuiTableColumnFlags_WidthFixed, 200.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        // bindings
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Bindings"), ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
            edit_binding = appendXmlString(bindings, Hkx::g_def_hkbVariableBindingSet_Binding);
        addTooltip("Add new binding");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_MINUS_CIRCLE) && edit_binding)
            if (bindings.remove_child(edit_binding))
            {
                edit_binding                      = {};
                bindings.attribute("numelements") = bindings.attribute("numelements").as_int() - 1;
            }
        addTooltip("Remove currently editing binding.");
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(bindings.attribute("numelements").as_string());

        if (ImGui::BeginListBox("##bindings", ImVec2(-FLT_MIN, -FLT_MIN)))
        {
            for (auto binding = bindings.first_child(); binding; binding = binding.next_sibling())
            {
                const bool is_selected = edit_binding == binding;
                if (ImGui::Selectable(binding.getByName("memberPath").text().as_string(), is_selected))
                    edit_binding = binding;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }

        ImGui::TableNextColumn();
        if (edit_binding)
        {
            if (ImGui::BeginTable("binding", 2, EditAreaTableFlag))
            {
                stringEdit(edit_binding.getByName("memberPath"),
                           "The memberPath is made up of member names, separated by '/'. Integers after colons in the path are array indices.\n"
                           "For example, \"children:2/blendWeight\" would seek an array member named \"children\", access the second member,\n"
                           "and then look for a member named \"blendWeight\" in that object.");
                if (!strcmp(edit_binding.getByName("bindingType").text().as_string(), "BINDING_TYPE_VARIABLE")) // binding variables
                    linkedPropPickerEdit<Hkx::Variable>(edit_binding.getByName("variableIndex"), file.m_var_manager,
                                                        "The index of the variable that is bound to an object member.");
                else
                    intScalarEdit(edit_binding.getByName("variableIndex"), ImGuiDataType_S32);
                intScalarEdit(edit_binding.getByName("bitIndex"), ImGuiDataType_S8,
                              "The index of the bit to which the variable is bound.\n"
                              "A value of -1 indicates that we are not binding to an individual bit.");
                enumEdit(edit_binding.getByName("bindingType"), Hkx::e_bindingType,
                         "Which data we are binding to.");
                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("No binding selected.");

        ImGui::EndTable();
    }
}

UICLASS(hkbStateMachine)
{
    if (ImGui::BeginTable("hkbStateMachine1", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("states", ImGuiTableColumnFlags_WidthFixed, 300.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        refList(obj.getByName("states"), {"hkbStateMachineStateInfo"}, file, "stateId", "name", "States");

        ImGui::TableNextColumn();
        if (ImGui::BeginTable("hkbStateMachine2", 2, EditAreaTableFlag))
        {
            stringEdit(obj.getByName("name"));
            intScalarEdit(obj.getByName("userData"), ImGuiDataType_U32);
            refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
            hkbEventSelectable(obj.getByName("eventToSendWhenStateOrTransitionChanges").first_child(), file,
                               "If non-null, this event is sent at the beginning and end of a transition, or once for an instantaneous transition.",
                               "eventToSendWhenStateOrTransitionChanges");
            fullTableSeparator();
            refEdit(obj.getByName("startStateChooser"), {"hkbStateChooser"}, file);                                    // this one in my ver replaced by startStateIdSelector
            intScalarEdit(obj.getByName("startStateId"), ImGuiDataType_S32, "an object that chooses the start state"); // need to change to state selector
            fullTableSeparator();
            linkedPropPickerEdit<Hkx::AnimationEvent>(obj.getByName("returnToPreviousStateEventId"), file.m_evt_manager,
                                                      "If this event is received, the state machine returns to the previous state if there is an appropriate transition defined.");
            linkedPropPickerEdit<Hkx::AnimationEvent>(obj.getByName("randomTransitionEventId"), file.m_evt_manager,
                                                      "If this event is received, the state machine chooses a random transition from among those available.\n\n"
                                                      "The event of the transition is ignored, but all other considerations of whether the transition should be taken are considered.");
            linkedPropPickerEdit<Hkx::AnimationEvent>(obj.getByName("transitionToNextHigherStateEventId"), file.m_evt_manager,
                                                      "If the event is received, the state machine chooses a state with the id higher than the m_currentStateId and do a transition to that state.\n\n"
                                                      "The event of the transition is ignored, but all other considerations of whether the transition should be taken are considered.\n"
                                                      "If no appropriate transition is found, an immediate transition will occur.");
            linkedPropPickerEdit<Hkx::AnimationEvent>(obj.getByName("transitionToNextLowerStateEventId"), file.m_evt_manager,
                                                      "If the event is received, the state machine chooses a state with the id lower than the m_currentStateId and do a transition to that state.\n\n"
                                                      "The event of the transition is ignored, but all other considerations of whether the transition should be taken are considered.\n"
                                                      "If no appropriate transition is found, an immediate transition will occur.");
            fullTableSeparator();
            linkedPropPickerEdit<Hkx::Variable>(obj.getByName("syncVariableIndex"), file.m_var_manager,
                                                "We use variables to sync the start state of the state machine.\n\n"
                                                "The value of this index currently must be initialized to the state id which the user wants the start state to be.\n"
                                                "The state machine syncs it start state by setting the value of the start state to that of the variable during activate\n"
                                                "and sets the variable value to the toState when the transition begins.");
            boolEdit(obj.getByName("wrapAroundStateId"),
                     "This decides whether to transition when the m_currentStateId is maximum and m_transitionToNextHigherStateEventId is set\n"
                     "m_currentStateId is minimum and m_transitionToNextLowerStateEventId is set.\n\n"
                     "If this is set to false there would be no transition.\n"
                     "Otherwise there would be a transition but in the first case the next state id will be a lower than the current one\n"
                     "and in the second case the next state id would be higher than the current state id.");
            intScalarEdit(obj.getByName("maxSimultaneousTransitions"), ImGuiDataType_S8,
                          "The number of transitions that can be active at once.\n\n"
                          "When a transition B interrupts another transition A, the state machine can continue playing A an input to B.\n"
                          "If a transition is triggered when there are already m_maxSimultaneousTransitions active transitions,\n"
                          "then the oldest transition will be immediately ended to make room for a new one.");
            enumEdit(obj.getByName("startStateMode"), Hkx::e_hkbStateMachine_StartStateMode,
                     "How to set the start state.");
            enumEdit(obj.getByName("selfTransitionMode"), Hkx::e_hkbStateMachine_StateMachineSelfTransitionMode,
                     "How to deal with self-transitions (when the state machine is transitioned to while still active).");
            fullTableSeparator();

            refEdit(obj.getByName("wildcardTransitions"), {"hkbStateMachineTransitionInfoArray"}, file,
                    "the list of transitions from any state (don't have a specific from state)");

            ImGui::EndTable();
        }

        ImGui::EndTable();
    }
}

UICLASS(hkbStateMachineStateInfo)
{
    if (ImGui::BeginTable("hkbStateMachineStateInfo1", 4, EditAreaTableFlag))
    {
        auto& file = Hkx::HkxFileManager::getSingleton()->getCurrentFile();

        stringEdit(obj.getByName("name"));
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
        intScalarEdit(obj.getByName("stateId"));
        sliderFloatEdit(obj.getByName("probability"), 0.0f, 1.0f,
                        "The state probability.  When choosing a random start state, each state is weighted according to its probability.\n"
                        "The probabilities of all of the states being considered are normalized so that their sum is 1.\n"
                        "When choosing a random transition, each transition in weighted according to the probability of the to-state.");
        boolEdit(obj.getByName("enable"),
                 "Enable this state. Otherwise, it will be inaccessible.");
        // listeners seems unused
        ImGui::TableNextRow();
        fullTableSeparator();
        refEdit(obj.getByName("generator"), std::vector(Hkx::g_class_generators.begin(), Hkx::g_class_generators.end()), file,
                "The generator associated with this state.");
        refEdit(obj.getByName("transitions"), {"hkbStateMachineTransitionInfoArray"}, file,
                "The transitions out of this state.");
        fullTableSeparator();

        auto enter_events = obj.getByName("enterNotifyEvents");
        refEdit(enter_events, {"hkbStateMachineEventPropertyArray"}, file,
                "These events are sent when the state is entered.");
        auto exit_events = obj.getByName("exitNotifyEvents");
        refEdit(exit_events, {"hkbStateMachineEventPropertyArray"}, file,
                "These events are sent when the state is exited.");

        if (auto node = file.getObj(enter_events.text().as_string()); node)
        {
            ImGui::TableNextColumn();
            ImGui::PushID(0);
            show_hkbStateMachineEventPropertyArray(node, file);
            ImGui::PopID();
            ImGui::TableNextColumn();
        }

        if (auto node = file.getObj(exit_events.text().as_string()); node)
        {
            ImGui::TableNextColumn();
            ImGui::PushID(1);
            show_hkbStateMachineEventPropertyArray(node, file);
            ImGui::PopID();
            ImGui::TableNextColumn();
        }

        ImGui::EndTable();
    }
}

UICLASS(hkbStateMachineTransitionInfoArray)
{
    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_trans = {};
    if (obj_cache != obj)
        edit_trans = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    if (ImGui::BeginTable("hkbStateMachineTransitionInfoArray1", 2))
    {
        ImGui::TableSetupColumn("trans", ImGuiTableColumnFlags_WidthFixed, 200.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        auto transitions_node = obj.getByName("transitions");

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("transitions"), ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
            appendXmlString(transitions_node, Hkx::g_def_hkbStateMachine_TransitionInfo);
        addTooltip("Add new event");
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(transitions_node.attribute("numelements").as_string());

        constexpr auto table_flag = ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
        if (ImGui::BeginTable("transitions", 2, table_flag))
        {
            pugi::xml_node mark_delete = {};
            bool           do_delete   = false;
            auto           numelements = transitions_node.attribute("numelements").as_ullong();
            size_t         i           = 0;
            for (auto transition : transitions_node.children())
            {
                ImGui::PushID(i);

                ImGui::TableNextColumn();
                if (ImGui::Button(ICON_FA_MINUS_CIRCLE))
                {
                    do_delete   = true;
                    mark_delete = transition;
                }
                addTooltip("Remove");

                ImGui::TableNextColumn();
                bool selected = false;
                if (ImGui::Selectable(std::format("To: {}", transition.getByName("toStateId").text().as_string()).c_str(), &selected))
                    edit_trans = transition;

                ImGui::PopID();

                ++i;
            }
            if (do_delete)
            {
                if (edit_trans == mark_delete)
                    edit_trans = {};
                transitions_node.remove_child(mark_delete);
                transitions_node.attribute("numelements") = transitions_node.attribute("numelements").as_int() - 1;
            }

            ImGui::EndTable();
        }

        ImGui::TableNextColumn();
        if (edit_trans)
        {
            if (ImGui::BeginTable("hkbStateMachineTransitionInfoArray2", 2, EditAreaTableFlag))
            {
                show_hkbStateMachine_TransitionInfo(edit_trans, file);
                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("Not editing any transition");

        ImGui::EndTable();
    }
}

UICLASS(hkbBlendingTransitionEffect)
{
    if (ImGui::BeginTable("hkbBlendingTransitionEffect", 4, EditAreaTableFlag))
    {
        stringEdit(obj.getByName("name"));
        intScalarEdit(obj.getByName("userData"), ImGuiDataType_U32);
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

        ImGui::TableNextRow();
        fullTableSeparator();

        enumEdit(obj.getByName("selfTransitionMode"), Hkx::e_hkbTransitionEffect_SelfTransitionMode,
                 "What to do if the to-generator is already active when the transition activates it.");
        enumEdit(obj.getByName("eventMode"), Hkx::e_hkbTransitionEffect_EventMode,
                 "How to process the events of the from- and to-generators.");

        fullTableSeparator();

        sliderFloatEdit(obj.getByName("duration"), 0.0f, 1000.0f,
                        "The duration of the transition.");
        sliderFloatEdit(obj.getByName("toGeneratorStartTimeFraction"), 0.0f, 1.0f,
                        "The start time of the to-generator when the transition begins, expressed as a fraction of its duration.");

        fullTableSeparator();

        enumEdit(obj.getByName("endMode"), Hkx::e_hkbBlendingTransitionEffect_EndMode,
                 "The treatment of the end of the from-generator.");
        enumEdit(obj.getByName("blendCurve"), Hkx::e_hkbBlendCurveUtils_BlendCurve,
                 "Which blend curve to use.");
        flagEdit(obj.getByName("flags"), Hkx::f_hkbBlendingTransitionEffect_FlagBits,
                 "Flags to indicate specialized behavior.");
    }
}

UICLASS(hkbBlenderGenerator)
{
    if (ImGui::BeginTable("hkbBlenderGenerator1", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("children", ImGuiTableColumnFlags_WidthFixed, 300.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        refList(obj.getByName("children"), {"hkbBlenderGeneratorChild"}, file, "", "", "Children");

        ImGui::TableNextColumn();
        if (ImGui::BeginTable("hkbBlenderGenerator2", 2, EditAreaTableFlag))
        {
            stringEdit(obj.getByName("name"));
            intScalarEdit(obj.getByName("userData"), ImGuiDataType_U32);
            refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

            ImGui::TableNextRow();
            fullTableSeparator();

            floatEdit(obj.getByName("referencePoseWeightThreshold"),
                      "If the sum of non-additive generator weights falls below this threshold, the reference pose is blended in.");
            floatEdit(obj.getByName("blendParameter"),
                      "This value controls the parametric blend.");
            floatEdit(obj.getByName("minCyclicBlendParameter"),
                      "The minimum value the blend parameter can have when doing a cyclic parametric blend.");
            floatEdit(obj.getByName("maxCyclicBlendParameter"),
                      "The maimum value the blend parameter can have when doing a cyclic parametric blend.");
            intScalarEdit(obj.getByName("indexOfSyncMasterChild"), ImGuiDataType_S16,
                          "If you want a particular child's duration to be used to sync all of the other children, set this to the index of the child.\n"
                          "Otherwise, set it to -1.");
            flagEdit(obj.getByName("flags"), Hkx::f_hkbBlenderGenerator_BlenderFlags,
                     "The flags affecting specialized behavior.", "", true);
            boolEdit(obj.getByName("subtractLastChild"),
                     "If this is set to true then the last child will be a subtracted from the blend of the rest");

            ImGui::EndTable();
        }

        ImGui::EndTable();
    }
}

UICLASS(hkbBlenderGeneratorChild)
{
    if (ImGui::BeginTable("hkbBlenderGeneratorChild", 4, EditAreaTableFlag))
    {
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

        ImGui::TableNextRow();
        fullTableSeparator();

        refEdit(obj.getByName("generator"), std::vector(Hkx::g_class_generators.begin(), Hkx::g_class_generators.end()), file,
                "The generator associated with this child.");
        refEdit(obj.getByName("boneWeights"), {"hkbBoneWeightArray"}, file,
                "A weight for each bone");
        floatEdit(obj.getByName("weight"),
                  "The blend weight for this child.");
        floatEdit(obj.getByName("worldFromModelWeight"),
                  "The weights used to determine the contribution of this child's world-from-model transform to the final world-from-model.");

        ImGui::EndTable();
    }
}

#define UIMAPITEM(hkclass)       \
    {                            \
#        hkclass, show_##hkclass \
    }

const robin_hood::unordered_map<std::string_view, std::function<void(pugi::xml_node, Hkx::HkxFile&)>> g_class_ui_map =
    {UIMAPITEM(hkbBehaviorGraph),
     UIMAPITEM(hkbVariableBindingSet),
     UIMAPITEM(hkbStateMachine),
     UIMAPITEM(hkbStateMachineStateInfo),
     UIMAPITEM(hkbStateMachineTransitionInfoArray),
     UIMAPITEM(hkbBlendingTransitionEffect),
     UIMAPITEM(hkbBlenderGenerator),
     UIMAPITEM(hkbBlenderGeneratorChild)};

void showEditUi(pugi::xml_node hkobject, Hkx::HkxFile& file)
{
    auto hkclass = hkobject.attribute("class").as_string();
    if (g_class_ui_map.contains(hkclass))
        g_class_ui_map.at(hkclass)(hkobject, file);
    else
        ImGui::TextDisabled("The class %s is currently not supported.", hkclass);
}

} // namespace Ui
} // namespace Haviour