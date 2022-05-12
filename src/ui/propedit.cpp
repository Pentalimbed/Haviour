#include "propedit.h"
#include "widgets.h"
#include "utils.h"
#include "hkx/hkclass.inl"
#include "classinterface.h"
#include "macros.h"

#include <spdlog/spdlog.h>
#include <imgui.h>
#include <extern/imgui_stdlib.h>

namespace Haviour
{
namespace Ui
{
PropEdit* PropEdit::getSingleton()
{
    static PropEdit edit;
    return std::addressof(edit);
}
PropEdit::PropEdit()
{
    auto file_manager = Hkx::HkxFileManager::getSingleton();
    m_file_listener   = file_manager->appendListener(Hkx::kEventFileChanged, [=]() { this->m_edit_obj_id = {}; this->m_history.clear() ; });
}
PropEdit::~PropEdit()
{
    auto file_manager = Hkx::HkxFileManager::getSingleton();
    file_manager->removeListener(Hkx::kEventFileChanged, m_file_listener);
}

void PropEdit::setObject(std::string_view obj_id)
{
    if (obj_id == m_edit_obj_id || !Hkx::HkxFileManager::getSingleton()->getCurrentFile()->getObj(obj_id))
        return;
    m_edit_obj_id = obj_id;
    m_history.push_front(m_edit_obj_id);
    if (m_history.size() > m_history_len)
        m_history.pop_back();
}

void PropEdit::showHistoryList()
{
    ImGui::TextUnformatted("History");
    if (ImGui::BeginListBox("##history", ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        for (auto history : m_history)
        {
            auto history_node = Hkx::HkxFileManager::getSingleton()->getCurrentFile()->getObj(history);
            if (history_node)
            {
                const bool is_selected = false;
                auto       disp_name   = hkobj2str(history_node);
                if (ImGui::Selectable(disp_name.c_str(), is_selected))
                    setObject(history);
                addTooltip(disp_name.c_str());
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            else
                ImGui::TextDisabled("Invalid object");
        }
        ImGui::EndListBox();
    }
}

void PropEdit::showRefList()
{
    ImGui::TextUnformatted("Referenced by");
    if (ImGui::BeginListBox("##refs", ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        if (!m_edit_obj_id.empty())
        {
            std::vector<std::string> ref_list = {};
            auto&                    file     = *Hkx::HkxFileManager::getSingleton()->getCurrentFile();
            file.getObjRefs(m_edit_obj_id, ref_list);
            for (auto& ref_id : ref_list)
            {
                const bool is_selected = false;
                auto       disp_name   = hkobj2str(file.getObj(ref_id));
                if (ImGui::Selectable(disp_name.c_str(), is_selected))
                    setObject(ref_id);
                addTooltip(disp_name.c_str());
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndListBox();
    }
}

void PropEdit::show()
{
    if (ImGui::Begin("Property Editor", &m_show, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        if (ImGui::BeginTable("propedit", 3, ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("history", ImGuiTableColumnFlags_WidthFixed, 200.0F);
            ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("refs", ImGuiTableColumnFlags_WidthFixed, 200.0F);

            // History list
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            showHistoryList();

            ImGui::TableNextColumn();
            if (ImGui::InputText("ID", &m_edit_obj_id_input, ImGuiInputTextFlags_EnterReturnsTrue))
                setObject(m_edit_obj_id_input);
            addTooltip("Press Enter to apply.");

            if (auto _file = Hkx::HkxFileManager::getSingleton()->getCurrentFile(); _file)
            {
                auto& file     = *_file;
                auto  edit_obj = file.getObj(m_edit_obj_id);
                if (edit_obj)
                {
                    auto class_str = edit_obj.attribute("class").as_string();

                    ImGui::SameLine();
                    if (ImGui::Button(ICON_FA_COPY))
                        ImGui::SetClipboardText(m_edit_obj_id.c_str());
                    addTooltip("Copy\nCopy object ID.");
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_FA_PASTE))
                    {
                        auto copied_obj = file.getObj(ImGui::GetClipboardText());
                        if (copied_obj && !strcmp(copied_obj.attribute("class").as_string(), class_str))
                        {
                            edit_obj.remove_children();
                            for (auto child : copied_obj.children())
                                edit_obj.append_copy(child);
                            file.buildRefList(m_edit_obj_id);
                        }
                        else
                            spdlog::warn("Copied object either not exist or is of different class.");
                    }
                    addTooltip("Paste\nIf ID in clipboard, override all values except ID.");

                    auto& def_map = Hkx::getClassDefaultMap();
                    if (def_map.contains(class_str))
                    {
                        ImGui::SameLine();
                        if (ImGui::Button(ICON_FA_RECYCLE))
                        {
                            std::string_view   def_str = def_map.at(class_str);
                            pugi::xml_document doc;
                            if (doc.load_buffer(def_str.data(), def_str.length()))
                            {
                                edit_obj.remove_children();
                                for (auto child : doc.first_child().children())
                                    edit_obj.append_copy(child);
                            }
                            else
                                spdlog::warn("Failed to load default value for class {}", class_str);
                        }
                        addTooltip("Reset.\nSet all parameteres to default.");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_FA_SCROLL))
                    {
                        ImGui::OpenPopup("Macro Select");
                    }
                    addTooltip("Macros");
                    if (ImGui::BeginPopup("Macro Select"))
                    {
                        bool has_something = false;
                        for (auto& macro : MacroManager::getSingleton()->getMacros())
                            if (!strcmp(macro->getClass(), class_str))
                            {
                                has_something = true;
                                if (ImGui::Selectable(macro->getName()))
                                {
                                    ImGui::CloseCurrentPopup();
                                    macro->open(edit_obj, &file);
                                }
                                addTooltip(macro->getHint());
                            }
                        if (!has_something)
                            ImGui::TextDisabled("No macro for %s :(", class_str);
                        ImGui::EndPopup();
                    }

                    if (file.isObjEssential(m_edit_obj_id))
                    {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "ESSENTIAL");
                    }

                    ImGui::Separator();

                    ImGui::Text("Editing: %s", hkobj2str(edit_obj).c_str());
                    if (auto class_info = Hkx::getClassInfo(); class_info.contains(edit_obj.attribute("class").as_string()))
                        addTooltip(class_info.at(edit_obj.attribute("class").as_string()).data());

                    ImGui::Separator();

                    showEditUi(edit_obj, file);
                }
                else
                {
                    ImGui::Separator();
                    ImGui::TextDisabled("Not editing any object");
                }
            }
            else
            {
                ImGui::Separator();
                ImGui::TextDisabled("No loaded file.");
            }

            ImGui::TableNextColumn();
            showRefList();

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

} // namespace Ui
} // namespace Haviour