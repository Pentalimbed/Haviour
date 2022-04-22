#include "classes.h"
#include "widgets.h"
#include "hkx/hkclass.h"

#include <iostream>
#include <imgui.h>

namespace Haviour
{
namespace Ui
{

void show_hkbBehaviorGraph(pugi::xml_node obj);
void show_hkbVariableBindingSet(pugi::xml_node obj);

const std::unordered_map<std::string_view, std::function<void(pugi::xml_node)>> g_class_ui_map =
    {{"hkbBehaviorGraph", show_hkbBehaviorGraph},
     {"hkbVariableBindingSet", show_hkbVariableBindingSet}};

void showEditUi(pugi::xml_node hkobject)
{
    auto hkclass = hkobject.attribute("class").as_string();
    if (g_class_ui_map.contains(hkclass))
    {
        g_class_ui_map.at(hkclass)(hkobject);
    }
    else
    {
        ImGui::TextDisabled("The class %s is currently not supported.", hkclass);
    }
}

void show_hkbBehaviorGraph(pugi::xml_node obj)
{
    if (ImGui::BeginTable("hkbBehaviorGraph", 4, ImGuiTableFlags_SizingStretchProp))
    {
        stringEdit(obj.find_child_by_attribute("name", "name"));
        intEdit(obj.find_child_by_attribute("name", "userData"));
        refEdit(obj.find_child_by_attribute("name", "variableBindingSet"), {"hkbVariableBindingSet"});
        enumEdit(obj.find_child_by_attribute("name", "variableMode"), Hkx::e_variableMode);
        refEdit(obj.find_child_by_attribute("name", "rootGenerator"), {"hkbStateMachine"});
        refEdit(obj.find_child_by_attribute("name", "data"), {"hkbBehaviorGraphData"});
        ImGui::EndTable();
    }
}

void show_hkbVariableBindingSet(pugi::xml_node obj)
{
    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_binding = {};
    if (obj_cache != obj)
        edit_binding = {};
    obj_cache = obj; // workaround to prevent crashing between files

    auto bindings = obj.find_child_by_attribute("name", "bindings");
    if (ImGui::BeginTable("hkbBehaviorGraph", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("bindings", ImGuiTableColumnFlags_WidthFixed, 200.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();

        ImGui::Text("Bindings"), ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        {
            appendXmlString(bindings, Hkx::g_def_binding);
            edit_binding = bindings;
        }
        addToolTip("Add new binding");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_MINUS_CIRCLE) && edit_binding)
        {
            bindings.remove_child(edit_binding);
            edit_binding = {};
        }
        addToolTip("Remove currently editing binding.");

        if (ImGui::BeginListBox("##refs", ImVec2(-FLT_MIN, -FLT_MIN)))
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
            if (ImGui::BeginTable("binding", 2, ImGuiTableFlags_SizingStretchProp))
            {
                stringEdit(edit_binding.find_child_by_attribute("name", "memberPath"));
                variableEdit(edit_binding.find_child_by_attribute("name", "variableIndex"));
                intEdit(edit_binding.find_child_by_attribute("name", "bitIndex"));
                enumEdit(edit_binding.find_child_by_attribute("name", "bindingType"), Hkx::e_bindingType);
                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("No binding selected.");

        ImGui::EndTable();
    }
}

} // namespace Ui
} // namespace Haviour