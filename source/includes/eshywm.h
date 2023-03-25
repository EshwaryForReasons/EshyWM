#pragma once

#include "config.h"
#include "window_manager.h"

#include <string>
#include <memory>
#include <iostream>

#define TASKBAR               EshyWM::taskbar
#define SWITCHER              EshyWM::switcher
#define CONTEXT_MENU          EshyWM::context_menu
#define RUN_MENU              EshyWM::run_menu

#define DISPLAY_X(i)          WindowManager::monitor_info[i].x
#define DISPLAY_Y(i)          WindowManager::monitor_info[i].y
#define DISPLAY_WIDTH(i)      WindowManager::monitor_info[i].width
#define DISPLAY_HEIGHT(i)     (WindowManager::monitor_info[i].height - EshyWMConfig::taskbar_height)

#define DISPLAY               WindowManager::display
#define ROOT                  DefaultRootWindow(WindowManager::display)

class EshyWMTaskbar;
class EshyWMSwitcher;
class EshyWMContextMenu;
class EshyWMRunMenu;

namespace EshyWM
{
    bool initialize();
    void on_screen_resolution_changed(uint new_width, uint new_height);

    extern std::shared_ptr<EshyWMTaskbar> taskbar;
    extern std::shared_ptr<EshyWMSwitcher> switcher;
    extern std::shared_ptr<EshyWMContextMenu> context_menu;
    extern std::shared_ptr<EshyWMRunMenu> run_menu;
};