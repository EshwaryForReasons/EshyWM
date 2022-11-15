#include "eshywm.h"
#include "window_manager.h"
#include "config.h"
#include "context_menu.h"
#include "taskbar.h"
#include "switcher.h"
#include "run_menu.h"

#include <glog/logging.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>

enum var_type
{
    VT_UINT,
    VT_ULONG,
    VT_STRING
};

template<class T>
struct key_value_pair
{
    T key;
    T value;
};

static key_value_pair<std::string> split(const std::string& s, char delimiter)
{
    const size_t pos = s.find(delimiter);
    return {s.substr(0, pos).c_str(), s.substr(pos + 1, s.length() - pos).c_str()};
}

static void parse_config_option(std::string line, var_type type, void* config_var, std::string option_name, std::string start_location = " ")
{
    if(line.find(option_name) != std::string::npos)
    {
        const std::size_t spos = line.find(start_location);

        switch(type)
        {
        case var_type::VT_UINT:
            *((uint*)config_var) = std::stoi(line.substr(spos, line.length() - spos));
            break;
        case var_type::VT_ULONG:
            *((ulong*)config_var) = std::stoul(line.substr(spos, line.length() - spos));
            break;
        case var_type::VT_STRING:
            *(std::string*)config_var = line.substr(spos, line.length() - spos);
            break;
        };
    }
}


bool EshyWM::initialize()
{
    update_config();
    update_background();
    run_startup_commands();

    window_manager = WindowManager::Create();
    if(!window_manager)
    {
        return false;
    }

    window_manager->initialize();

#define CENTER_W(w)       (DISPLAY_WIDTH - w) / 2
#define CENTER_H(h)       (DISPLAY_HEIGHT - h) / 2

    context_menu = std::make_shared<EshyWMContextMenu>(rect{0, 0, CONFIG->context_menu_width, 150}, CONFIG->context_menu_color);
    switcher = std::make_shared<EshyWMSwitcher>(rect{CENTER_W(CONFIG->switcher_width), CENTER_H(CONFIG->switcher_height), CONFIG->switcher_width, CONFIG->switcher_height}, CONFIG->switcher_color);
    run_menu = std::make_shared<EshyWMRunMenu>(rect{CENTER_W(CONFIG->run_menu_width), CENTER_H(CONFIG->run_menu_height), CONFIG->run_menu_width, CONFIG->run_menu_height}, CONFIG->run_menu_color);

#undef CENTER_W
#undef CENTER_H

    //Create taskbar
    taskbar = std::make_shared<EshyWMTaskbar>(rect{0, DISPLAY_HEIGHT, DISPLAY_WIDTH, CONFIG->taskbar_height}, CONFIG->taskbar_color);
    taskbar->show();

    window_manager->handle_preexisting_windows();
    for(;;)
    {
        window_manager->main_loop();
    }
    return true;
}

void EshyWM::update_config()
{
    //Make a config class if there isn't one already
    if(!get_current_config())
    {
        current_config = std::make_shared<EshyWMConfig>();
    }

    std::ifstream config_file(get_current_config()->get_config_file_path());
    std::string line;

    while(std::getline(config_file, line))
    {
        parse_config_option(line, VT_STRING, &get_current_config()->background_path, "background:", "\"");

        parse_config_option(line, VT_UINT, &get_current_config()->window_frame_border_width, "window_frame_border_width:");
        parse_config_option(line, VT_UINT, &get_current_config()->window_frame_border_color, "window_frame_border_color:");
        parse_config_option(line, VT_UINT, &get_current_config()->window_background_color, "window_background_color:");

        parse_config_option(line, VT_UINT, &get_current_config()->titlebar_height, "titlebar_height:");
        parse_config_option(line, VT_UINT, &get_current_config()->titlebar_button_size, "titlebar_button_size:");
        parse_config_option(line, VT_UINT, &get_current_config()->titlebar_button_padding, "titlebar_button_padding:");
        parse_config_option(line, VT_ULONG, &get_current_config()->titlebar_button_normal_color, "titlebar_button_normal_color:");
        parse_config_option(line, VT_ULONG, &get_current_config()->titlebar_button_hovered_color, "titlebar_button_hovered_color:");
        parse_config_option(line, VT_ULONG, &get_current_config()->titlebar_button_pressed_color, "titlebar_button_pressed_color:");
        parse_config_option(line, VT_ULONG, &get_current_config()->titlebar_title_color, "titlebar_title_color:");

        parse_config_option(line, VT_UINT, &get_current_config()->context_menu_width, "context_menu_width:");
        parse_config_option(line, VT_ULONG, &get_current_config()->context_menu_color, "context_menu_color:");
        parse_config_option(line, VT_ULONG, &get_current_config()->context_menu_secondary_color, "context_menu_secondary_color:");

        parse_config_option(line, VT_UINT, &get_current_config()->taskbar_height, "taskbar_height:");
        parse_config_option(line, VT_ULONG, &get_current_config()->taskbar_color, "taskbar_color:");

        parse_config_option(line, VT_UINT, &get_current_config()->switcher_width, "switcher_width:");
        parse_config_option(line, VT_UINT, &get_current_config()->switcher_height, "switcher_height:");
        parse_config_option(line, VT_UINT, &get_current_config()->switcher_button_height, "switcher_button_height:");
        parse_config_option(line, VT_ULONG, &get_current_config()->switcher_color, "switcher_color:");

        parse_config_option(line, VT_ULONG, &get_current_config()->double_click_time, "double_click_time:");
    }

    config_file.close();
}

void EshyWM::run_startup_commands()
{
    std::ifstream config_file(get_current_config()->get_config_file_path());
    std::string current_command;
    bool b_in_startup_section = false;

    while(std::getline(config_file, current_command))
    {   
        if(current_command.find("[startup-commands]") != std::string::npos)
        {
            b_in_startup_section = true;
            continue;
        }
        else if(current_command.find("[~startup-commands]") != std::string::npos)
        {
            b_in_startup_section = false;
            break;
        }

        if(b_in_startup_section)
        {
            system((current_command + " &").c_str());
        }
    }

    config_file.close();
}


void EshyWM::on_screen_resolution_changed(uint new_width, uint new_height)
{
    TASKBAR->update_taskbar_size(new_width, new_height);

    //Reset the background to match the screen size
    update_background();
}


void EshyWM::update_background(std::string _background_path)
{   
    std::string background_path = _background_path.empty() ? get_current_config()->background_path : _background_path;

    //Reset background
    system(("feh --bg-scale " + background_path).c_str());
}