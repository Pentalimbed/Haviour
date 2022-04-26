// Widgets for editing all sorts of params.
// All edits must be used in a table context.
#pragma once
#include "hkx/hkxfile.h"

#include <fmt/format.h>
#include <pugixml.hpp>
#include <imgui.h>
#include <extern/font_awesome_5.h>

namespace Haviour
{
namespace Ui
{
#define copyableText(text, ...)                \
    ImGui::TextUnformatted(text, __VA_ARGS__); \
    addTooltip("Left click to copy text");     \
    if (ImGui::IsItemClicked())                \
        ImGui::SetClipboardText(text);


#define addTooltip(...) \
    if (ImGui::IsItemHovered()) ImGui::SetTooltip(__VA_ARGS__);

bool variablePickerPopup(const char*           str_id,
                         Hkx::VariableManager& var_manager,
                         Hkx::Variable&        out);

bool eventPickerPopup(const char*                 str_id,
                      Hkx::AnimationEventManager& evt_manager,
                      Hkx::AnimationEvent&        out);

template <size_t N>
void flagPopup(const char* str_id, pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags)
{
    std::string value_text = hkparam.text().as_string();
    uint32_t    value      = 0;
    if (ImGui::BeginPopup(str_id))
    {
        bool value_changed = false;
        for (EnumWrapper flag : flags)
        {
            if (flag.val && value_text.contains(flag.name))
                value |= flag.val;
        }
        for (EnumWrapper flag : flags)
            if (flag.val)
            {
                if (ImGui::CheckboxFlags(flag.name.data(), &value, flag.val))
                    value_changed = true;
                if (!flag.hint.empty())
                    addTooltip(flag.hint.data());
            }
        if (value_changed)
        {
            value_text = {};
            if (!value) value_text = "0";
            else
            {
                for (EnumWrapper flag : flags)
                    if (value & flag.val)
                        value_text.append(fmt::format("{}|", flag.name));
                if (value_text.ends_with('|'))
                    value_text.pop_back();
            }
            hkparam.text() = value_text.c_str();
        }
        ImGui::EndPopup();
    }
}

template <size_t N>
void flagEditButton(const char* str_id, pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags)
{
    flagPopup(str_id, hkparam, flags);
    if (ImGui::Button(ICON_FA_FLAG))
        ImGui::OpenPopup(str_id);
    addTooltip("Edit individual flags");
}

////////////////    EDITS
inline void twoSepartors()
{
    ImGui::TableNextColumn();
    ImGui::Separator();
    ImGui::TableNextColumn();
    ImGui::Separator();
}

void stringEdit(pugi::xml_node   hkparam,
                std::string_view hint        = {},
                std::string_view manual_name = {});

void boolEdit(pugi::xml_node   hkparam,
              std::string_view hint        = {},
              std::string_view manual_name = {});

template <size_t N>
void enumEdit(pugi::xml_node hkparam, const std::array<EnumWrapper, N>& enums, std::string_view hint = {}, std::string_view manual_name = {})
{
    ImGui::TableNextColumn();
    std::string value = hkparam.text().as_string();
    if (ImGui::BeginCombo(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), value.c_str()))
    {
        for (EnumWrapper enum_wrapper : enums)
        {
            const bool is_selected = value == enum_wrapper.name;
            if (ImGui::Selectable(enum_wrapper.name.data(), is_selected))
                hkparam.text().set(enum_wrapper.name.data());
            if (!enum_wrapper.hint.empty())
                addTooltip(enum_wrapper.hint.data());
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    else if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
}

void intScalarEdit(pugi::xml_node hkparam, ImGuiDataType data_type = ImGuiDataType_S32, std::string_view hint = {}, std::string_view manual_name = {});

void sliderIntEdit(pugi::xml_node   hkparam,
                   int              lb,
                   int              ub,
                   std::string_view hint        = {},
                   std::string_view manual_name = {});

template <size_t N>
void flagEdit(pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags, std::string_view hint = {}, std::string_view manual_name = {})
{
    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    stringEdit(hkparam, hint, manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());
    flagEditButton("Edit flags", hkparam, flags);

    ImGui::PopID();
}

void variablePickerEdit(pugi::xml_node        hkparam,
                        Hkx::VariableManager& var_manager,
                        std::string_view      hint        = {},
                        std::string_view      manual_name = {});

void eventPickerEdit(pugi::xml_node              hkparam,
                     Hkx::AnimationEventManager& evt_manager,
                     std::string_view            hint        = {},
                     std::string_view            manual_name = {});

void sliderFloatEdit(pugi::xml_node   hkparam,
                     float            lb,
                     float            ub,
                     std::string_view hint        = {},
                     std::string_view manual_name = {});

void floatEdit(pugi::xml_node   hkparam,
               std::string_view hint        = {},
               std::string_view manual_name = {});

// for floats written in UINT32 values
void convertFloatEdit(pugi::xml_node   hkparam,
                      std::string_view hint        = {},
                      std::string_view manual_name = {});

void quadEdit(pugi::xml_node   hkparam,
              std::string_view hint        = {},
              std::string_view manual_name = {});

void refEdit(pugi::xml_node                       hkparam,
             const std::vector<std::string_view>& classes,
             Hkx::HkxFile&                        file,
             std::string_view                     hint        = {},
             std::string_view                     manual_name = {});

void refEdit(std::string&                         value,
             const std::vector<std::string_view>& classes,
             pugi::xml_node                       parent,
             Hkx::HkxFile&                        file,
             std::string_view                     hint,
             std::string_view                     manual_name);

////////////////    Linked Prop Edits

void varEditPopup(const char* str_id, Hkx::Variable& var, Hkx::HkxFile& file);
void evtEditPopup(const char* str_id, Hkx::AnimationEvent& evt, Hkx::HkxFile& file);
void propEditPopup(const char* str_id, Hkx::CharacterProperty& evt, Hkx::HkxFile& file);

} // namespace Ui
} // namespace Haviour