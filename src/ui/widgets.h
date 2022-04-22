// Widgets for editing all sorts of params.
// Must be used in a table context.
#pragma once

#include <format>

#include <pugixml.hpp>
#include <imgui.h>
#include <extern/imgui_stdlib.h>
#include <extern/font_awesome_5.h>

#include "propedit.h"
#include "global.h"
#include "hkx/hkclass.h"

namespace Haviour
{
namespace Ui
{
inline bool variablePopup(const char* str_id, Hkx::Variable& out)
{
    static std::vector<Hkx::Variable> var_list_cache;
    static bool                       need_update = true;
    static std::string                search_text;

    if (need_update)
    {
        need_update    = false;
        search_text    = {};
        var_list_cache = Hkx::HkxFile::getSingleton()->getVariableList();
    }

    if (ImGui::BeginPopup(str_id))
    {
        ImGui::InputText("Filter", &search_text);
        ImGui::Separator();

        for (auto var : var_list_cache)
            if (!var.deleted &&
                (search_text.empty() ||
                 !std::ranges::search(var.name, search_text, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty()) &&
                ImGui::Selectable(std::format("{:3} {}", var.idx, var.name).c_str()))
            {
                ImGui::CloseCurrentPopup();
                out = var;
                return true;
            }

        ImGui::EndPopup();
    }
    return false;
}

////////////////    EDITS

inline void stringEdit(pugi::xml_node hkparam)
{
    ImGui::TableNextColumn();
    std::string value = hkparam.text().as_string();
    if (ImGui::InputText(hkparam.attribute("name").as_string(), &value))
        hkparam.text() = value.c_str();

    ImGui::TableNextColumn();
    // copyButton(value.c_str());
}

template <size_t N>
inline void enumEdit(pugi::xml_node hkparam, std::array<std::string_view, N> enums)
{
    ImGui::TableNextColumn();
    std::string value = hkparam.text().as_string();
    if (ImGui::BeginCombo(hkparam.attribute("name").as_string(), value.c_str()))
    {
        for (std::string_view enum_name : enums)
        {
            const bool is_selected = value == enum_name;
            if (ImGui::Selectable(enum_name.data(), is_selected))
                hkparam.text().set(enum_name.data());
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::TableNextColumn();
    // copyButton(value.c_str());
}

inline void intEdit(pugi::xml_node hkparam)
{
    ImGui::TableNextColumn();
    auto value = hkparam.text().as_int();
    if (ImGui::InputInt(hkparam.attribute("name").as_string(), &value))
        hkparam.text() = value;

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());
}

inline void variableEdit(pugi::xml_node hkparam)
{
    ImGui::PushID(hkparam.attribute("name").as_string());

    intEdit(hkparam);

    Hkx::Variable out_var;
    if (variablePopup("Pickvar", out_var))
        hkparam.text() = out_var.idx;
    if (ImGui::Button(ICON_FA_SEARCH))
        ImGui::OpenPopup("Pickvar");
    addToolTip("Select variable");

    ImGui::PopID();
}

inline void sliderFloatEdit(pugi::xml_node hkparam, float lb, float ub)
{
    ImGui::TableNextColumn();
    float value = hkparam.text().as_float();
    if (ImGui::SliderFloat(hkparam.attribute("name").as_string(), &value, lb, ub, "%.6f"))
        hkparam.text() = std::format("{:.6f}", value).c_str();

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());
}

inline void floatEdit(pugi::xml_node hkparam)
{
    ImGui::TableNextColumn();
    float value = hkparam.text().as_float();
    if (ImGui::InputFloat(hkparam.attribute("name").as_string(), &value, 0.000001f, 0.1f, "%.6f"))
        hkparam.text() = std::format("{:.6f}", value).c_str();

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());
}

inline void refEdit(pugi::xml_node hkparam, std::vector<std::string_view> classes)
{
    ImGui::TableNextColumn();

    ImGui::PushID(hkparam.attribute("name").as_string());

    bool        edited    = false;
    std::string old_value = hkparam.text().as_string();
    auto        value     = old_value;
    auto        hkxfile   = Hkx::HkxFile::getSingleton();

    if (ImGui::InputText(hkparam.attribute("name").as_string(), &value))
    {
        edited = true;
        if (auto obj = hkxfile->getNode(value))                                                                    // Is obj
            if (std::ranges::find(classes, std::string_view(obj.attribute("class").as_string())) != classes.end()) // Same class
                hkparam.text() = value.c_str();
            else
                hkparam.text() = "null";
        else
            hkparam.text() = "null";
    }
    addToolTip("Valid object ID only, otherwise null");

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());

    // ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_CIRCLE_RIGHT))
        PropEdit::getSingleton()->setObject(value);
    addToolTip("Go to");

    ImGui::SameLine();
    if (ImGui::BeginPopup("Add Class"))
    {
        for (auto class_name : classes)
            if (ImGui::Selectable(class_name.data()))
            {
                // TODO add item
                ImGui::CloseCurrentPopup();
            }
        ImGui::EndPopup();
    }
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        ImGui::OpenPopup("Add Class");
    addToolTip("Append");

    if (edited)
    {
        hkxfile->deRef(old_value, hkparam.parent());
        hkxfile->addRef(value, hkparam.parent());
    }

    ImGui::PopID();
}

} // namespace Ui
} // namespace Haviour