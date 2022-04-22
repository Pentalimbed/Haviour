#include "propedit.h"
#include "global.h"
#include "classes.h"
#include "hkx/hkxfile.h"
#include "logger.h"

#include <memory>

#include <imgui.h>
#include <extern/imgui_stdlib.h>
#include <extern/imgui_notify.h>

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
    m_file_listener = Hkx::HkxFile::getSingleton()->appendListener(Hkx::kEventLoadFile, [=]() { this->m_edit_obj = {}; this->m_edit_obj_id = {}; this->m_history.clear() ; });
}
PropEdit::~PropEdit()
{
    Hkx::HkxFile::getSingleton()->removeListener(Hkx::kEventLoadFile, m_file_listener);
}

void PropEdit::setObject(pugi::xml_node hkobject)
{
    if (hkobject == m_edit_obj)
        return;
    m_edit_obj    = hkobject;
    m_edit_obj_id = hkobject.attribute("name").as_string();
    m_history.push_front(m_edit_obj);
    if (m_history.size() > m_history_len)
        m_history.pop_back();
}
void PropEdit::setObject(std::string obj_id)
{
    auto node = Hkx::HkxFile::getSingleton()->getNode(obj_id);
    if (node)
        setObject(node);
}

void PropEdit::showHistoryList()
{
    ImGui::TextUnformatted("History");
    if (ImGui::BeginListBox("##history", ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        for (auto history : m_history)
        {
            const bool is_selected = false;
            auto       disp_name   = hkobj2str(history);
            if (ImGui::Selectable(disp_name.c_str(), is_selected))
                setObject(history);
            addToolTip(disp_name.c_str());
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }
}

void PropEdit::showRefList()
{
    ImGui::TextUnformatted("Referenced by");
    if (ImGui::BeginListBox("##refs", ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        if (m_edit_obj)
            for (auto ref : Hkx::HkxFile::getSingleton()->getNodeRefs(m_edit_obj_id))
            {
                const bool is_selected = false;
                auto       disp_name   = hkobj2str(ref);
                if (ImGui::Selectable(disp_name.c_str(), is_selected))
                    setObject(ref);
                addToolTip(disp_name.c_str());
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        ImGui::EndListBox();
    }
}

void PropEdit::show()
{
    if (ImGui::Begin("Property Editor", &g_show_prop_edit))
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
            if (ImGui::InputText("ID", &m_edit_obj_id, ImGuiInputTextFlags_EnterReturnsTrue))
                setObject(m_edit_obj_id);
            addToolTip("Press Enter to apply.");

            if (m_edit_obj)
            {
                ImGui::SameLine();
                if (ImGui::Button("Copy")) ImGui::SetClipboardText(m_edit_obj_id.c_str());
                addToolTip("Copy object ID.");
                ImGui::SameLine();
                if (ImGui::Button("Paste"))
                {
                    auto copied_obj = Hkx::HkxFile::getSingleton()->getNode(ImGui::GetClipboardText());
                    if (copied_obj && (copied_obj.attribute("class") == m_edit_obj.attribute("class")))
                    {
                        for (auto param : copied_obj)
                            if (strcmp(param.attribute("name").as_string(), "name"))
                                if (auto edit_param = m_edit_obj.find_child_by_attribute("name", param.attribute("name").as_string()))
                                    edit_param = param;
                    }
                    else
                        ImGui::InsertNotification({ImGuiToastType_Warning, 3000, "Copied object either not exist or is of different class."});
                }
                addToolTip("If ID in clipboard, override all values except ID and name.");
                ImGui::SameLine();
                if (ImGui::Button("Clear")) {}
                addToolTip("Set all to default value");

                ImGui::Separator();

                showEditUi(m_edit_obj);
            }
            else
            {
                ImGui::Separator();
                ImGui::TextDisabled("Not editing any object");
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