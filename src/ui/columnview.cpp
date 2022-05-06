#include "columnview.h"
#include "widgets.h"
#include "propedit.h"
#include "hkx/hkclass.inl"
#include "hkx/hkutils.h"

#include <memory>

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
        if (file_manager->isFileSelected())
            m_columns.push_back({});
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
    constexpr auto table_flag = ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
    auto file_manager = Hkx::HkxFileManager::getSingleton();
    if (ImGui::Begin("Column View", &m_show, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (file_manager->isFileSelected())
        {
            auto&       file               = file_manager->getCurrentFile();
            std::string root_state_machine = file.getRootStateMachine().data();

            if (ImGui::InputText("Navigate to object", &m_nav_edit_str, ImGuiInputTextFlags_EnterReturnsTrue))
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

            if (root_state_machine.empty() || !file.getObj(root_state_machine))
                ImGui::TextDisabled("Failed to get root generator object");
            else
            {
                std::vector<std::string> reflist    = {root_state_machine};
                size_t                   column_idx = 0;
                for (auto& column : m_columns)
                {
                    if (ImGui::BeginTable(std::format("{}", column_idx).c_str(), 2, table_flag, {200.0f, -FLT_MIN}))
                    {
                        for (auto& ref : reflist)
                        {
                            if (ref.starts_with('#'))
                            {
                                auto ref_obj   = file.getObj(ref);
                                auto ref_class = ref_obj.attribute("class").as_string();

                                if (m_class_show.contains(ref_class))
                                {
                                    bool        is_selected = column.m_selected.contains(ref);
                                    std::string disp_name   = ref_obj.getByName("name").text().as_string();
                                    if (disp_name.empty())
                                        disp_name = ref_obj.attribute("name").as_string();

                                    ImGui::PushID(disp_name.c_str());
                                    ImGui::TableNextColumn();
                                    bool is_editing = PropEdit::getSingleton()->getEditObj() == ref;
                                    if (is_editing)
                                        ImGui::BeginDisabled();
                                    if (ImGui::Button(ICON_FA_PEN))
                                        PropEdit::getSingleton()->setObject(ref);
                                    if (is_editing)
                                        ImGui::EndDisabled();
                                    addTooltip("Edit");

                                    ImGui::TableNextColumn();

                                    if (m_class_color_map.contains(ref_class))
                                        ImGui::PushStyleColor(ImGuiCol_Text, m_class_color_map.at(ref_class).Value);
                                    if (ImGui::Selectable(disp_name.c_str(), is_selected))
                                    {
                                        if (is_selected)
                                            column.m_selected.erase(ref);
                                        else
                                            column.m_selected.emplace(ref);
                                    }
                                    if (m_class_color_map.contains(ref_class))
                                        ImGui::PopStyleColor();

                                    addTooltip(disp_name.c_str());
                                    ImGui::PopID();
                                }
                            }
                            else
                            {
                                ImGui::TableNextColumn();
                                ImGui::TableNextColumn();
                                ImGui::TextUnformatted(ref.c_str());
                                addTooltip(ref.c_str());
                            }
                        }
                        ImGui::EndTable();
                    }
                    ImGui::SameLine();

                    auto reflist_copy = reflist;
                    reflist.clear();
                    for (auto& ref : reflist_copy)
                        if (column.m_selected.contains(ref))
                        {
                            auto        ref_obj   = file.getObj(ref);
                            std::string disp_name = ref_obj.getByName("name").text().as_string();
                            if (disp_name.empty())
                                disp_name = ref_obj.attribute("name").as_string();

                            reflist.push_back("> " + disp_name);
                            file.getRefedObjs(ref, reflist);
                            if (reflist.back() == "> " + disp_name)
                                reflist.pop_back(); // empty ones don't get shown
                        }

                    if (reflist.empty())
                        break;

                    ++column_idx;
                }
                if (column_idx >= m_columns.size()) // latest column needed
                {
                    m_columns.push_back({});
                    ImGui::SetScrollHereX(1.0f);
                }
                else
                    m_columns.erase(m_columns.begin() + column_idx + 1, m_columns.end());
            }
        }
        else
            ImGui::TextDisabled("No loaded file.");

        //     ImGui::EndTable();
        // }
    }
    ImGui::End();
}
} // namespace Ui
} // namespace Haviour