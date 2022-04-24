// Widgets for editing all sorts of params.
// All edits must be used in a table context.
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
inline bool variablePickerPopup(const char* str_id, Hkx::Variable& out)
{
    static std::vector<Hkx::Variable> var_list_cache;
    static bool                       need_update = true;
    static std::string                search_text;

    if (need_update)
    {
        need_update    = false;
        search_text    = {};
        var_list_cache = Hkx::HkxFile::getSingleton()->m_var_manager.getVariableList();
    }

    if (ImGui::BeginPopup(str_id))
    {
        ImGui::InputText("Filter", &search_text);
        ImGui::Separator();

        std::vector<Hkx::Variable> var_filtered;
        var_filtered.reserve(var_list_cache.size());
        std::ranges::copy_if(var_list_cache, std::back_inserter(var_filtered),
                             [](auto& var) {
                                 auto var_disp_name = std::format("{:3} {}", var.idx, var.name.text().as_string());
                                 return !var.invalid &&
                                     (search_text.empty() ||
                                      !std::ranges::search(var_disp_name, search_text, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty());
                             });

        ImGuiListClipper clipper;
        clipper.Begin(var_filtered.size());
        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                auto& var = var_filtered[row_n];

                if (!var.invalid)
                {
                    auto var_disp_name = std::format("{:3} {}", var.idx, var.name.text().as_string());
                    if (ImGui::Selectable(var_disp_name.c_str()))
                    {
                        ImGui::CloseCurrentPopup();
                        out = var;
                        ImGui::EndPopup();
                        return true;
                    }
                }
            }

        ImGui::EndPopup();
    }
    else
        need_update = true;

    return false;
}

template <size_t N>
inline void flagPopup(const char* str_id, pugi::xml_node hkparam, const std::array<Hkx::EnumWrapper, N>& flags)
{
    std::string value_text = hkparam.text().as_string();
    uint32_t    value      = 0;
    if (ImGui::BeginPopup(str_id))
    {
        bool value_changed = false;
        for (Hkx::EnumWrapper flag : flags)
        {
            if (flag.val && value_text.contains(flag.name))
                value |= flag.val;
        }
        for (Hkx::EnumWrapper flag : flags)
            if (flag.val)
            {
                if (ImGui::CheckboxFlags(flag.name.data(), &value, flag.val))
                    value_changed = true;
                if (!flag.hint.empty())
                    addToolTip(flag.hint.data());
            }
        if (value_changed)
        {
            value_text = {};
            if (!value) value_text = "0";
            else
            {
                for (Hkx::EnumWrapper flag : flags)
                    if (value & flag.val)
                        value_text.append(std::format("{}|", flag.name));
                if (value_text.ends_with('|'))
                    value_text.pop_back();
            }
            hkparam.text() = value_text.c_str();
        }
        ImGui::EndPopup();
    }
}
template <size_t N>
inline void flagEditButton(const char* str_id, pugi::xml_node hkparam, const std::array<Hkx::EnumWrapper, N>& flags)
{
    flagPopup(str_id, hkparam, flags);
    if (ImGui::Button(ICON_FA_FLAG))
        ImGui::OpenPopup(str_id);
    addToolTip("Edit individual flags");
}

////////////////    EDITS

