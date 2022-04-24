#include "listview.h"
#include "propedit.h"
#include "global.h"

#include <memory>

#include <imgui.h>
#include <extern/imgui_stdlib.h>
#include <extern/font_awesome_5.h>

namespace Haviour
{
namespace Ui
{

ListView* ListView::getSingleton()
{
    static ListView listview;
    return std::addressof(listview);
}

ListView::ListView()
{
    m_file_listener = Hkx::HkxFile::getSingleton()->appendListener(Hkx::kEventLoadFile, [=]() { m_current_class_idx = 0; updateCache(); });
}
ListView::~ListView()
{
    Hkx::HkxFile::getSingleton()->removeListener(Hkx::kEventLoadFile, m_file_listener);
}

void ListView::show()
{
    if (ImGui::Begin("List View", &g_show_list_view))
    {
        auto& hkx_file = *Hkx::HkxFile::getSingleton();
        if (hkx_file.isLoaded())
        {
            if (ImGui::InputText("Filter", &m_filter_string))
                updateCache();
            ImGui::SameLine();
            if (ImGui::Checkbox("Filter ID", &m_filter_id))
                updateCache();

            auto class_list = hkx_file.getClasses();
            if (m_current_class_idx > class_list.size())
                m_current_class_idx = 0;
            const char* combo_text = class_list[m_current_class_idx].data();

            if (ImGui::BeginCombo("Class", combo_text, ImGuiComboFlags_HeightLargest))
            {
                for (int i = 0; i < class_list.size(); i++)
                {
                    const bool is_selected = (m_current_class_idx == i);
                    if (ImGui::Selectable(class_list[i].data(), is_selected))
                    {
                        m_current_class_idx = i;
                        updateCache();
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }

            ImGui::SameLine();
            if (ImGui::BeginPopup("Add Class"))
            {
                for (auto class_name : class_list)
                    if ((class_name != "All") && ImGui::Selectable(class_name.data()))
                    {
                        // TODO add item
                        ImGui::CloseCurrentPopup();
                    }
                ImGui::EndPopup();
            }
            if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
                if (m_current_class_idx)
                {
                    // TODO add item
                }
                else
                    ImGui::OpenPopup("Add Class");
            addToolTip("Add new object");

            ImGui::Separator();

            drawTable();
        }
        else
        {
            ImGui::TextDisabled("No loaded file.");
        }
    }
    ImGui::End();
}

void ListView::updateCache()
{
    auto& hkx_file   = *Hkx::HkxFile::getSingleton();
    auto  class_list = hkx_file.getClasses();
    if (m_current_class_idx)
        m_cache_list = hkx_file.getNodeClassList(class_list[m_current_class_idx]);
    else
        m_cache_list = hkx_file.getFullNodeList();

    if (!m_filter_string.empty())
    {
        if (m_filter_id)
            std::erase_if(m_cache_list, [&](pugi::xml_node& node) {
                return std::ranges::search(
                           std::string_view(node.attribute("name").as_string()),
                           m_filter_string,
                           [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); })
                    .empty();
            });
        else
            std::erase_if(m_cache_list, [&](pugi::xml_node& node) {
                return std::ranges::search(
                           std::string_view(node.find_child_by_attribute("hkparam", "name", "name").text().as_string()),
                           m_filter_string,
                           [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); })
                    .empty();
            });
    }


    if (m_current_sort_spec.SpecsCount)
        std::ranges::sort(m_cache_list, [&](const pugi::xml_node& a, const pugi::xml_node& b) {
            for (int n = 0; n < m_current_sort_spec.SpecsCount; n++)
            {
                const auto sort_spec = m_current_sort_spec.Specs[n];
                int        delta     = 0;
                switch (sort_spec.ColumnUserID)
                {
                    case kColId:
                        delta = strcmp(a.attribute("name").as_string(), b.attribute("name").as_string());
                        break;
                    case kColName:
                        delta = strcmp(a.find_child_by_attribute("hkparam", "name", "name").text().as_string(),
                                       b.find_child_by_attribute("hkparam", "name", "name").text().as_string());
                        break;
                    case kColClass:
                        delta = strcmp(a.attribute("class").as_string(), b.attribute("class").as_string());
                        break;
                    default:
                        break;
                }
                if (delta)
                    return (delta < 0) != (sort_spec.SortDirection == ImGuiSortDirection_Ascending);
            }
            return true;
        });
}

void ListView::drawTable()
{
    constexpr auto table_flag =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
    if (ImGui::BeginTable("Objects", 4, table_flag))
    {
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0, kColAction);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 0, kColId);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 0, kColName);
        ImGui::TableSetupColumn("Class", ImGuiTableColumnFlags_WidthFixed, 0, kColClass);
        ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
            if (sorts_specs->SpecsDirty)
            {
                m_current_sort_spec = *sorts_specs;
                updateCache();
                sorts_specs->SpecsDirty = false;
            }

        ImGuiListClipper clipper;
        clipper.Begin(m_cache_list.size());
        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                // Display a data item
                auto hkobject = m_cache_list[row_n];

                ImGui::PushID(hkobject.attribute("name").as_string());
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Button(ICON_FA_PEN_SQUARE)) // Edit
                    PropEdit::getSingleton()->setObject(hkobject);
                addToolTip("Edit");
                ImGui::SameLine();
                ImGui::Button(ICON_FA_TRASH); // Delete
                addToolTip("Delete");
                ImGui::TableNextColumn();
                copyableText(hkobject.attribute("name").as_string());
                ImGui::TableNextColumn();
                copyableText(hkobject.find_child_by_attribute("hkparam", "name", "name").text().as_string());
                ImGui::TableNextColumn();
                copyableText(hkobject.attribute("class").as_string());
                ImGui::PopID();
            }

        ImGui::EndTable();
    }
}
} // namespace Ui
} // namespace Haviour