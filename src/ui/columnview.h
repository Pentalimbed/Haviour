#pragma once

#include "hkx/hkxfile.h"

#include <vector>
#include <imgui.h>

namespace Haviour
{
namespace Ui
{
struct Column
{
    std::string m_selected = {};
};

class ColumnView
{
public:
    static ColumnView* getSingleton();
    void               show();
    bool               m_show = true;

private:
    ColumnView();
    ~ColumnView();

    Hkx::HkxFileManager::Handle m_file_listener;

    std::vector<Column> m_columns;

    StringSet          m_class_show;
    StringMap<ImColor> m_class_color_map;
};
} // namespace Ui
} // namespace Haviour