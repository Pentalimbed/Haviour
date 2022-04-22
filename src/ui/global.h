#pragma once

#include <eventpp/eventdispatcher.h>

namespace Haviour
{
namespace Ui
{
#define copyableText(text, ...)                \
    ImGui::TextUnformatted(text, __VA_ARGS__); \
    addToolTip("Left click to copy text");     \
    if (ImGui::IsItemClicked())                \
        ImGui::SetClipboardText(text);

#define copyButton(text)               \
    if (ImGui::Button(ICON_FA_COPY))   \
        ImGui::SetClipboardText(text); \
    addToolTip("Copy");

#define addToolTip(...)         \
    if (ImGui::IsItemHovered()) \
        ImGui::SetTooltip(__VA_ARGS__);

//////////////    Show Window
extern bool g_show_prop_edit;
extern bool g_show_list_view;
extern bool g_show_tree_view;
extern bool g_show_node_view;
extern bool g_show_about;

} // namespace Ui

} // namespace Haviour