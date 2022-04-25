// Widgets for editing all sorts of params.
// All edits must be used in a table context.
#pragma once
#include "hkx/hkxfile.h"

#include <fmt/format.h>
#include <pugixml.hpp>

namespace Haviour
{
namespace Ui
{
#define addTooltip(...) \
    if (ImGui::IsItemHovered()) ImGui::SetTooltip(__VA_ARGS__);

bool variablePickerPopup(const char*           str_id,
                         Hkx::VariableManager& var_manager,
                         Hkx::Variable&        out);

template <size_t N>
void flagPopup(const char*                       str_id,
               pugi::xml_node                    hkparam,
               const std::array<EnumWrapper, N>& flags);

template <size_t N>
void flagEditButton(const char*                       str_id,
                    pugi::xml_node                    hkparam,
                    const std::array<EnumWrapper, N>& flags);

////////////////    EDITS
void stringEdit(pugi::xml_node   hkparam,
                std::string_view manual_name = {});

void boolEdit(pugi::xml_node   hkparam,
              std::string_view manual_name = {});

template <size_t N>
void enumEdit(pugi::xml_node                    hkparam,
              const std::array<EnumWrapper, N>& enums,
              std::string_view                  manual_name = {});

void intEdit(pugi::xml_node   hkparam,
             std::string_view manual_name = {});

void sliderIntEdit(pugi::xml_node   hkparam,
                   int              lb,
                   int              ub,
                   std::string_view manual_name = {});

template <size_t N>
void flagEdit(pugi::xml_node                    hkparam,
              const std::array<EnumWrapper, N>& flags,
              std::string_view                  manual_name = {});

void variablePickerEdit(pugi::xml_node        hkparam,
                        Hkx::VariableManager& var_manager,
                        std::string_view      manual_name = {});

void sliderFloatEdit(pugi::xml_node   hkparam,
                     float            lb,
                     float            ub,
                     std::string_view manual_name = {});

void floatEdit(pugi::xml_node   hkparam,
               std::string_view manual_name = {});

// for floats written in UINT32 values
void convertFloatEdit(pugi::xml_node   hkparam,
                      std::string_view manual_name = {});

void quadEdit(pugi::xml_node   hkparam,
              std::string_view manual_name);

void refEdit(pugi::xml_node                       hkparam,
             const std::vector<std::string_view>& classes,
             Hkx::HkxFile&                        file,
             std::string_view                     manual_name = {});

////////////////    Linked Prop Edits

void varEditPopup(const char* str_id, Hkx::Variable& var, Hkx::HkxFile& file);
void evtEditPopup(const char* str_id, Hkx::AnimationEvent& evt, Hkx::HkxFile& file);
void propEditPopup(const char* str_id, Hkx::CharacterProperty& evt, Hkx::HkxFile& file);

} // namespace Ui
} // namespace Haviour