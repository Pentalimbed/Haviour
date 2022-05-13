#pragma once

#include "hkx/hkxfile.h"

namespace Haviour
{
namespace Ui
{
class VarEdit
{
public:
    static VarEdit* getSingleton();

    void show();

    bool m_show = true;

private:
    Hkx::HkxFileManager::Handle m_file_listener;

    enum ListShowEnum : int
    {
        kShowVariables,
        kShowProperties,
        kShowAttributes
    };
    int m_show_list;

    std::string            m_var_filter = {}, m_evt_filter = {}, m_prop_filter = {}, m_charprop_filter = {}, m_anim_filter = {};
    Hkx::Variable          m_var_current = {}, m_charprop_current = {};
    Hkx::AnimationEvent    m_evt_current  = {};
    Hkx::CharacterProperty m_prop_current = {};
    // behaviour file
    void showVarList();
    void showEvtList();
    void showPropList();
    // character file
    void showCharPropList();
    void showAnimList();
};
} // namespace Ui
} // namespace Haviour