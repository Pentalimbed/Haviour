#include "listview.h"
#include "propedit.h"
#include "widgets.h"
#include "hkx/hkclass.inl"

#include <execution>
#include <ranges>

#include <extern/imgui_stdlib.h>
#include <extern/font_awesome_5.h>

namespace Haviour
{
namespace Ui
{
ListView* ListView::getSingleton()
{
    static ListView view;
    return std::addressof(view);
}
ListView::ListView()
{
    auto file_manager = Hkx::HkxFileManager::getSingleton();
    m_file_listener   = file_manager->appendListener(Hkx::kEventFileChanged, [=]() { m_current_class_idx = 0; updateCache(); });
    m_obj_listener    = file_manager->appendListener(Hkx::kEventObjChanged, [=]() { updateCache(); });
}
ListView::~ListView()
{
    auto file_manager = Hkx::HkxFileManager::getSingleton();
    file_manager->removeListener(Hkx::kEventFileChanged, m_file_listener);
    file_manager->removeListener(Hkx::kEventObjChanged, m_obj_listener);
}

void ListView::show()
{
    if (ImGui::Begin("List View", &m_show))
    {
        auto file_manager = Hkx::HkxFileManager::getSingleton();
        if (auto file = file_manager->getCurrentFile(); file)
        {
            auto& hkxfile = *file;
            if (ImGui::InputText("Filter", &m_filter))
                updateCache();
            ImGui::SameLine();
            if (ImGui::Checkbox("Filter ID", &m_do_filter_id))
                updateCache();

            std::vector<std::string> class_list;
            hkxfile.getClasses(class_list);
            m_current_class_idx    = std::clamp(0uLL, m_current_class_idx, class_list.size() - 1);
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
            if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
                if (m_current_class_idx)
                    PropEdit::getSingleton()->setObject(hkxfile.addObj(class_list[m_current_class_idx]));
                else
                    ImGui::OpenPopup("Add Class");
            addTooltip("Add new object");
            if (ImGui::BeginPopup("Add Class"))
            {
                auto&                         class_info      = Hkx::getClassInfo();
                auto                          temp_view       = Hkx::getClassDefaultMap() | std::views::keys;
                std::vector<std::string_view> all_avail_class = {temp_view.begin(), temp_view.end()};
                std::ranges::sort(all_avail_class);
                for (auto class_name : all_avail_class)
                {
                    if (ImGui::Selectable(class_name.data()))
                    {
                        PropEdit::getSingleton()->setObject(hkxfile.addObj(class_name));
                        ImGui::CloseCurrentPopup();
                        break;
                    }
                    if (class_info.contains(class_name))
                        addTooltip(class_info.at(class_name).data());
                }
                ImGui::EndPopup();
            }

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

void ListView::updateCache(bool sort_only)
{
    if (!Hkx::HkxFileManager::getSingleton()->getCurrentFile())
        return;
    auto& hkxfile = *Hkx::HkxFileManager::getSingleton()->getCurrentFile();
    if (!sort_only)
    {
        m_cache_list.clear();
        if (m_current_class_idx)
        {
            std::vector<std::string> class_list;
            hkxfile.getClasses(class_list);
            hkxfile.getObjListByClass(class_list[m_current_class_idx], m_cache_list);
        }
        else
            hkxfile.getObjList(m_cache_list);

        if (!m_filter.empty())
            if (m_do_filter_id)
                std::erase_if(m_cache_list, [&](const std::string& id) {
                    return std::ranges::search(
                               std::string_view(hkxfile.getObj(id).attribute("name").as_string()),
                               m_filter,
                               [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); })
                        .empty();
                });
            else
                std::erase_if(m_cache_list, [&](const std::string& id) {
                    return std::ranges::search(
                               std::string_view(hkxfile.getObj(id).find_child_by_attribute("hkparam", "name", "name").text().as_string()),
                               m_filter,
                               [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); })
                        .empty();
                });
    }
    if (m_current_sort_spec.SpecsCount)
        std::ranges::sort(
            m_cache_list,
            [&](const std::string& a_str, const std::string& b_str) {
                for (int n = 0; n < m_current_sort_spec.SpecsCount; n++)
                {
                    auto a = hkxfile.getObj(a_str);
                    auto b = hkxfile.getObj(b_str);

                    const auto sort_spec = m_current_sort_spec.Specs[n];
                    int        delta     = 0;
                    switch (sort_spec.ColumnUserID)
                    {
                        case kColId:
                            delta = strcmp(a.attribute("name").as_string(), b.attribute("name").as_string());
                            break;
                        case kColName:
                            delta = strcmp(getObjContextName(a), getObjContextName(b));
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
    auto& hkxfile = *Hkx::HkxFileManager::getSingleton()->getCurrentFile();

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
                updateCache(true);
                sorts_specs->SpecsDirty = false;
            }

        std::string_view marked_delete = {};
        ImGuiListClipper clipper;
        clipper.Begin(m_cache_list.size());
        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                // Display a data item
                std::string_view id  = m_cache_list[row_n];
                auto             obj = hkxfile.getObj(id);

                ImGui::PushID(id.data());
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Button(ICON_FA_PEN)) // Edit
                    PropEdit::getSingleton()->setObject(id);
                addTooltip("Edit");
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_TRASH)) // Delete
                    marked_delete = id;
                addTooltip("Delete");
                ImGui::TableNextColumn();
                copyableText(id.data());
                ImGui::TableNextColumn();
                copyableText(getObjContextName(obj));
                ImGui::TableNextColumn();
                copyableText(obj.attribute("class").as_string());
                ImGui::PopID();
            }

        if (!marked_delete.empty())
            hkxfile.delObj(marked_delete);

        ImGui::EndTable();
    }
}
} // namespace Ui
} // namespace Haviour