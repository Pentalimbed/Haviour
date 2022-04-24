#include "varlist.h"
#include "widgets.h"
#include "hkx/hkclass.h"

#include <memory>

#include <imgui.h>

namespace Haviour
{
namespace Ui
{
VarList* VarList::getSingleton()
{
    static VarList list;
    return std::addressof(list);
}

VarList::VarList()
{
    m_file_listener = Hkx::HkxFile::getSingleton()->appendListener(Hkx::kEventLoadFile, [=]() { m_editing_var = {}, m_editing_evt = {}, m_editing_charprop = {}; });
}
VarList::~VarList()
{
    Hkx::HkxFile::getSingleton()->removeListener(Hkx::kEventLoadFile, m_file_listener);
}

void VarList::show()
{
    if (ImGui::Begin("Variable/Event List", &g_show_var_evt_list, ImGuiWindowFlags_NoScrollbar))
    {
        auto hkxfile = Hkx::HkxFile::getSingleton();
        if (hkxfile->isLoaded())
        {
            if (ImGui::BeginTable("varlisttbl", 2))
            {
                ImGui::TableNextColumn();
                ImGui::RadioButton("Variables", &m_show_var_charprop_attr, 0);
                ImGui::RadioButton("Character Properties", &m_show_var_charprop_attr, 1);
                addToolTip("This edits character properties within the behaviour file itself,\nnot the ones in the actual character.");
                ImGui::RadioButton("Attributes", &m_show_var_charprop_attr, 2);
                ImGui::Separator();
                switch (m_show_var_charprop_attr)
                {
                    case 0: showVarList(); break;
                    case 1: showNameInfoList("Character Properties", hkxfile->m_charprop_manager, m_editing_charprop, m_charprop_filter, [=]() { this->showCharPropEditUi(this->m_editing_charprop); }); break;
                    default: ImGui::TextDisabled("Not supported yet.");
                }
                ImGui::TableNextColumn();
                showNameInfoList("Events", hkxfile->m_evt_manager, m_editing_evt, m_evt_filter, [=]() { this->showEventEditUi(this->m_editing_evt); });
                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::TextDisabled("No loaded file.");
        }
    }
    ImGui::End();
}

void VarList::showVarList()
{
    ImGui::PushID("varedit");
    constexpr auto table_flag =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
    const static auto color_invalid = ImColor(0.2f, 0.2f, 0.2f).Value;
    const static auto color_bool    = ImColor(0xFF, 0x9C, 0x83).Value;
    const static auto color_int     = ImColor(0x07, 0xd2, 0xd9).Value;
    const static auto color_float   = ImColor(0xF6, 0x6f, 0x9a).Value;
    const static auto color_attr    = ImColor(0x9d, 0x00, 0x1c).Value;
    const static auto color_quad    = ImColor(0x5a, 0xe6, 0xb8).Value;

    auto hkxfile = Hkx::HkxFile::getSingleton();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Variables"), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        ImGui::OpenPopup("Type Select");
    addToolTip("Add new variable");
    bool scroll_to_bottom = false;
    if (ImGui::BeginPopup("Type Select"))
    {
        for (auto data_type : Hkx::e_variableType)
            if ((data_type != "VARIABLE_TYPE_INVALID") && ImGui::Selectable(data_type.data()))
            {
                ImGui::CloseCurrentPopup();
                scroll_to_bottom = true;
                hkxfile->m_var_manager.addVariable(Hkx::getVarTypeEnum(data_type));
                break;
            }
        ImGui::EndPopup();
    }

    ImGui::InputText("Filter", &m_var_filter), ImGui::SameLine();
    if (ImGui::BeginPopup("Color scheme"))
    {
        ImGui::TextColored(color_invalid, "Invalid");
        ImGui::TextColored(color_bool, "Boolean");
        ImGui::TextColored(color_int, "Int8/16/32");
        ImGui::TextColored(color_float, "Real");
        ImGui::TextColored(color_attr, "Pointer");
        ImGui::TextColored(color_quad, "Vector/Quaternion");
        ImGui::EndPopup();
    }
    if (ImGui::Button(ICON_FA_QUESTION))
        ImGui::OpenPopup("Color scheme");
    addToolTip("Coloring");

    ImGui::Separator();

    if (ImGui::BeginTable("##VarList", 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_WidthFixed, 36);
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        auto var_list = hkxfile->m_var_manager.getVariableList();

        std::vector<Hkx::Variable> var_filtered;
        var_filtered.reserve(var_list.size());
        std::ranges::copy_if(var_list, std::back_inserter(var_filtered),
                             [=](auto& var) {
                                 auto var_disp_name = std::format("{:3} {}", var.idx, var.name.text().as_string()); // :3
                                 return !var.invalid &&
                                     (m_var_filter.empty() ||
                                      !std::ranges::search(var_disp_name, m_var_filter, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty());
                             });

        ImGuiListClipper clipper;
        clipper.Begin(var_filtered.size());
        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                auto& var = var_filtered[row_n];

                ImGui::TableNextColumn();

                auto var_type_enum = Hkx::getVarTypeEnum(var.getType());
                if (var_type_enum < 0)
                    ImGui::PushStyleColor(ImGuiCol_Text, color_invalid);
                else if (var_type_enum < 1)
                    ImGui::PushStyleColor(ImGuiCol_Text, color_bool);
                else if (var_type_enum < 4)
                    ImGui::PushStyleColor(ImGuiCol_Text, color_int);
                else if (var_type_enum < 5)
                    ImGui::PushStyleColor(ImGuiCol_Text, color_float);
                else if (var_type_enum < 6)
                    ImGui::PushStyleColor(ImGuiCol_Text, color_attr);
                else
                    ImGui::PushStyleColor(ImGuiCol_Text, color_quad);
                ImGui::Text("%d", var.idx);
                addToolTip(var.getType().data());
                ImGui::PopStyleColor();

                ImGui::TableNextColumn();
                const bool is_selected = false;
                if (ImGui::Selectable(var.name.text().as_string(), is_selected))
                {
                    m_editing_var = var;
                    ImGui::OpenPopup("Editing Varibale");
                }
                addToolTip("Click to edit");
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        if (ImGui::BeginPopup("Editing Varibale", ImGuiWindowFlags_AlwaysAutoResize))
        {
            showVarEditUi(m_editing_var);
            ImGui::EndPopup();
        }

        if (scroll_to_bottom)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndTable();
    }
    ImGui::PopID();
}

void VarList::showVarEditUi(Hkx::Variable var)
{
    if (ImGui::BeginTable("varedit", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(600.0, 0.0)))
    {
        stringEdit(var.name, "name");
        if (ImGui::Button(ICON_FA_TRASH))
        {
            auto name     = var.name.text().as_string();
            auto ref_node = Hkx::HkxFile::getSingleton()->getFirstVariableRef(var.idx);
            if (ref_node)
            {
                spdlog::warn("This variable is still reference by {} and potentially more object.\nID copied to clipboard.", ref_node.attribute("name").as_string());
                ImGui::SetClipboardText(ref_node.attribute("name").as_string());
            }
            else
            {
                Hkx::HkxFile::getSingleton()->m_var_manager.delVariable(var.idx);
                spdlog::info("Variable {} deleted!", name);
                ImGui::CloseCurrentPopup();
                ImGui::EndTable();
                return;
            }
        }
        addToolTip("Delete\nIf it is still referenced, the variable won't delete\nand one of its referencing object will be copied to clipboard.");

        auto var_type_enum = Hkx::getVarTypeEnum(var.getType());
        switch (var_type_enum)
        {
            case Hkx::VARIABLE_TYPE_BOOL:
                boolEdit(var.value);
                break;
            case Hkx::VARIABLE_TYPE_INT8:
                sliderIntEdit(var.value, INT8_MIN, INT8_MAX);
                break;
            case Hkx::VARIABLE_TYPE_INT16:
                sliderIntEdit(var.value, INT16_MIN, INT16_MAX);
                break;
            case Hkx::VARIABLE_TYPE_INT32:
                intEdit(var.value);
                break;
            case Hkx::VARIABLE_TYPE_REAL:
                convertFloatEdit(var.value);
                break;
            case Hkx::VARIABLE_TYPE_POINTER:
                ImGui::TableNextColumn();
                ImGui::TextDisabled("Currently not supported");
                ImGui::TableNextColumn();
                break;
            case Hkx::VARIABLE_TYPE_VECTOR3:
                ImGui::TableNextColumn();
                ImGui::InputFloat3("value", Hkx::HkxFile::getSingleton()->m_var_manager.getQuadValue(var.value.text().as_uint()).data(), "%.6f");
                ImGui::TableNextColumn();
                break;
            case Hkx::VARIABLE_TYPE_VECTOR4:
            case Hkx::VARIABLE_TYPE_QUATERNION:
                ImGui::TableNextColumn();
                ImGui::InputFloat4("value", Hkx::HkxFile::getSingleton()->m_var_manager.getQuadValue(var.value.text().as_uint()).data(), "%.6f");
                ImGui::TableNextColumn();
                break;
            default:
                ImGui::TableNextColumn();
                ImGui::TextColored(ImColor(1.0f, 0.3f, 0.3f, 1.0f), "Invalid value type!");
                ImGui::TableNextColumn();
                break;
        }

        auto role_node = var.info.find_child_by_attribute("name", "role").first_child();
        enumEdit(role_node.find_child_by_attribute("name", "role"), Hkx::e_hkbRoleAttribute_Role);
        flagEdit(role_node.find_child_by_attribute("name", "flags"), Hkx::f_hkbRoleAttribute_roleFlags);

        ImGui::EndTable();
    }
}

void VarList::showNameInfoList(std::string_view title, Hkx::NameInfoManger& manager, Hkx::NameInfo& selection, std::string& filter, std::function<void()> popup)
{
    ImGui::PushID(title.data());
    constexpr auto table_flag =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(title.data()), ImGui::SameLine();
    bool scroll_to_bottom = false;
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
    {
        manager.addEntry();
        scroll_to_bottom = true;
    }

    addToolTip("Add new entry");

    ImGui::InputText("Filter", &filter);

    ImGui::Separator();

    if (ImGui::BeginTable(std::format("##List_{}", title).c_str(), 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_WidthFixed, 36);
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        auto nameinfo_list = manager.getEntryList();

        std::vector<Hkx::NameInfo> nameinfo_filtered;
        nameinfo_filtered.reserve(nameinfo_list.size());
        std::ranges::copy_if(nameinfo_list, std::back_inserter(nameinfo_filtered),
                             [=](auto& ninfo) {
                                 auto disp_name = std::format("{:3} {}", ninfo.idx, ninfo.name.text().as_string()); // :3
                                 return !ninfo.invalid &&
                                     (filter.empty() ||
                                      !std::ranges::search(disp_name, filter, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty());
                             });

        ImGuiListClipper clipper;
        clipper.Begin(nameinfo_filtered.size());
        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                auto& ninfo = nameinfo_filtered[row_n];

                ImGui::TableNextColumn();
                ImGui::Text("%d", ninfo.idx);

                ImGui::TableNextColumn();
                const bool is_selected = false;
                if (ImGui::Selectable(ninfo.name.text().as_string(), is_selected))
                {
                    selection = ninfo;
                    ImGui::OpenPopup("Editing Varibale");
                }
                addToolTip("Click to edit");
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }

        if (ImGui::BeginPopup("Editing Varibale", ImGuiWindowFlags_AlwaysAutoResize))
        {
            popup();
            ImGui::EndPopup();
        }

        if (scroll_to_bottom)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndTable();
    }
    ImGui::PopID();
}

void VarList::showEventEditUi(Hkx::NameInfo ninfo)
{
    if (ImGui::BeginTable("eventedit", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(300.0, 0.0)))
    {
        stringEdit(ninfo.name, "name");
        if (ImGui::Button(ICON_FA_TRASH))
        {
            auto name     = ninfo.name.text().as_string();
            auto ref_node = Hkx::HkxFile::getSingleton()->getFirstEventRef(ninfo.idx);
            if (ref_node)
            {
                spdlog::warn("This variable is still reference by {} and potentially more object.\nID copied to clipboard.", ref_node.attribute("name").as_string());
                ImGui::SetClipboardText(ref_node.attribute("name").as_string());
            }
            else
            {
                Hkx::HkxFile::getSingleton()->m_evt_manager.delEntry(ninfo.idx);
                spdlog::info("Variable {} deleted!", name);
                ImGui::CloseCurrentPopup();
                ImGui::EndTable();
                return;
            }
        }
        addToolTip("Delete\nIf it is still referenced, the variable won't delete\nand one of its referencing object will be copied to clipboard.");

        flagEdit(ninfo.info.find_child_by_attribute("name", "flags"), Hkx::f_hkbEventInfo_Flags);

        ImGui::EndTable();
    }
}

void VarList::showCharPropEditUi(Hkx::NameInfo ninfo)
{
    if (ImGui::BeginTable("charpropedit", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(300.0, 0.0)))
    {
        stringEdit(ninfo.name, "name");
        if (ImGui::Button(ICON_FA_TRASH))
        {
            auto name     = ninfo.name.text().as_string();
            auto ref_node = Hkx::HkxFile::getSingleton()->getFirstEventRef(ninfo.idx);
            if (ref_node)
            {
                spdlog::warn("This variable is still reference by {} and potentially more object.\nID copied to clipboard.", ref_node.attribute("name").as_string());
                ImGui::SetClipboardText(ref_node.attribute("name").as_string());
            }
            else
            {
                Hkx::HkxFile::getSingleton()->m_charprop_manager.delEntry(ninfo.idx);
                spdlog::info("Variable {} deleted!", name);
                ImGui::CloseCurrentPopup();
                ImGui::EndTable();
                return;
            }
        }
        addToolTip("Delete\nIf it is still referenced, the variable won't delete\nand one of its referencing object will be copied to clipboard.");

        auto role_node = ninfo.info.find_child_by_attribute("name", "role").first_child();
        enumEdit(role_node.find_child_by_attribute("name", "role"), Hkx::e_hkbRoleAttribute_Role);
        flagEdit(role_node.find_child_by_attribute("name", "flags"), Hkx::f_hkbRoleAttribute_roleFlags);

        ImGui::EndTable();
    }
}
} // namespace Ui
} // namespace Haviour