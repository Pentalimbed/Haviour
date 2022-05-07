// Widgets for editing all sorts of params.
// All edits must be used in a table context.
#pragma once
#include "hkx/hkxfile.h"

#include <fmt/format.h>
#include <pugixml.hpp>
#include <imgui.h>
#include <extern/font_awesome_5.h>
#include <extern/imgui_stdlib.h>

namespace Haviour
{

namespace Hkx
{
VariableTypeEnum getVarTypeEnum(std::string_view enumstr);
}

namespace Ui
{
const auto g_color_invalid = ImColor(0.2f, 0.2f, 0.2f).Value;
const auto g_color_bool    = ImColor(0xFF, 0x9C, 0x83).Value;
const auto g_color_int     = ImColor(0x07, 0xd2, 0xd9).Value;
const auto g_color_float   = ImColor(0xF6, 0x6f, 0x9a).Value;
const auto g_color_attr    = ImColor(0x9d, 0x00, 0x1c).Value;
const auto g_color_quad    = ImColor(0x5a, 0xe6, 0xb8).Value;

#define copyableText(text, ...)                \
    ImGui::TextUnformatted(text, __VA_ARGS__); \
    addTooltip("Left click to copy text");     \
    if (ImGui::IsItemClicked())                \
        ImGui::SetClipboardText(text);

#define addTooltip(...) \
    if (ImGui::IsItemHovered()) ImGui::SetTooltip(__VA_ARGS__);

template <typename Manager>
bool linkedPropPickerPopup(const char*              str_id,
                           Manager&                 prop_manager,
                           bool                     set_focus,
                           typename Manager::Entry& out)
{
    static bool        need_update = true;
    static std::string search_text;

    if (ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
    {
        if (set_focus)
            ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Filter", &search_text);

        ImGui::Separator();

        auto prop_list = prop_manager.getEntryList();
        std::erase_if(prop_list,
                      [=](auto& prop) {
                          auto prop_disp_name = std::format("{:4} {}", prop.m_index, prop.get<Hkx::PropName>().text().as_string()); // :3
                          return !(prop.m_valid &&
                                   (search_text.empty() ||
                                    !std::ranges::search(prop_disp_name, search_text, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty()));
                      });

        ImGui::BeginChild("ITEMS", {400.0f, 20.0f * std::min(prop_list.size(), 30ull)}, false);
        {
            ImGuiListClipper clipper;
            clipper.Begin(prop_list.size());
            while (clipper.Step())
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                {
                    auto& prop = prop_list[row_n];

                    if (prop.m_valid)
                    {
                        auto prop_disp_name = fmt::format("{:4} {}", prop.m_index, prop.get<Hkx::PropName>().text().as_string());

                        if constexpr (std::is_same_v<Manager::Entry, Hkx::Variable>)
                        {
                            auto var_type      = prop.get<Hkx::PropVarInfo>().getByName("type").text().as_string();
                            auto var_type_enum = Hkx::getVarTypeEnum(var_type);
                            if (var_type_enum < 0)
                                ImGui::PushStyleColor(ImGuiCol_Text, g_color_invalid);
                            else if (var_type_enum < 1)
                                ImGui::PushStyleColor(ImGuiCol_Text, g_color_bool);
                            else if (var_type_enum < 4)
                                ImGui::PushStyleColor(ImGuiCol_Text, g_color_int);
                            else if (var_type_enum < 5)
                                ImGui::PushStyleColor(ImGuiCol_Text, g_color_float);
                            else if (var_type_enum < 6)
                                ImGui::PushStyleColor(ImGuiCol_Text, g_color_attr);
                            else
                                ImGui::PushStyleColor(ImGuiCol_Text, g_color_quad);
                        }

                        if (ImGui::Selectable(prop_disp_name.c_str()))
                        {
                            out = prop;
                            if constexpr (std::is_same_v<Manager::Entry, Hkx::Variable>)
                                ImGui::PopStyleColor();
                            ImGui::CloseCurrentPopup();
                            ImGui::EndPopup();
                            return true;
                        }

                        if constexpr (std::is_same_v<Manager::Entry, Hkx::Variable>)
                            ImGui::PopStyleColor();
                    }
                }
        }
        ImGui::EndChild();

        ImGui::EndPopup();
    }

    return false;
}

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
void flagIntPopup(const char* str_id, pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags)
{
    uint32_t value = hkparam.text().as_uint();
    if (ImGui::BeginPopup(str_id))
    {
        for (EnumWrapper flag : flags)
            if (flag.val)
            {
                ImGui::CheckboxFlags(flag.name.data(), &value, flag.val);
                if (!flag.hint.empty())
                    addTooltip(flag.hint.data());
            }
        hkparam.text() = value;
        ImGui::EndPopup();
    }
}

template <size_t N>
void flagEditButton(const char* str_id, pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags, bool as_int = false)
{
    if (as_int)
        flagIntPopup(str_id, hkparam, flags);
    else
        flagPopup(str_id, hkparam, flags);
    if (ImGui::Button(ICON_FA_FLAG))
        ImGui::OpenPopup(str_id);
    addTooltip("Edit individual flags");
}

void statePickerPopup(const char* str_id, pugi::xml_node hkparam, pugi::xml_node state_machine, Hkx::BehaviourFile& file);

std::optional<int16_t> bonePickerButton(Hkx::SkeletonFile& skel_file, Hkx::BehaviourFile& file, int16_t value);

void varBindingButton(const char* str_id, pugi::xml_node hkparam, Hkx::BehaviourFile& file);

////////////////    EDITS
inline void fullTableSeparator()
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    while (ImGui::TableGetColumnIndex() < ImGui::TableGetColumnCount() - 1)
    {
        ImGui::Separator();
        ImGui::TableNextColumn();
    }
    ImGui::Separator();
}

void stringEdit(pugi::xml_node   hkparam,
                std::string_view hint        = {},
                std::string_view manual_name = {});

void boolEdit(pugi::xml_node      hkparam,
              Hkx::BehaviourFile& file,
              std::string_view    hint        = {},
              std::string_view    manual_name = {});

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

void intScalarEdit(pugi::xml_node hkparam, Hkx::BehaviourFile& file, ImGuiDataType data_type = ImGuiDataType_S32, std::string_view hint = {}, std::string_view manual_name = {});

// void sliderIntEdit(pugi::xml_node   hkparam,
//                    int              lb,
//                    int              ub,
//                    std::string_view hint        = {},
//                    std::string_view manual_name = {});

template <size_t N>
void flagEdit(pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags, std::string_view hint = {}, std::string_view manual_name = {}, bool as_int = false)
{
    stringEdit(hkparam, hint, manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());
    flagEditButton("Edit flags", hkparam, flags, as_int);
    ImGui::PopID();
}

template <typename Manager>
void linkedPropPickerEdit(pugi::xml_node      hkparam,
                          Hkx::BehaviourFile& file,
                          Manager&            prop_manager,
                          std::string_view    hint        = {},
                          std::string_view    manual_name = {})
{
    intScalarEdit(hkparam, file, ImGuiDataType_S32, hint, manual_name);
    if (getParentObj(hkparam).getByName("variableBindingSet"))
        ImGui::SameLine();

    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    bool set_focus = false;
    if (ImGui::Button(ICON_FA_SEARCH))
    {
        set_focus = true;
        ImGui::OpenPopup("PickProp");
    }
    addTooltip("Select");
    if (hkparam.text().as_int() >= 0)
    {
        ImGui::SameLine();
        ImGui::TextUnformatted(prop_manager.getEntry(hkparam.text().as_ullong()).getName());
    }

    // POPUP
    typename Manager::Entry out_prop;
    if (linkedPropPickerPopup("PickProp", prop_manager, set_focus, out_prop))
        hkparam.text() = out_prop.m_index;

    ImGui::PopID();
}

void sliderFloatEdit(pugi::xml_node      hkparam,
                     float               lb,
                     float               ub,
                     Hkx::BehaviourFile& file,
                     std::string_view    hint        = {},
                     std::string_view    manual_name = {});

void floatEdit(pugi::xml_node      hkparam,
               Hkx::BehaviourFile& file,
               std::string_view    hint        = {},
               std::string_view    manual_name = {});

// for floats written in UINT32 values
void convertFloatEdit(pugi::xml_node   hkparam,
                      std::string_view hint        = {},
                      std::string_view manual_name = {});

void quadEdit(pugi::xml_node      hkparam,
              Hkx::BehaviourFile& file,
              std::string_view    hint        = {},
              std::string_view    manual_name = {});

void refEdit(pugi::xml_node                       hkparam,
             const std::vector<std::string_view>& classes, // should've used array but i don't wanna stuff too much templates here
             Hkx::BehaviourFile&                  file,
             std::string_view                     hint        = {},
             std::string_view                     manual_name = {});

bool refEdit(std::string&                         value,
             const std::vector<std::string_view>& classes,
             pugi::xml_node                       parent,
             Hkx::BehaviourFile&                  file,
             std::string_view                     hint,
             std::string_view                     manual_name);

void refList(pugi::xml_node                       hkparam,
             const std::vector<std::string_view>& classes,
             Hkx::BehaviourFile&                  file,
             std::string_view                     hint_attribute = {},
             std::string_view                     name_attribute = {},
             std::string_view                     manual_name    = {});

pugi::xml_node refLiveEditList(
    pugi::xml_node                       hkparam,
    const std::vector<std::string_view>& classes,
    Hkx::BehaviourFile&                  file,
    std::string_view                     hint_attribute = {},
    std::string_view                     name_attribute = {},
    std::string_view                     manual_name    = {});

void objLiveEditList(
    pugi::xml_node                             hkparam,
    pugi::xml_node&                            edit_item,
    const char*                                def_str,
    Hkx::BehaviourFile&                        file,
    std::string_view                           str_id,
    std::function<std::string(pugi::xml_node)> disp_name_func);

void stateEdit(pugi::xml_node      hkparam,
               Hkx::BehaviourFile& file,
               pugi::xml_node      state_machine = {},
               std::string_view    hint          = {},
               std::string_view    manual_name   = {});

void boneEdit(pugi::xml_node      hkparam,
              Hkx::BehaviourFile& file,
              Hkx::SkeletonFile&  skel_file,
              std::string_view    hint        = {},
              std::string_view    manual_name = {});

////////////////    Linked Prop Edits

void varEditPopup(const char* str_id, Hkx::Variable& var, Hkx::BehaviourFile& file);
void evtEditPopup(const char* str_id, Hkx::AnimationEvent& evt, Hkx::BehaviourFile& file);
void propEditPopup(const char* str_id, Hkx::CharacterProperty& evt, Hkx::BehaviourFile& file);

} // namespace Ui
} // namespace Haviour