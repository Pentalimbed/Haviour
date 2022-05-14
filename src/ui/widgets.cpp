#include "widgets.h"
#include "hkx/hkclass.inl"
#include "propedit.h"
#include "hkx/hkutils.h"

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace Haviour
{
namespace Ui
{

bool iconButton(const char* label)
{
    return ImGui::Button(label, ImVec2(10, 10));
}

std::optional<pugi::xml_node> statePickerPopup(const char* str_id, pugi::xml_node state_machine, Hkx::HkxFile& file, int selected_state_id, bool just_open)
{
    if (ImGui::IsPopupOpen(str_id))
    {
        auto                        states_node = state_machine.getByName("states");
        size_t                      num_objs    = states_node.attribute("numelements").as_ullong();
        std::istringstream          states_stream(states_node.text().as_string());
        std::vector<pugi::xml_node> states(num_objs);

        for (size_t i = 0; i < num_objs; ++i)
        {
            std::string temp_str;
            states_stream >> temp_str;
            states[i] = file.getObj(temp_str);
        }

        return pickerPopup<pugi::xml_node>(
            str_id, states,
            [](auto& state_obj, std::string_view filter_str) { 
                auto state_id = state_obj.getByName("stateId").text().as_int();
                return hasText(fmt::format("{:4} {}", state_id, state_obj.getByName("name").text().as_string()), filter_str); },
            [=](auto& state_obj) {
                auto state_id = state_obj.getByName("stateId").text().as_int();
                return ImGui::Selectable(fmt::format("{:4} {}", state_id, state_obj.getByName("name").text().as_string()).c_str(), selected_state_id == state_id);
            },
            just_open);
    }
    return std::nullopt;
}

std::optional<int16_t> bonePickerPopup(const char* str_id, Hkx::SkeletonFile& skel_file, int16_t selected_bone_id, bool just_open)
{
    static bool use_ragdoll_skel = false;

    if (ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
    {
        if (skel_file.isFileLoaded())
        {
            ImGui::Checkbox("Ragdoll Skeleton", &use_ragdoll_skel);
            if (just_open)
                ImGui::SetKeyboardFocusHere(); // set focus to keyboard

            auto                                           bone_node = skel_file.getBoneNode(use_ragdoll_skel);
            std::vector<std::pair<size_t, pugi::xml_node>> bone_list(bone_node.attribute("numelements").as_ullong());
            size_t                                         idx = 0;
            for (auto bone : bone_node.children())
            {
                bone_list[idx] = {idx, bone};
                ++idx;
            }

            auto res = filteredPickerListBox<std::pair<size_t, pugi::xml_node>>(
                fmt::format("##{}", str_id).c_str(),
                bone_list,
                [=](auto& pair, std::string_view text) { return hasText(pair.second.getByName("name").text().as_string(), text); },
                [=](auto& pair) { return ImGui::Selectable(fmt::format("{:4} {}", pair.first, pair.second.getByName("name").text().as_string()).c_str(), selected_bone_id == pair.first); });

            if (res.has_value())
            {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return res.value().first;
            }
        }
        else
        {
            ImGui::TextDisabled("No loaded skeleton file.");
        }

        ImGui::EndPopup();
    }
    return std::nullopt;
}

std::optional<std::string_view> animPickerPopup(const char* str_id, Hkx::CharacterFile& char_file, std::string_view selected_anim, bool just_open)
{
    if (ImGui::IsPopupOpen(str_id))
    {
        auto                          anim_names_node = char_file.getAnimNames();
        std::vector<std::string_view> anim_list(anim_names_node.attribute("numelements").as_ullong());
        size_t                        idx = 0;
        for (auto anim_name : anim_names_node.children())
            anim_list[idx++] = anim_name.text().as_string();

        return pickerPopup<std::string_view>(
            str_id, anim_list,
            [](auto& name, std::string_view filter_str) { return hasText(name, filter_str); },
            [=](auto& name) { return ImGui::Selectable(name.data(), name == selected_anim); },
            just_open);
    }
    return std::nullopt;
}

void varBindingButton(const char* str_id, pugi::xml_node hkparam, Hkx::BehaviourFile& file)
{
    static pugi::xml_node param_cache;
    static pugi::xml_node bindings     = {};
    static std::string    param_path   = {};
    static pugi::xml_node prev_binding = {};

    auto binding_set = getParentObj(hkparam).getByName("variableBindingSet");

    ImGui::PushID(str_id);
    if (ImGui::Button(ICON_FA_LINK))
    {
        bindings   = file.getObj(binding_set.text().as_string()).getByName("bindings");
        param_path = getParamPath(hkparam);
        // auto prev_bindings = bindings.select_nodes(fmt::format("/hkparam/hkobject/hkparam[@name='memberPath' and text()='{}']", param_path).c_str()); // fuck xpath
        prev_binding = bindings.find_node([](auto node) { return !strcmp(node.attribute("name").as_string(), "memberPath") && (param_path == node.text().as_string()); }).parent();
        ImGui::OpenPopup("bindmenu");
    }
    addTooltip("Edit binding");
    ImGui::PopID();

    bool open_var  = false;
    bool open_prop = false;
    if (ImGui::BeginPopup("bindmenu", ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (binding_set)
        {
            if (prev_binding)
            {
                std::string var_name;
                auto        varidx = prev_binding.getByName("variableIndex").text().as_string();
                if (!strcmp(prev_binding.getByName("bindingType").text().as_string(), "BINDING_TYPE_VARIABLE"))
                    var_name = file.m_var_manager.getEntry(prev_binding.getByName("variableIndex").text().as_int()).getName();
                else
                    var_name = file.m_prop_manager.getEntry(prev_binding.getByName("variableIndex").text().as_int()).getName();
                ImGui::TextUnformatted(ICON_FA_LINK);
                ImGui::SameLine();
                ImGui::TextUnformatted("Bound with:");
                ImGui::TextUnformatted(var_name.c_str());

                if (ImGui::BeginTable("bit", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ScalarEdit<ImGuiDataType_S8>(prev_binding.getByName("bitIndex"), nullptr,
                                                 "The index of the bit to which the variable is bound.\n"
                                                 "A value of -1 indicates that we are not binding to an individual bit.")();
                    ImGui::EndTable();
                }
            }
            else
                ImGui::TextDisabled("Not bound with any variable.");

            ImGui::Separator();

            if (ImGui::Selectable("Bind Varaibale"))
                open_var = true;
            if (ImGui::Selectable("Bind Character Property"))
                open_prop = true;
        }
        else
        {
            ImGui::TextDisabled("This object is unbindable.");
        }

        ImGui::EndPopup();
    }

    if (binding_set)
    {
        bool set_focus = false;
        if (open_var)
        {
            set_focus = true;
            ImGui::OpenPopup("bindvar");
        }
        if (open_prop)
        {
            set_focus = true;
            ImGui::OpenPopup("bindprop");
        }

        auto var  = linkedPropPickerPopup("bindvar", file.m_var_manager, set_focus);
        auto prop = linkedPropPickerPopup("bindprop", file.m_prop_manager, set_focus);

        if (var.has_value() || prop.has_value())
        {
            auto parent = getParentObj(hkparam);
            if (!bindings)
            {
                auto bindings_id = file.addObj("hkbVariableBindingSet");
                file.addRef(bindings_id, parent.attribute("name").as_string());
                binding_set.text() = bindings_id.data();
                bindings           = file.getObj(bindings_id).getByName("bindings");
            }
            if (!prev_binding)
            {
                prev_binding                                = appendXmlString(bindings, Hkx::g_def_hkbVariableBindingSet_Binding);
                prev_binding.getByName("memberPath").text() = param_path.c_str();
            }
            if (var.has_value())
            {
                prev_binding.getByName("variableIndex").text() = var.value().m_index;
                prev_binding.getByName("bindingType").text()   = "BINDING_TYPE_VARIABLE";
            }
            else
            {
                prev_binding.getByName("variableIndex").text() = prop.value().m_index;
                prev_binding.getByName("bindingType").text()   = "BINDING_TYPE_CHARACTER_PROPERTY";
            }
            if ((param_path == "enable") && std::string_view(parent.attribute("class").as_string()).contains("Modifier"))
                bindings.parent().getByName("indexOfBindingToEnable").text() = getChildIndex(prev_binding);
        }
    }
}

void varBindingButton(const char* str_id, pugi::xml_node hkparam, Hkx::BehaviourFile* file)
{
    static pugi::xml_node param_cache;
    static pugi::xml_node bindings     = {};
    static std::string    param_path   = {};
    static pugi::xml_node prev_binding = {};

    auto binding_set = getParentObj(hkparam).getByName("variableBindingSet");

    ImGui::PushID(str_id);
    if (ImGui::Button(ICON_FA_LINK))
    {
        if (file && binding_set)
        {
            bindings     = file->getObj(binding_set.text().as_string()).getByName("bindings");
            param_path   = getParamPath(hkparam);
            prev_binding = bindings.find_node([](auto node) { return !strcmp(node.attribute("name").as_string(), "memberPath") && (param_path == node.text().as_string()); }).parent();
        }
        ImGui::OpenPopup("bindmenu");
    }
    addTooltip("Edit binding");

    bool open_var  = false;
    bool open_prop = false;
    if (ImGui::BeginPopup("bindmenu", ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (file && binding_set)
        {
            if (prev_binding)
            {
                std::string var_name;
                auto        varidx = prev_binding.getByName("variableIndex").text().as_string();
                if (!strcmp(prev_binding.getByName("bindingType").text().as_string(), "BINDING_TYPE_VARIABLE"))
                    var_name = file->m_var_manager.getEntry(prev_binding.getByName("variableIndex").text().as_int()).getName();
                else
                    var_name = file->m_prop_manager.getEntry(prev_binding.getByName("variableIndex").text().as_int()).getName();
                ImGui::TextUnformatted(ICON_FA_LINK);
                ImGui::SameLine();
                ImGui::TextUnformatted("Bound with:");
                ImGui::TextUnformatted(var_name.c_str());

                if (ImGui::BeginTable("bit", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ScalarEdit<ImGuiDataType_S8>(prev_binding.getByName("bitIndex"), nullptr,
                                                 "The index of the bit to which the variable is bound.\n"
                                                 "A value of -1 indicates that we are not binding to an individual bit.")();
                    ImGui::EndTable();
                }
            }
            else
                ImGui::TextDisabled("Not bound with any variable.");

            ImGui::Separator();

            if (ImGui::Selectable("Bind Varaibale"))
                open_var = true;
            if (ImGui::Selectable("Bind Character Property"))
                open_prop = true;
        }
        else
        {
            ImGui::TextDisabled("This object is unbindable.");
        }

        ImGui::EndPopup();
    }

    if (file && binding_set)
    {
        bool set_focus = false;
        if (open_var)
        {
            set_focus = true;
            ImGui::OpenPopup("bindvar");
        }
        if (open_prop)
        {
            set_focus = true;
            ImGui::OpenPopup("bindprop");
        }

        auto var  = linkedPropPickerPopup("bindvar", file->m_var_manager, set_focus);
        auto prop = linkedPropPickerPopup("bindprop", file->m_prop_manager, set_focus);

        if (var.has_value() || prop.has_value())
        {
            auto parent = getParentObj(hkparam);
            if (!bindings)
            {
                auto bindings_id = file->addObj("hkbVariableBindingSet");
                file->addRef(bindings_id, parent.attribute("name").as_string());
                binding_set.text() = bindings_id.data();
                bindings           = file->getObj(bindings_id).getByName("bindings");
            }
            if (!prev_binding)
            {
                prev_binding                                = appendXmlString(bindings, Hkx::g_def_hkbVariableBindingSet_Binding);
                prev_binding.getByName("memberPath").text() = param_path.c_str();
            }
            if (var.has_value())
            {
                prev_binding.getByName("variableIndex").text() = var.value().m_index;
                prev_binding.getByName("bindingType").text()   = "BINDING_TYPE_VARIABLE";
            }
            else
            {
                prev_binding.getByName("variableIndex").text() = prop.value().m_index;
                prev_binding.getByName("bindingType").text()   = "BINDING_TYPE_CHARACTER_PROPERTY";
            }
            if ((param_path == "enable") && std::string_view(parent.attribute("class").as_string()).contains("Modifier"))
                bindings.parent().getByName("indexOfBindingToEnable").text() = getChildIndex(prev_binding);
        }
    }
    ImGui::PopID();
}

////////////////    hkParam Edits

bool ParamEdit::showButton()
{
    varBindingButton(getName(), m_hkparam,
                     (m_file && (m_file->getType() == Hkx::HkxFile::kBehaviour)) ? dynamic_cast<Hkx::BehaviourFile*>(m_file) : nullptr);
    return false;
}

void BoolEdit::fetchValue() { m_value = m_hkparam.text().as_bool(); }
void BoolEdit::updateValue() { m_hkparam.text() = m_value ? "true" : "false"; }
bool BoolEdit::showEdit() { return ImGui::Checkbox(getName(), &m_value); }

void QuadEdit::fetchValue()
{
    std::string text(m_hkparam.text().as_string());
    std::erase_if(text, [](char& c) { return (c == '(') || (c == ')'); });
    std::istringstream text_stream(text);
    text_stream >> m_value[0] >> m_value[1] >> m_value[2] >> m_value[3];
}
void QuadEdit::updateValue() { m_hkparam.text() = fmt::format("({:.6f} {:.6f} {:.6f} {:.6f})", m_value[0], m_value[1], m_value[2], m_value[3]).c_str(); }
bool QuadEdit::showEdit() { return ImGui::InputFloat4(getName(), m_value, "%.6f"); }

void FloatAsIntEdit::fetchValue()
{
    auto raw_value = m_hkparam.text().as_ullong();
    m_value        = *reinterpret_cast<float*>(&raw_value);
}
void FloatAsIntEdit::updateValue() { m_hkparam.text() = *reinterpret_cast<uint32_t*>(&m_value); }

void StringEdit::fetchValue() { m_value = m_hkparam.text().as_string(); }
void StringEdit::updateValue() { m_hkparam.text() = m_value.c_str(); }
bool StringEdit::showEdit() { return ImGui::InputText(getName(), &m_value); }

bool AnimEdit::showButton()
{
    ParamEdit::showButton();
    ImGui::SameLine();
    auto res = animPickerButton("picker", Hkx::HkxFileManager::getSingleton()->m_char_file, m_value);
    if (res.has_value())
        m_hkparam.text() = res.value().data();
    return res.has_value();
}

bool StateEdit::showButton()
{
    ParamEdit::showButton();
    ImGui::SameLine();
    auto res = statePickerButton(getName(), m_state_machine, *m_file, m_value);
    if (res.has_value())
        m_value = res.value().getByName("stateId").text().as_int();
    if (auto state = getStateById(m_state_machine, m_value, *m_file); state)
    {
        ImGui::SameLine();
        ImGui::TextUnformatted(state.getByName("name").text().as_string());
    }
    return res.has_value();
}

bool BoneEdit::showButton()
{
    ParamEdit::showButton();
    ImGui::SameLine();
    auto res = bonePickerButton(getName(), Hkx::HkxFileManager::getSingleton()->m_skel_file, m_value);
    if (res.has_value())
        m_value = res.value();
    return res.has_value();
}

void setPropObj(std::string_view value) { PropEdit::getSingleton()->setObject(value); }

////////////////  EDITS

bool refEdit(std::string&                         value,
             const std::vector<std::string_view>& classes,
             pugi::xml_node                       parent,
             Hkx::HkxFile&                        file,
             std::string_view                     hint,
             std::string_view                     manual_name)
{
    ImGui::TableNextColumn();

    ImGui::PushID(manual_name.data());

    bool        edited    = false;
    std::string old_value = value;

    if (ImGui::Button(ICON_FA_ARROW_CIRCLE_RIGHT))
        PropEdit::getSingleton()->setObject(value);
    addTooltip("Go to");

    ImGui::SameLine();
    if (ImGui::BeginPopup("Append"))
    {
        auto& class_info = Hkx::getClassInfo();
        for (auto class_name : classes)
        {
            if (!Hkx::getClassDefaultMap().contains(class_name))
                ImGui::BeginDisabled();
            if (ImGui::Selectable(class_name.data()))
            {
                value  = file.addObj(class_name);
                edited = true;
                ImGui::CloseCurrentPopup();
                break;
            }
            if (!Hkx::getClassDefaultMap().contains(class_name))
                ImGui::EndDisabled();
            if (class_info.contains(class_name))
                addTooltip(class_info.at(class_name).data());
        }
        ImGui::EndPopup();
    }
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        ImGui::OpenPopup("Append");
    addTooltip("Append");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_MINUS_CIRCLE))
    {
        ImGui::PopID();
        return true;
    }
    addTooltip("Remove");

    ImGui::TableNextColumn();

    ImGui::SetNextItemWidth(60);
    if (ImGui::InputText(manual_name.data(), &value))
    {
        edited = true;
        if (auto obj = file.getObj(value); obj) // Is obj
        {
            if (!classes.empty() && std::ranges::find(classes, std::string_view(obj.attribute("class").as_string())) == classes.end()) // Different class
                value = "null";
        }
        else
            value = "null";
    }
    if (!hint.empty())
        addTooltip(hint.data());

    if (edited)
    {
        auto parent_id = parent.attribute("name").as_string();
        if (!isRefBy(old_value, parent))
            file.deRef(old_value, parent_id);
        file.addRef(value, parent_id);
        // for states
        if (auto obj = file.getObj(value); !strcmp(obj.attribute("class").as_string(), "hkbStateMachineStateInfo"))
            if (auto state_machine = getParentStateMachine(obj, file); state_machine.getByName("states").attribute("numelements").as_uint() > 1)
                obj.getByName("stateId").text() = getBiggestStateId(state_machine, file) + 1;
    }

    ImGui::PopID();

    return false;
}

void refList(pugi::xml_node                       hkparam,
             const std::vector<std::string_view>& classes,
             Hkx::HkxFile&                        file,
             std::string_view                     hint_attribute,
             std::string_view                     name_attribute,
             std::string_view                     manual_name)
{
    auto                     parent   = getParentObj(hkparam);
    std::vector<std::string> objs     = {};
    size_t                   num_objs = hkparam.attribute("numelements").as_ullong();
    std::istringstream       states_stream(hkparam.text().as_string());
    for (size_t i = 0; i < num_objs; ++i)
    {
        std::string temp_str;
        states_stream >> temp_str;
        objs.push_back(temp_str);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data()), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        objs.push_back("null");
    addTooltip("Add new object reference");
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(hkparam.attribute("numelements").as_string());

    constexpr auto table_flag = ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
    if (ImGui::BeginTable("states", 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        size_t mark_delete = UINT64_MAX;
        bool   do_delete   = false;
        for (size_t i = 0; i < objs.size(); ++i)
        {
            ImGui::PushID(i);

            if (refEdit(objs[i], classes, parent, file,
                        hint_attribute.empty() ? "" : file.getObj(objs[i]).getByName(hint_attribute.data()).text().as_string(),
                        name_attribute.empty() ? "" : file.getObj(objs[i]).getByName(name_attribute.data()).text().as_string()))
            {
                do_delete   = true;
                mark_delete = i;
            }

            ImGui::PopID();
        }
        if (do_delete)
            objs.erase(objs.begin() + mark_delete);

        hkparam.text()                   = printVector(objs).c_str();
        hkparam.attribute("numelements") = objs.size();

        ImGui::EndTable();
    }
}

pugi::xml_node refLiveEditList(
    pugi::xml_node                       hkparam,
    const std::vector<std::string_view>& classes,
    Hkx::HkxFile&                        file,
    std::string_view                     hint_attribute,
    std::string_view                     name_attribute,
    std::string_view                     manual_name)
{
    auto                     parent   = getParentObj(hkparam);
    std::vector<std::string> objs     = {};
    size_t                   num_objs = hkparam.attribute("numelements").as_ullong();
    std::istringstream       states_stream(hkparam.text().as_string());
    for (size_t i = 0; i < num_objs; ++i)
    {
        std::string temp_str;
        states_stream >> temp_str;
        objs.push_back(temp_str);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data()), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
    {
        hkparam.attribute("numelements") = hkparam.attribute("numelements").as_int() + 1;
        objs.push_back("null");
    }
    addTooltip("Add new object reference");
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(hkparam.attribute("numelements").as_string());

    constexpr auto table_flag = ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
    if (ImGui::BeginTable("states", 3, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        size_t mark_delete = UINT64_MAX;
        bool   do_delete   = false;
        for (size_t i = 0; i < objs.size(); ++i)
        {
            auto obj = Hkx::HkxFileManager::getSingleton()->getCurrentFile()->getObj(objs[i]);

            ImGui::PushID(i);

            ImGui::TableNextColumn();
            if (ImGui::Button(ICON_FA_PEN))
            {
                ImGui::PopID();
                ImGui::EndTable();
                return obj;
            }
            addTooltip("Edit here");

            if (refEdit(objs[i], classes, parent, file,
                        obj.getByName(hint_attribute.data()).text().as_string(),
                        obj.getByName(name_attribute.data()).text().as_string()))
            {
                do_delete   = true;
                mark_delete = i;
            }

            ImGui::PopID();
        }
        if (do_delete)
        {
            objs.erase(objs.begin() + mark_delete);
            hkparam.attribute("numelements") = hkparam.attribute("numelements").as_int() - 1;
        }

        hkparam.text() = printVector(objs).c_str();

        ImGui::EndTable();
    }
    return {};
}

void objLiveEditList(
    pugi::xml_node                             hkparam,
    pugi::xml_node&                            edit_item,
    const char*                                def_str,
    std::string_view                           str_id,
    std::function<std::string(pugi::xml_node)> disp_name_func)
{
    ImGui::AlignTextToFramePadding();
    ImGui::BulletText(str_id.data()), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        appendXmlString(hkparam, def_str);
    addTooltip("Add new item");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_MINUS_CIRCLE) && edit_item)
        if (hkparam.remove_child(edit_item))
        {
            edit_item                        = {};
            hkparam.attribute("numelements") = hkparam.attribute("numelements").as_int() - 1;
        }
    addTooltip("Remove currently editing item.");
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(hkparam.attribute("numelements").as_string());

    if (ImGui::BeginListBox(fmt::format("##{}", str_id).c_str(), ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        size_t idx = 0;
        for (auto item : hkparam.children())
        {
            const bool is_selected = edit_item == item;
            auto       disp_name   = disp_name_func(item);
            ImGui::PushID(idx++);
            if (ImGui::Selectable(disp_name.c_str(), is_selected))
                edit_item = item;
            ImGui::PopID();
        }
        ImGui::EndListBox();
    }
}

////////////////    Linked Prop Edits

void varEditPopup(const char*                           str_id,
                  Hkx::Variable&                        var,
                  Hkx::VariableManager&                 manager,
                  std::function<pugi::xml_node(size_t)> get_ref_fn,
                  Hkx::HkxFile&                         file)
{
    if (ImGui::BeginPopup(str_id))
    {
        if (ImGui::BeginTable("varedit", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(600, -FLT_MIN)))
        {
            StringEdit(var.get<Hkx::PropName>()).name("name")();
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_TRASH))
            {
                auto name     = var.get<Hkx::PropName>().text().as_string();
                auto ref_node = get_ref_fn(var.m_index);
                if (ref_node)
                {
                    spdlog::warn("This variable is still referenced by {} and potentially more object.\nID copied to clipboard.", ref_node.attribute("name").as_string());
                    ImGui::SetClipboardText(ref_node.attribute("name").as_string());
                }
                else
                {
                    manager.delEntry(var.m_index);
                    spdlog::info("Variable {} deleted!", name);
                    ImGui::EndTable();
                    ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                    return;
                }
            }
            addTooltip("Delete/Mark vairable obsolete\nIf it is still referenced, the variable won't delete\nand one of its referencing object will be copied to clipboard.");

            auto var_type_enum  = Hkx::getVarTypeEnum(var.get<Hkx::PropVarInfo>().getByName("type").text().as_string());
            auto var_value_node = var.get<Hkx::PropWordValue>().first_child();
            switch (var_type_enum)
            {
                case Hkx::VARIABLE_TYPE_BOOL:
                    BoolEdit{var_value_node}();
                    break;
                case Hkx::VARIABLE_TYPE_INT8:
                    ScalarEdit<ImGuiDataType_S8>{var_value_node}();
                    break;
                case Hkx::VARIABLE_TYPE_INT16:
                    ScalarEdit<ImGuiDataType_S16>{var_value_node}();
                    break;
                case Hkx::VARIABLE_TYPE_INT32:
                    ScalarEdit<ImGuiDataType_S32>{var_value_node}();
                    break;
                case Hkx::VARIABLE_TYPE_REAL:
                    FloatAsIntEdit{var_value_node}();
                    break;
                case Hkx::VARIABLE_TYPE_POINTER:
                    ImGui::TableNextColumn();
                    if (ImGui::InputText("value", &manager.getPointerValue(var_value_node.text().as_uint())))
                        file.buildRefList(getParentObj(var.get<Hkx::PropWordValue>()).attribute("name").as_string());
                    ImGui::TableNextColumn();
                    break;
                case Hkx::VARIABLE_TYPE_VECTOR3:
                    ImGui::TableNextColumn();
                    ImGui::InputFloat3("value", manager.getQuadValue(var_value_node.text().as_uint()).data(), "%.6f");
                    ImGui::TableNextColumn();
                    break;
                case Hkx::VARIABLE_TYPE_VECTOR4:
                case Hkx::VARIABLE_TYPE_QUATERNION:
                    ImGui::TableNextColumn();
                    ImGui::InputFloat4("value", manager.getQuadValue(var_value_node.text().as_uint()).data(), "%.6f");
                    ImGui::TableNextColumn();
                    break;
                default:
                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImColor(1.0f, 0.3f, 0.3f, 1.0f), "Invalid value type!");
                    ImGui::TableNextColumn();
                    break;
            }

            auto role_node = var.get<Hkx::PropVarInfo>().getByName("role").first_child();
            EnumEdit<Hkx::e_hkbRoleAttribute_Role>(role_node.getByName("role"), file)();
            FlagEdit<Hkx::f_hkbRoleAttribute_roleFlags>(role_node.getByName("flags"))();

            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }
}

void evtEditPopup(const char* str_id, Hkx::AnimationEvent& evt, Hkx::BehaviourFile& file)
{
    if (ImGui::BeginPopup(str_id))
    {
        if (ImGui::BeginTable("eventedit", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(300.0, 0.0)))
        {
            StringEdit(evt.get<Hkx::PropName>()).name("name")();
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_TRASH))
            {
                auto name     = evt.get<Hkx::PropName>().text().as_string();
                auto ref_node = file.getFirstEventRef(evt.m_index);
                if (ref_node)
                {
                    spdlog::warn("This event is still referenced by {} and potentially more object.\nID copied to clipboard.", ref_node.attribute("name").as_string());
                    ImGui::SetClipboardText(ref_node.attribute("name").as_string());
                }
                else
                {
                    file.m_evt_manager.delEntry(evt.m_index);
                    spdlog::info("Event {} deleted!", name);
                    ImGui::EndTable();
                    ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                    return;
                }
            }
            addTooltip("Delete\nIf it is still referenced, the event won't delete\nand one of its referencing object will be copied to clipboard.");

            FlagEdit<Hkx::f_hkbEventInfo_Flags>(evt.get<Hkx::PropEventInfo>().getByName("flags"))();

            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }
}

void propEditPopup(const char* str_id, Hkx::CharacterProperty& prop, Hkx::BehaviourFile& file)
{
    if (ImGui::BeginPopup(str_id))
    {
        if (ImGui::BeginTable("eventedit", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(300.0, 0.0)))
        {
            StringEdit(prop.get<Hkx::PropName>()).name("name")();
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_TRASH))
            {
                auto name     = prop.get<Hkx::PropName>().text().as_string();
                auto ref_node = file.getFirstEventRef(prop.m_index);
                if (ref_node)
                {
                    spdlog::warn("This property is still referenced by {} and potentially more object.\nID copied to clipboard.", ref_node.attribute("name").as_string());
                    ImGui::SetClipboardText(ref_node.attribute("name").as_string());
                }
                else
                {
                    file.m_evt_manager.delEntry(prop.m_index);
                    spdlog::info("Event {} deleted!", name);
                    ImGui::EndTable();
                    ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                    return;
                }
            }
            addTooltip("Delete\nIf it is still referenced, the property won't delete\nand one of its referencing object will be copied to clipboard.");

            EnumEdit<Hkx::e_variableTypeEnum>(prop.get<Hkx::PropVarInfo>().getByName("type"))();
            auto role_node = prop.get<Hkx::PropVarInfo>().getByName("role").first_child();
            EnumEdit<Hkx::e_hkbRoleAttribute_Role>(role_node.getByName("role"))();
            FlagEdit<Hkx::f_hkbRoleAttribute_roleFlags>(role_node.getByName("flags"))();

            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }
}

} // namespace Ui
} // namespace Haviour