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

#define DEFUICLASS(hkclass) void show_##hkclass(pugi::xml_node obj, Hkx::HkxFile& file)

///////////////////////    NOT REGISTERED BUT COMMONLY USED

DEFUICLASS(hkbEvent)
{
    eventPickerEdit(obj.find_child_by_attribute("name", "id"), file.m_evt_manager);
    refEdit(obj.find_child_by_attribute("name", "payload"), {"hkbStringEventPayload"}, file);
}
void hkbEventSelectable(pugi::xml_node obj, Hkx::HkxFile& file, const char* hint, const char* text)
{
    ImGui::TableNextColumn();
    if (ImGui::Selectable(text))
        ImGui::OpenPopup(text);
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
}

///////////////////////    REGISTERED INTERFACES

constexpr auto EditAreaTableFlag = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;

DEFUICLASS(hkbBehaviorGraph)
{
    if (ImGui::BeginTable("hkbBehaviorGraph", 4, EditAreaTableFlag))
    {
        stringEdit(obj.find_child_by_attribute("name", "name"));
        intScalarEdit(obj.find_child_by_attribute("name", "userData"), ImGuiDataType_U32);
        refEdit(obj.find_child_by_attribute("name", "variableBindingSet"), {"hkbVariableBindingSet"}, file);
        enumEdit(obj.find_child_by_attribute("name", "variableMode"), Hkx::e_variableMode,
                 "How do deal with variables when the behavior is inactive.");
        refEdit(obj.find_child_by_attribute("name", "rootGenerator"), {"hkbStateMachine"}, file,
                "The root node of the behavior graph.");
        refEdit(obj.find_child_by_attribute("name", "data"), {"hkbBehaviorGraphData"}, file,
                "The constant data associated with the behavior.");
        ImGui::EndTable();
    }
}

