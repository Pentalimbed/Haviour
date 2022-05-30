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
    StringSet m_selected;
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

    std::string m_nav_edit_str;

    float m_col_width   = 200.0f;
    float m_item_height = 27.0f;
    bool  m_align_child = true;

    void showColumns();

    void expandChildren(std::string_view obj, size_t col);
};
} // namespace Ui
} // namespace Haviour