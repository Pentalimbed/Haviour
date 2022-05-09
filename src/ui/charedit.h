#pragma once

#include "hkx/linkedmanager.h"

namespace Haviour
{
namespace Ui
{
class CharEdit
{
public:
    static CharEdit* getSingleton();

    void show();

    bool m_show = false;

private:
    std::string   m_prop_filter  = {};
    Hkx::Variable m_prop_current = {};

    void showPropList();
};
} // namespace Ui
} // namespace Haviour