DEFUICLASS(hkbVariableBindingSet)
{
    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_binding = {};
    if (obj_cache != obj)
        edit_binding = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    auto bindings = obj.find_child_by_attribute("name", "bindings");
    if (ImGui::BeginTable("hkbBehaviorGraph1", 2, ImGuiTableFlags_SizingStretchProp))
    {
        intScalarEdit(obj.find_child_by_attribute("name", "indexOfBindingToEnable"), ImGuiDataType_S32,
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
            if (edit_binding = appendXmlString(bindings, Hkx::g_def_hkbVariableBindingSet_Binding); edit_binding)
                bindings.attribute("numelements") = bindings.attribute("numelements").as_int() + 1;
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
                if (ImGui::Selectable(binding.find_child_by_attribute("name", "memberPath").text().as_string(), is_selected))
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
                stringEdit(edit_binding.find_child_by_attribute("name", "memberPath"),
                           "The memberPath is made up of member names, separated by '/'. Integers after colons in the path are array indices.\n"
                           "For example, \"children:2/blendWeight\" would seek an array member named \"children\", access the second member,\n"
                           "and then look for a member named \"blendWeight\" in that object.");
                if (!strcmp(edit_binding.find_child_by_attribute("name", "bindingType").text().as_string(), "BINDING_TYPE_VARIABLE")) // binding variables
                    variablePickerEdit(edit_binding.find_child_by_attribute("name", "variableIndex"), file.m_var_manager,
                                       "The index of the variable that is bound to an object member.");
                else
                    intScalarEdit(edit_binding.find_child_by_attribute("name", "variableIndex"), ImGuiDataType_S32);
                intScalarEdit(edit_binding.find_child_by_attribute("name", "bitIndex"), ImGuiDataType_S8,
                              "The index of the bit to which the variable is bound.\n"
                              "A value of -1 indicates that we are not binding to an individual bit.");
                enumEdit(edit_binding.find_child_by_attribute("name", "bindingType"), Hkx::e_bindingType,
                         "Which data we are binding to.");
                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("No binding selected.");

        ImGui::EndTable();
    }
}

DEFUICLASS(hkbStringEventPayload)
{
    if (ImGui::BeginTable("hkbStringEventPayload", 2, EditAreaTableFlag))
    {
        stringEdit(obj.find_child_by_attribute("name", "data"));
        ImGui::EndTable();
    }
}

DEFUICLASS(hkbStateMachine)
{
    if (ImGui::BeginTable("hkbStateMachine1", 2))
    {
        ImGui::TableSetupColumn("states", ImGuiTableColumnFlags_WidthFixed, 200.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        // states
        std::vector<std::string> states      = {};
        auto                     states_node = obj.find_child_by_attribute("name", "states");
        size_t                   num_states  = states_node.attribute("numelements").as_ullong();
        std::istringstream       states_stream(states_node.text().as_string());
        for (size_t i = 0; i < num_states; ++i)
        {
            std::string temp_str;
            states_stream >> temp_str;
            states.push_back(temp_str);
        }

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("States"), ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        {
            states_node.attribute("numelements") = states_node.attribute("numelements").as_int() + 1;
            states.push_back("null");
        }
        addTooltip("Add new state");
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(states_node.attribute("numelements").as_string());

        constexpr auto table_flag = ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
        if (ImGui::BeginTable("states", 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
        {
            size_t mark_delete = UINT64_MAX;
            bool   do_delete   = false;
            for (size_t i = 0; i < states.size(); ++i)
            {
                ImGui::SetNextItemWidth(60);
                refEdit(states[i], {"hkbStateMachineStateInfo"}, obj, file, "",
                        Hkx::HkxFileManager::getSingleton()->getCurrentFile().getObj(states[i]).find_child_by_attribute("name", "stateId").text().as_string());
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_MINUS_CIRCLE))
                {
                    do_delete   = true;
                    mark_delete = i;
                }
                addTooltip("Remove");
            }
            if (do_delete)
                states.erase(states.begin() + mark_delete);

            states_node.text() = printIdVector(states).c_str();

            ImGui::EndTable();
        }

        ImGui::TableNextColumn();
        if (ImGui::BeginTable("hkbStateMachine2", 2, EditAreaTableFlag))
        {
            stringEdit(obj.find_child_by_attribute("name", "name"));
            intScalarEdit(obj.find_child_by_attribute("name", "userData"), ImGuiDataType_U32);
            refEdit(obj.find_child_by_attribute("name", "variableBindingSet"), {"hkbVariableBindingSet"}, file);
            hkbEventSelectable(obj.find_child_by_attribute("name", "eventToSendWhenStateOrTransitionChanges").first_child(), file,
                               "If non-null, this event is sent at the beginning and end of a transition, or once for an instantaneous transition.",
                               "eventToSendWhenStateOrTransitionChanges");
            twoSepartors();
            refEdit(obj.find_child_by_attribute("name", "startStateChooser"), {"hkbStateChooser"}, file);                                    // this one in my ver replaced by startStateIdSelector
            intScalarEdit(obj.find_child_by_attribute("name", "startStateId"), ImGuiDataType_S32, "an object that chooses the start state"); // need to change to state selector
            twoSepartors();
            eventPickerEdit(obj.find_child_by_attribute("name", "returnToPreviousStateEventId"), file.m_evt_manager,
                            "If this event is received, the state machine returns to the previous state if there is an appropriate transition defined.");
            eventPickerEdit(obj.find_child_by_attribute("name", "randomTransitionEventId"), file.m_evt_manager,
                            "If this event is received, the state machine chooses a random transition from among those available.\n\n"
                            "The event of the transition is ignored, but all other considerations of whether the transition should be taken are considered.");
            eventPickerEdit(obj.find_child_by_attribute("name", "transitionToNextHigherStateEventId"), file.m_evt_manager,
                            "If the event is received, the state machine chooses a state with the id higher than the m_currentStateId and do a transition to that state.\n\n"
                            "The event of the transition is ignored, but all other considerations of whether the transition should be taken are considered.\n"
                            "If no appropriate transition is found, an immediate transition will occur.");
            eventPickerEdit(obj.find_child_by_attribute("name", "transitionToNextLowerStateEventId"), file.m_evt_manager,
                            "If the event is received, the state machine chooses a state with the id lower than the m_currentStateId and do a transition to that state.\n\n"
                            "The event of the transition is ignored, but all other considerations of whether the transition should be taken are considered.\n"
                            "If no appropriate transition is found, an immediate transition will occur.");
            twoSepartors();
            variablePickerEdit(obj.find_child_by_attribute("name", "syncVariableIndex"), file.m_var_manager,
                               "We use variables to sync the start state of the state machine.\n\n"
                               "The value of this index currently must be initialized to the state id which the user wants the start state to be.\n"
                               "The state machine syncs it start state by setting the value of the start state to that of the variable during activate\n"
                               "and sets the variable value to the toState when the transition begins.");
            boolEdit(obj.find_child_by_attribute("name", "wrapAroundStateId"),
                     "This decides whether to transition when the m_currentStateId is maximum and m_transitionToNextHigherStateEventId is set\n"
                     "m_currentStateId is minimum and m_transitionToNextLowerStateEventId is set.\n\n"
                     "If this is set to false there would be no transition.\n"
                     "Otherwise there would be a transition but in the first case the next state id will be a lower than the current one\n"
                     "and in the second case the next state id would be higher than the current state id.");
            intScalarEdit(obj.find_child_by_attribute("name", "maxSimultaneousTransitions"), ImGuiDataType_S8,
                          "The number of transitions that can be active at once.\n\n"
                          "When a transition B interrupts another transition A, the state machine can continue playing A an input to B.\n"
                          "If a transition is triggered when there are already m_maxSimultaneousTransitions active transitions,\n"
                          "then the oldest transition will be immediately ended to make room for a new one.");
            enumEdit(obj.find_child_by_attribute("name", "startStateMode"), Hkx::e_hkbStateMachine_StartStateMode,
                     "How to set the start state.");
            enumEdit(obj.find_child_by_attribute("name", "selfTransitionMode"), Hkx::e_hkbStateMachine_StateMachineSelfTransitionMode,
                     "How to deal with self-transitions (when the state machine is transitioned to while still active).");
            refEdit(obj.find_child_by_attribute("name", "wildcardTransitions"), {"hkbStateMachineTransitionInfoArray"}, file,
                    "the list of transitions from any state (don't have a specific from state)");
            ImGui::EndTable();
        }

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
     UIMAPITEM(hkbStringEventPayload),
     UIMAPITEM(hkbStateMachine)};

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