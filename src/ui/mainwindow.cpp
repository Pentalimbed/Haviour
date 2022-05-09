#include "mainwindow.h"
#include "hkx/hkxfile.h"
#include "varedit.h"
#include "listview.h"
#include "propedit.h"
#include "columnview.h"
#include "hkx/hkclass.inl"
#include "macros.h"
#include "widgets.h"

#include <fstream>

#include <spdlog/spdlog.h>
#include <nfd.h>
#include <imgui.h>
#include <extern/imgui_notify.h>

namespace Haviour
{
namespace Ui
{
bool g_show_about = false;

void showAboutWindow()
{
    if (ImGui::Begin("About", &g_show_about, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::TextUnformatted("Haviour ver.x.y.z by Penta-limbed-cat / Five-limbed-cat / 5Cat / ProfJack etc.\nA tool for editing Bethesda's xml-formatted Havok hkx behaviour file.\nCopyleft © 2697BCE Whoever");

        ImGui::Separator();

        if (ImGui::BeginTable("Credits", 4, ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::BulletText("FONT");
            ImGui::TableNextColumn();
            ImGui::Text("Atkinson Hyperlegible");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("The Braille Institute");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::Text("Font Awesome 5");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("Fonticons, Inc.");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::BulletText("GUI");
            ImGui::TableNextColumn();
            ImGui::Text("Dear ImGui (GLFW-OpenGL3 Backend)");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("Omar Cornut & devs");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::BulletText("THEME");
            ImGui::TableNextColumn();
            ImGui::Text("ImGuiFontStudio Base Theme (Green/Blue)");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("Aiekick");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::BulletText("LIBS");
            ImGui::TableNextColumn();
            ImGui::Text("spdlog");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("Gabi Melman & devs");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::Text("robin_hood unordered map & set");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("Martin Ankerl");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::Text("pugixml");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("Arseny Kapoulkine & devs");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::Text("Native File Dialog");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("Michael Labbe");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::Text("eventpp");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("Wang Qi");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::Text("imgui-notify");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("patrickcjk");

            ImGui::EndTable();
        }
        ImGui::End();
    }
}

bool shortcut(ImGuiKeyModFlags mod, ImGuiKey key, bool repeat = true)
{
    return (mod == ImGui::GetIO().KeyMods) && ImGui::IsKeyPressed(key, repeat);
}

void newFile()
{
    auto        file_manager = Hkx::HkxFileManager::getSingleton();
    nfdchar_t*  outPath      = nullptr;
    nfdresult_t result       = NFD_SaveDialog("hkx", nullptr, &outPath);
    if (result == NFD_OKAY)
    {
        std::string path = outPath;
        if (!path.ends_with(".hkx"))
            path.append(".hkx");
        {
            std::ofstream hkxstream(path);
            hkxstream << Hkx::g_def_hkx;
        }
        file_manager->loadFile(path);
        free(outPath);
    }
    else if (result == NFD_ERROR)
    {
        spdlog::error("Error with file dialog:\n\t{}", NFD_GetError());
    }
}

void openFile()
{
    auto        file_manager = Hkx::HkxFileManager::getSingleton();
    nfdchar_t*  outPath      = nullptr;
    nfdresult_t result       = NFD_OpenDialog(nullptr, nullptr, &outPath);
    if (result == NFD_OKAY)
    {
        file_manager->loadFile(outPath);
        free(outPath);
    }
    else if (result == NFD_ERROR)
    {
        spdlog::error("Error with file dialog:\n\t{}", NFD_GetError());
    }
}

void saveFileAs()
{
    auto        file_manager = Hkx::HkxFileManager::getSingleton();
    nfdchar_t*  outPath      = nullptr;
    nfdresult_t result       = NFD_SaveDialog("hkx", file_manager->getCurrentFile().getPath().data(), &outPath);
    if (result == NFD_OKAY)
    {
        file_manager->saveFile(outPath);
        free(outPath);
    }
    else if (result == NFD_ERROR)
    {
        spdlog::error("Error with file dialog:\n\t{}", NFD_GetError());
    }
}

void showMenuBar()
{
    auto file_manager = Hkx::HkxFileManager::getSingleton();

    if (shortcut(ImGuiKeyModFlags_Ctrl, ImGuiKey_O)) openFile();
    if (shortcut(ImGuiKeyModFlags_Ctrl, ImGuiKey_S)) file_manager->saveFile();
    if (shortcut(ImGuiKeyModFlags_Ctrl, ImGuiKey_F4)) file_manager->closeCurrentFile();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            bool dummy_selected = false;
            if (ImGui::MenuItem("New", "CTRL+N"))
                newFile();
            if (ImGui::MenuItem("Open", "CTRL+O"))
                openFile();
            if (ImGui::MenuItem("Save", "CTRL+S", &dummy_selected, file_manager->isFileSelected()))
                file_manager->saveFile();
            if (ImGui::MenuItem("Save As", nullptr, &dummy_selected, file_manager->isFileSelected()))
                saveFileAs();

            ImGui::Separator();

            auto path_list = file_manager->getPathList();
            if (path_list.empty())
                ImGui::TextDisabled("No file loaded");
            else
                for (int i = 0; i < path_list.size(); ++i)
                    if (ImGui::MenuItem(path_list[i].data(), nullptr, i == file_manager->getCurrentFileIdx()))
                        file_manager->setCurrentFile(i);

            ImGui::Separator();

            if (ImGui::MenuItem("Close", "Ctrl+F4"))
                file_manager->closeCurrentFile();
            if (ImGui::MenuItem("Close All"))
                file_manager->closeAllFiles();

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Resources"))
        {
            if (ImGui::MenuItem("Load Project File")) {}
            addTooltip("Load both the character and skeleton");

            ImGui::Separator();

            if (ImGui::MenuItem("Load Skeleton"))
            {
                nfdchar_t*  outPath = nullptr;
                nfdresult_t result  = NFD_OpenDialog(nullptr, nullptr, &outPath);
                if (result == NFD_OKAY)
                {
                    file_manager->m_skel_file.loadFile(outPath);
                    free(outPath);
                }
                else if (result == NFD_ERROR)
                {
                    spdlog::error("Error with file dialog:\n\t{}", NFD_GetError());
                }
            }
            if (ImGui::MenuItem("Load Character"))
            {
                nfdchar_t*  outPath = nullptr;
                nfdresult_t result  = NFD_OpenDialog(nullptr, nullptr, &outPath);
                if (result == NFD_OKAY)
                {
                    file_manager->m_char_file.loadFile(outPath);
                    free(outPath);
                }
                else if (result == NFD_ERROR)
                {
                    spdlog::error("Error with file dialog:\n\t{}", NFD_GetError());
                }
            }

            ImGui::Separator();

            ImGui::TextDisabled("Skelton: %s", file_manager->m_skel_file.isFileLoaded() ? file_manager->m_skel_file.getPath().data() : "None");
            ImGui::TextDisabled("Character: %s", file_manager->m_char_file.isFileLoaded() ? file_manager->m_char_file.getPath().data() : "None");

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Build Reference List", nullptr, false, file_manager->isFileSelected()))
                file_manager->getCurrentFile().buildRefList();
            if (ImGui::MenuItem("Reindex Objects", nullptr, false, file_manager->isFileSelected()))
                file_manager->getCurrentFile().reindexObj();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            ImGui::MenuItem("Property Editor", nullptr, &PropEdit::getSingleton()->m_show);
            ImGui::MenuItem("Variable/Event List", nullptr, &VarEdit::getSingleton()->m_show);
            ImGui::Separator();
            ImGui::MenuItem("List View", nullptr, &ListView::getSingleton()->m_show);
            ImGui::MenuItem("Column View", nullptr, &ColumnView::getSingleton()->m_show);
            // ImGui::MenuItem("Node View", nullptr, &g_show_node_view);
            ImGui::Separator();
            ImGui::MenuItem("About", nullptr, &g_show_about);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void showDockSpace()
{
    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoDecoration;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Main", NULL, flags))
    {
        ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    }
    ImGui::PopStyleVar();
    ImGui::End();
}

void showMainWindow()
{
    showMenuBar();
    showDockSpace();

    if (VarEdit::getSingleton()->m_show) VarEdit::getSingleton()->show();
    if (PropEdit::getSingleton()->m_show) PropEdit::getSingleton()->show();

    if (ListView::getSingleton()->m_show) ListView::getSingleton()->show();
    if (ColumnView::getSingleton()->m_show) ColumnView::getSingleton()->show();

    if (g_show_about) showAboutWindow();

    MacroManager::getSingleton()->show();
}
} // namespace Ui

} // namespace Haviour