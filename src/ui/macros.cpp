#include "macros.h"

#include "widgets.h"
#include "hkx/hkutils.h"
#include "hkx/hkclass.inl"

#include <filesystem>

#include <imgui.h>
#include <extern/imgui_stdlib.h>

namespace Haviour
{
namespace Ui
{
void MacroModal::open(pugi::xml_node working_obj, Hkx::HkxFile* file)
{
    m_open        = true;
    m_working_obj = working_obj;
    m_file        = file;
}
void MacroModal::show()
{
    if (getClass() && (!m_working_obj || !m_file))
    {
        m_open = false;
        return;
    }

    if (m_open)
    {
        m_open = false;
        ImGui::OpenPopup(getName());
    }

    bool unused_open;
    if (ImGui::BeginPopupModal(getName(), &unused_open, ImGuiWindowFlags_NoScrollbar))
    {
        drawUi();
        ImGui::EndPopup();
    }
}
//////////////////// Trigger Parser

void TriggerMacro::open(pugi::xml_node working_obj, Hkx::HkxFile* file)
{
    if (file->getType() == Hkx::HkxFile::kBehaviour)
    {
        MacroModal::open(working_obj, file);
        m_parse_text = hkTriggerArray2Str(working_obj, *dynamic_cast<Hkx::BehaviourFile*>(file));
    }
}

void TriggerMacro::drawUi()
{
    ImGui::TextUnformatted(getHint());
    ImGui::Separator();
    if (ImGui::BeginTable("tbl", 2, ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("opts", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("txts", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::Checkbox("Create New Events", &m_create_new_event);
        addTooltip("If checked, the macro will create new events for those that couldn't be matched.\n"
                   "If unchecked, unmatched events will be ignored.");
        ImGui::Checkbox("Replace Triggers", &m_replace);
        addTooltip("If checked, all triggers in the trigger array will be replaced.\n"
                   "If unchecked, new triggers will be appended.");
        ImGui::Separator();
        if (ImGui::Button("Parse"))
        {
            parse();
            ImGui::CloseCurrentPopup();
        }

        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Trigger Text");
        ImGui::InputTextMultiline("##Trigger Text", &m_parse_text, {-FLT_MIN, -FLT_MIN});
        ImGui::EndTable();
    }
}

void TriggerMacro::parse()
{
    auto file          = dynamic_cast<Hkx::BehaviourFile*>(m_file);
    auto triggers_node = m_working_obj.getByName("triggers");
    if (m_replace)
    {
        triggers_node.remove_children();
        triggers_node.attribute("numelements") = 0;
    }

    std::istringstream stream(m_parse_text.data());
    std::string        line;
    while (std::getline(stream, line))
    {
        std::istringstream line_stream(line.data());

        std::string evt_name;
        line_stream >> evt_name;
        auto evt = file->m_evt_manager.getEntryByName(evt_name);

        if (!evt.m_valid)
        {
            if (m_create_new_event)
            {
                evt                             = file->m_evt_manager.addEntry();
                evt.get<Hkx::PropName>().text() = evt_name.c_str();
            }
            else
                continue;
        }

        line_stream.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
        auto ch      = line_stream.peek();
        bool acyclic = line_stream.peek() == '+';

        float time;
        line_stream >> time;

        auto trigger                                                    = appendXmlString(triggers_node, Hkx::g_def_hkbClipTrigger);
        trigger.getByName("localTime").text()                           = fmt::format("{:.6f}", time).c_str();
        trigger.getByName("acyclic").text()                             = acyclic ? "true" : "false";
        trigger.getByName("relativeToEndOfClip").text()                 = time < 0 ? "true" : "false";
        trigger.getByName("event").first_child().getByName("id").text() = evt.m_index;
    }
}

//////////////////// CRC32

void Crc32Macro::open(pugi::xml_node working_obj, Hkx::HkxFile* file)
{
    MacroModal::open(working_obj, file);
    m_path = {};
}

void Crc32Macro::drawUi()
{
    ImGui::TextUnformatted(getHint());
    ImGui::Separator();
    if (ImGui::InputTextWithHint("Path", R"(meshes\actors\character\animations\1hm_attackright.hkx)", &m_path, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        auto temp_path = m_path;

        for (auto& ch : temp_path)
            if (ch == '/') ch = '\\';
        transform(temp_path.begin(), temp_path.end(), temp_path.begin(), ::tolower);

        std::filesystem::path path(temp_path);
        if (path.has_stem() && path.has_parent_path())
        {
            auto parent = path.parent_path().string();
            auto stem   = path.stem().string();

            m_out_crc = fmt::format("{}\n{}\n{}",
                                    Crc32::update(parent.data(), parent.length()),
                                    Crc32::update(stem.data(), stem.length()),
                                    7891816);
        }
    }
    addTooltip("Press Enter to convert");
    ImGui::InputTextMultiline("Output", &m_out_crc, {}, ImGuiInputTextFlags_ReadOnly);
}

//////////////////// Macro manager

MacroManager* MacroManager::getSingleton()
{
    static MacroManager manager;
    return std::addressof(manager);
}

MacroManager::MacroManager()
{
    m_macros.emplace_back(std::make_unique<TriggerMacro>());
    m_macros.emplace_back(std::make_unique<Crc32Macro>());

    m_file_listener = Hkx::HkxFileManager::getSingleton()->appendListener(Hkx::kEventFileChanged, [=]() {
        for (auto& macro : m_macros)
        {
            macro->m_open        = false;
            macro->m_file        = nullptr;
            macro->m_working_obj = {};
        }
    });
}

void MacroManager::show()
{
    for (auto& macro : m_macros)
        macro->show();
}

} // namespace Ui
} // namespace Haviour