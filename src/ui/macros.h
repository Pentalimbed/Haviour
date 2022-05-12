#pragma once

#include "hkx/hkxfile.h"

#include <string>
#include <vector>
#include <memory>

#include <pugixml.hpp>

namespace Haviour
{
namespace Ui
{

class MacroModal
{
public:
    friend class MacroManager;

    virtual constexpr const char* getName()  = 0;
    virtual constexpr const char* getClass() = 0;
    virtual constexpr const char* getHint()  = 0;
    virtual void                  open(pugi::xml_node working_obj, Hkx::HkxFile* file);
    virtual void                  show();
    virtual void                  drawUi() = 0;

protected:
    bool           m_open = false;
    pugi::xml_node m_working_obj;
    Hkx::HkxFile*  m_file; // potential leak here, but it's modal so ok?
};

class TriggerMacro : public MacroModal
{
public:
    virtual void                  open(pugi::xml_node working_obj, Hkx::HkxFile* file) override;
    virtual constexpr const char* getName() override { return "Parse Trigger"; }
    virtual constexpr const char* getClass() override { return "hkbClipTriggerArray"; }
    virtual constexpr const char* getHint() override
    {
        return "This macro will parse text and populate hkbClipTriggerArray from it.\n\n"
               "Format: eventName (+/-)localTime\n\n"
               "Example:\n"
               "EventA 0.01 // trigger EventA at 0.01 sec\n"
               "EventB -1.2 // trigger EventB 1.2 sec before clip ends\n"
               "EventC +100 // plus symbol indicates acyclic trigger so that the time don't get clamped\n"
               "...";
    }
    virtual void drawUi() override;

private:
    std::string m_parse_text;
    bool        m_create_new_event = false;
    bool        m_replace          = false;

    void parse();
};

/////////////////////////// MACRO MANAGER

class MacroManager
{
public:
    static MacroManager* getSingleton();

    void show();

private:
    std::vector<std::unique_ptr<MacroModal>> m_macros;
    Hkx::HkxFileManager::Handle              m_file_listener;

    MacroManager();

public:
    inline const decltype(m_macros)& getMacros() { return m_macros; }
};

} // namespace Ui
} // namespace Haviour