inline void stringEdit(pugi::xml_node hkparam, std::string_view manual_name = {})
{
    ImGui::TableNextColumn();
    std::string value = hkparam.text().as_string();
    if (ImGui::InputText(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
        hkparam.text() = value.c_str();

    ImGui::TableNextColumn();
    // copyButton(value.c_str());
}

inline void boolEdit(pugi::xml_node hkparam, std::string_view manual_name = {})
{
    ImGui::TableNextColumn();
    bool value = hkparam.text().as_bool();
    if (ImGui::Checkbox(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
        hkparam.text() = value;

    ImGui::TableNextColumn();
    // copyButton(value.c_str());
}

template <size_t N>
inline void enumEdit(pugi::xml_node hkparam, const std::array<Hkx::EnumWrapper, N>& enums, std::string_view manual_name = {})
{
    ImGui::TableNextColumn();
    std::string value = hkparam.text().as_string();
    if (ImGui::BeginCombo(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), value.c_str()))
    {
        for (Hkx::EnumWrapper enum_wrapper : enums)
        {
            const bool is_selected = value == enum_wrapper.name;
            if (ImGui::Selectable(enum_wrapper.name.data(), is_selected))
                hkparam.text().set(enum_wrapper.name.data());
            if (!enum_wrapper.hint.empty())
                addToolTip(enum_wrapper.hint.data());
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::TableNextColumn();
    // copyButton(value.c_str());
}

inline void intEdit(pugi::xml_node hkparam, std::string_view manual_name = {})
{
    ImGui::TableNextColumn();
    auto value = hkparam.text().as_int();
    if (ImGui::InputInt(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
        hkparam.text() = value;

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());
}

inline void sliderIntEdit(pugi::xml_node hkparam, int lb, int ub, std::string_view manual_name = {})
{
    ImGui::TableNextColumn();
    int value = hkparam.text().as_int();
    if (ImGui::SliderInt(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, lb, ub, "%.6f"))
        hkparam.text() = value;

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());
}

template <size_t N>
inline void flagEdit(pugi::xml_node hkparam, const std::array<Hkx::EnumWrapper, N>& flags, std::string_view manual_name = {})
{
    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    stringEdit(hkparam, manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());
    flagEditButton("Edit flags", hkparam, flags);

    ImGui::PopID();
}

inline void variablePickerEdit(pugi::xml_node hkparam, std::string_view manual_name = {})
{
    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    intEdit(hkparam, manual_name);

    Hkx::Variable out_var;
    if (variablePickerPopup("Pickvar", out_var))
        hkparam.text() = out_var.idx;
    if (ImGui::Button(ICON_FA_SEARCH))
        ImGui::OpenPopup("Pickvar");
    addToolTip("Select variable");

    ImGui::PopID();
}

inline void sliderFloatEdit(pugi::xml_node hkparam, float lb, float ub, std::string_view manual_name = {})
{
    ImGui::TableNextColumn();
    float value = hkparam.text().as_float();
    if (ImGui::SliderFloat(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, lb, ub, "%.6f"))
        hkparam.text() = std::format("{:.6f}", value).c_str();

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());
}

inline void floatEdit(pugi::xml_node hkparam, std::string_view manual_name = {})
{
    ImGui::TableNextColumn();
    float value = hkparam.text().as_float();
    if (ImGui::InputFloat(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, 0.0f, 0.0f, "%.6f"))
        hkparam.text() = std::format("{:.6f}", value).c_str();

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());
}

// for floats written in UINT32 values
inline void convertFloatEdit(pugi::xml_node hkparam, std::string_view manual_name = {})
{
    ImGui::TableNextColumn();
    auto  raw_value = hkparam.text().as_uint();
    float value     = *reinterpret_cast<float*>(&raw_value);
    if (ImGui::InputFloat(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, 0.0f, 0.0f))
        hkparam.text() = *reinterpret_cast<uint32_t*>(&value);

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());
}

inline void quadEdit(pugi::xml_node hkparam, std::string_view manual_name)
{
    ImGui::TableNextColumn();

    float       value[4];
    std::string text(hkparam.text().as_string());
    std::erase_if(text, [](char& c) { return (c == '(') || (c == ')'); });
    std::istringstream text_stream(text);
    text_stream >> value[0] >> value[1] >> value[2] >> value[3];

    if (ImGui::InputFloat4(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), value, "%.6f"))
        hkparam.text() = std::format("({:.6f} {:.6f} {:.6f} {:.6f})", value[0], value[1], value[2], value[3]).c_str();

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());
}

inline void refEdit(pugi::xml_node hkparam, const std::vector<std::string_view>& classes, std::string_view manual_name = {})
{
    ImGui::TableNextColumn();

    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    bool        edited    = false;
    std::string old_value = hkparam.text().as_string();
    auto        value     = old_value;
    auto        hkxfile   = Hkx::HkxFile::getSingleton();

    if (ImGui::InputText(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
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

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());

    // ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_CIRCLE_RIGHT))
        PropEdit::getSingleton()->setObject(value);
    addToolTip("Go to");

    ImGui::SameLine();
    if (ImGui::BeginPopup("Append"))
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
        ImGui::OpenPopup("Append");
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