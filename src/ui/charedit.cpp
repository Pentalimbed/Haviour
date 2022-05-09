#include "charedit.h"
#include "widgets.h"
#include "hkx/hkclass.inl"

#include <memory>

namespace Haviour
{
namespace Ui
{
CharEdit* CharEdit::getSingleton()
{
    static CharEdit edit;
    return std::addressof(edit);
}

void CharEdit::show()
{
}

void CharEdit::showPropList()
{
    ImGui::PushID("propedit");
    constexpr auto table_flag =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;

    bool  scroll_to_bottom = false;
    auto  file_manager     = Hkx::HkxFileManager::getSingleton();
    auto& current_file     = file_manager->m_char_file;

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Character Property"), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        ImGui::OpenPopup("Type Select");
    addTooltip("Add new character property");
    if (ImGui::BeginPopup("Type Select"))
    {
        for (auto data_type : Hkx::e_variableType)
            if ((data_type != "VARIABLE_TYPE_INVALID") && ImGui::Selectable(data_type.data()))
            {
                ImGui::CloseCurrentPopup();
                scroll_to_bottom = true;
                current_file.m_prop_manager.addEntry(Hkx::getVarTypeEnum(data_type));
                break;
            }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_HASHTAG))
        current_file.m_prop_manager.reindex();
    addTooltip("Reindex variables\nDiscard all variables marked obsolete");

    ImGui::InputText("Filter", &m_prop_filter), ImGui::SameLine();
    ImGui::Button(ICON_FA_QUESTION_CIRCLE);
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(g_color_invalid, "Invalid");
        ImGui::TextColored(g_color_bool, "Boolean");
        ImGui::TextColored(g_color_int, "Int8/16/32");
        ImGui::TextColored(g_color_float, "Real");
        ImGui::TextColored(g_color_attr, "Pointer");
        ImGui::TextColored(g_color_quad, "Vector/Quaternion");
        ImGui::EndTooltip();
    }
    ImGui::Separator();

    if (ImGui::BeginTable("##PropList", 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_WidthFixed, 36);
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        auto var_list = current_file.m_prop_manager.getEntryList();
        std::erase_if(var_list,
                      [=](auto& var) {
                          auto disp_name = std::format("{:3} {}", var.m_index, var.get<Hkx::PropName>().text().as_string()); // :3
                          return !(var.m_valid &&
                                   (m_prop_filter.empty() ||
                                    !std::ranges::search(disp_name, m_prop_filter, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty()));
                      });

        ImGuiListClipper clipper;
        clipper.Begin(var_list.size());
        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                auto& var = var_list[row_n];

                ImGui::TableNextColumn();

                auto var_type = var.get<Hkx::PropVarInfo>().getByName("type").text().as_string();
                ImGui::PushStyleColor(ImGuiCol_Text, getVarColor(var));
                ImGui::Text("%d", var.m_index);
                addTooltip(var_type);
                ImGui::PopStyleColor();

                ImGui::TableNextColumn();
                const bool is_selected = false;
                if (ImGui::Selectable(std::format("{}##{}", var.get<Hkx::PropName>().text().as_string(), var.m_index).c_str(), is_selected))
                {
                    m_prop_current = var;
                    ImGui::OpenPopup("Editing Varibale");
                }
                addTooltip("Click to edit");
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        // varEditPopup("Editing Varibale", m_prop_current, current_file);

        if (scroll_to_bottom)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndTable();
    }
    ImGui::PopID();
}

} // namespace Ui
} // namespace Haviour