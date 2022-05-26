#include "columnview.h"
#include "widgets.h"
#include "propedit.h"
#include "hkx/hkclass.inl"
#include "hkx/hkutils.h"

#include <memory>
#include <ranges>

#include <spdlog/spdlog.h>

namespace Haviour
{
namespace Ui
{
ColumnView* ColumnView::getSingleton()
{
    static ColumnView view;
    return std::addressof(view);
}
ColumnView::ColumnView()
{
    auto file_manager = Hkx::HkxFileManager::getSingleton();
    m_file_listener   = file_manager->appendListener(Hkx::kEventFileChanged, [=]() {
        if (file_manager->isCurrentFileReady())
            m_columns = {{}};
    });

    m_class_show = {Hkx::g_class_generators.begin(), Hkx::g_class_generators.end()};
    m_class_show.insert(Hkx::g_class_modifiers.begin(), Hkx::g_class_modifiers.end());
    m_class_show.emplace("hkbStateMachineStateInfo");
    m_class_show.emplace("hkbBlenderGeneratorChild");
    m_class_show.emplace("BSBoneSwitchGeneratorBoneData");

    m_class_color_map["hkbStateMachine"]          = g_color_bool;
    m_class_color_map["hkbStateMachineStateInfo"] = g_color_float;
    for (auto gen : Hkx::g_class_generators)
        m_class_color_map[gen.data()] = g_color_int;
    for (auto gen : Hkx::g_class_modifiers)
        m_class_color_map[gen.data()] = g_color_attr;
    m_class_color_map["hkbBlenderGeneratorChild"] =
        m_class_color_map["BSBoneSwitchGeneratorBoneData"] = g_color_quad;
}
ColumnView::~ColumnView()
{
    Hkx::HkxFileManager::getSingleton()->removeListener(Hkx::kEventFileChanged, m_file_listener);
}

void ColumnView::show()
{
    auto file_manager = Hkx::HkxFileManager::getSingleton();

    if (ImGui::Begin("Column View", &m_show, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        if (auto _file = file_manager->getCurrentFile(); _file && _file->getType() == Hkx::HkxFile::kBehaviour)
        {
            auto&       file               = *dynamic_cast<Hkx::BehaviourFile*>(_file);
            std::string root_state_machine = file.getRootStateMachine().data();

            if (ImGui::BeginTable("DisplayArea", 2, ImGuiTableFlags_Resizable))
            {
                ImGui::TableNextColumn();
                // Navigation
                if (ImGui::InputTextWithHint("Navigate to", "#0100", &m_nav_edit_str, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    spdlog::info("Navigating to {}", m_nav_edit_str);
                    std::vector<std::string> path = {};
                    getNavPath(m_nav_edit_str, root_state_machine, file, path);
                    if (!path.empty())
                    {
                        if (m_columns.size() < path.size())
                            m_columns.resize(path.size());
                        for (int i = 0; i < path.size(); ++i)
                            m_columns[i].m_selected.insert(path[i].data());
                    }
                    else
                        spdlog::warn("Failed to navigate to {}", m_nav_edit_str);
                }
                addTooltip("Press enter to navigate.");
                ImGui::Separator();
                ImGui::SliderFloat("Column Width", &m_col_width, 100.0f, 400.0f);
                ImGui::SliderFloat("Item Height", &m_item_height, 25.0f, 40.0f);
                ImGui::Checkbox("Align with Children", &m_align_child);

                ImGui::TableNextColumn();
                showColumns();

                ImGui::EndChild();

                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("No loaded file.");
    }
    ImGui::End();
}

void ColumnView::showColumns()
{
    struct Item
    {
        std::string id;
        size_t      parent_item_idx;
        size_t      height = 0;
        size_t      pos    = 0;
    };

    // Columns
    if (ImGui::BeginChild("Columns", {-FLT_MIN, -FLT_MIN}, false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        auto table_flag =
            ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
        if (!m_align_child)
            table_flag = table_flag | ImGuiTableFlags_ScrollY;

        auto        file_manager       = Hkx::HkxFileManager::getSingleton();
        auto&       file               = *dynamic_cast<Hkx::BehaviourFile*>(file_manager->getCurrentFile());
        std::string root_state_machine = file.getRootStateMachine().data();

        if (root_state_machine.empty() || !file.getObj(root_state_machine))
            ImGui::TextDisabled("Failed to get root generator object");
        else
        {
            // get the items
            std::vector<std::vector<Item>> items   = {{{root_state_machine, 0}}};
            size_t                         max_pos = 0;
            while (!items.back().empty())
            {
                if (items.size() > m_columns.size())
                    m_columns.push_back({});

                auto col_idx = items.size() - 1;
                items.push_back({});

                for (size_t i = 0; i < items[col_idx].size(); ++i)
                {
                    auto& item = items[col_idx][i];
                    if (m_columns[col_idx].m_selected.contains(item.id))
                    {
                        auto        obj       = file.getObj(item.id);
                        std::string disp_name = obj.getByName("name").text().as_string();
                        if (disp_name.empty())
                            disp_name = obj.attribute("name").as_string();

                        items.back().push_back({"> " + disp_name, i});

                        std::vector<std::string> reflist = {};
                        file.getRefedObjs(item.id, reflist);
                        for (auto& ref : reflist)
                            if (m_class_show.contains(file.getObj(ref).attribute("class").as_string()))
                                items.back().push_back({ref, i});
                    }
                }
            }
            items.pop_back();
            // remove excessive columns
            if (m_columns.size() > items.size())
                m_columns.resize(items.size());

            if (m_align_child)
            {
                // DP
                // back propagation calculate height
                for (size_t col_idx = items.size() - 1; col_idx > 0; --col_idx)
                    for (auto& item : items[col_idx])
                        items[col_idx - 1][item.parent_item_idx].height += std::max(item.height, 1ull);
                // forward position
                for (size_t col_idx = 1; col_idx < items.size(); ++col_idx)
                    for (size_t item_idx = 0; item_idx < items[col_idx].size(); ++item_idx)
                    {
                        auto& item = items[col_idx][item_idx];
                        item.pos   = items[col_idx - 1][item.parent_item_idx].pos;
                        if (item_idx)
                            item.pos = std::max(item.pos,
                                                items[col_idx][item_idx - 1].pos + std::max(items[col_idx][item_idx - 1].height, 1ull));
                        max_pos = std::max(max_pos, item.pos);
                    }
            }

            // draw stuff
            for (size_t col_idx = 0; col_idx < items.size(); ++col_idx)
            {
                auto& column = m_columns[col_idx];
                if (ImGui::BeginTable(std::format("{}", col_idx).c_str(), 2, table_flag,
                                      {m_col_width, m_align_child ? m_item_height * (max_pos + 1) + 2 : -FLT_MIN}))
                {
                    for (size_t item_idx = 0; item_idx < items[col_idx].size(); ++item_idx)
                    {
                        // get to pos
                        auto& item = items[col_idx][item_idx];
                        if (m_align_child)
                            while (ImGui::TableGetRowIndex() + 1 < item.pos)
                            {
                                ImGui::TableNextRow(0, m_item_height);
                                if (item_idx && (items[col_idx][item_idx - 1].parent_item_idx == item.parent_item_idx))
                                {
                                    ImGui::TableNextColumn();
                                    ImGui::TextDisabled("|");
                                }
                            }

                        ImGui::TableNextRow(0, m_item_height);

                        if (item.id.starts_with("#")) // obj
                        {
                            auto        obj         = file.getObj(item.id);
                            auto        obj_class   = obj.attribute("class").as_string();
                            bool        is_selected = column.m_selected.contains(item.id);
                            bool        is_editing  = PropEdit::getSingleton()->getEditObj() == item.id;
                            std::string disp_name   = obj.getByName("name").text().as_string();
                            if (disp_name.empty())
                                disp_name = obj.attribute("name").as_string();

                            ImGui::PushID(item_idx);

                            ImGui::TableNextColumn();
                            if (is_editing)
                                ImGui::BeginDisabled();
                            if (ImGui::Button(ICON_FA_PEN))
                                PropEdit::getSingleton()->setObject(item.id);
                            if (is_editing)
                                ImGui::EndDisabled();
                            addTooltip("Edit");

                            ImGui::TableNextColumn();
                            if (m_class_color_map.contains(obj_class))
                                ImGui::PushStyleColor(ImGuiCol_Text, m_class_color_map.at(obj_class).Value);
                            if (ImGui::Selectable(disp_name.c_str(), is_selected))
                            {
                                if (is_selected)
                                    column.m_selected.erase(item.id);
                                else if (ImGui::IsKeyDown(ImGuiKey_ModShift)) // Multiselect
                                    column.m_selected.emplace(item.id);
                                else
                                    column.m_selected = {item.id};
                            }
                            addTooltip(disp_name.c_str());
                            if (m_class_color_map.contains(obj_class))
                                ImGui::PopStyleColor();

                            ImGui::PopID();
                        }
                        else
                        {
                            ImGui::TableNextColumn();
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            ImGui::TextUnformatted(item.id.c_str());
                            addTooltip(item.id.c_str());
                        }
                    }
                    if (m_align_child)
                        while (ImGui::TableGetRowIndex() + 1 < max_pos)
                            ImGui::TableNextRow(0, m_item_height);

                    ImGui::EndTable();
                }
                ImGui::SameLine();
            }
        }
    }
}

} // namespace Ui
} // namespace Haviour