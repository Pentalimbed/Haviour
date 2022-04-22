#include "mainwindow.h"
#include "global.h"
#include "listview.h"
#include "propedit.h"
#include "hkx/hkxfile.h"

#include <spdlog/spdlog.h>
#include <nfd.h>
#include <imgui.h>
#include <extern/imgui_notify.h>

namespace Haviour
{
namespace Ui
{
void showAboutWindow()
{
    if (ImGui::Begin("About", &g_show_about))
    {
        ImGui::TextUnformatted("Haviour ver.x.y.z by Penta-limbed-cat / Five-limbed-cat / 5Cat / ProfJack etc.\nA tool for editing xml-formatted Havok hkx behaviour file.\nCopyleft © 2697BCE Whoever");

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
            ImGui::Text("pugixml");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("Arseny Kapoulkine & devs");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::Text("nativefiledialog");
            ImGui::TableNextColumn();
            ImGui::Text("by");
            ImGui::TableNextColumn();
            ImGui::Text("Michael Labbe");

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

void showMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", "CTRL+O"))
            {
                nfdchar_t*  outPath = nullptr;
                nfdresult_t result  = NFD_OpenDialog(nullptr, nullptr, &outPath);
                if (result == NFD_OKAY)
                {
                    spdlog::info("Opening file: {}", outPath);
                    Hkx::HkxFile::getSingleton()->loadFile(outPath);
                    free(outPath);
                }
                else if (result == NFD_ERROR)
                {
                    spdlog::error("Error with file dialog:\n\t{}", NFD_GetError());
                }
            }
            if (ImGui::MenuItem("Save", "CTRL+S")) {}
            if (ImGui::MenuItem("Save As"))
            {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Rebuild Reference List"))
                Hkx::HkxFile::getSingleton()->rebuildRefList();
            if (ImGui::MenuItem("Rebuild Variable List"))
                Hkx::HkxFile::getSingleton()->rebuildVariableList();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            ImGui::MenuItem("Property Editor", nullptr, &g_show_prop_edit);
            ImGui::MenuItem("List View", nullptr, &g_show_list_view);
            ImGui::MenuItem("Tree View", nullptr, &g_show_tree_view);
            ImGui::MenuItem("Node View", nullptr, &g_show_node_view);
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

    if (g_show_about) showAboutWindow();
    if (g_show_list_view) ListView::getSingleton()->show();
    if (g_show_prop_edit) PropEdit::getSingleton()->show();
}
} // namespace Ui

} // namespace Haviour