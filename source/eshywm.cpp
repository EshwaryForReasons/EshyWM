#include "eshywm.h"
#include "window_manager.h"
#include "config.h"
#include "context_menu.h"
#include "taskbar.h"
#include "switcher.h"
#include "run_menu.h"
#include "util.h"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>

std::shared_ptr<EshyWMTaskbar> EshyWM::Internal::taskbar;
std::shared_ptr<EshyWMSwitcher> EshyWM::Internal::switcher;
std::shared_ptr<EshyWMContextMenu> EshyWM::Internal::context_menu;
std::shared_ptr<EshyWMRunMenu> EshyWM::Internal::run_menu;

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

static void update_background(const std::string& _background_path = std::string())
{
    const std::string s = _background_path.empty() ? EshyWMConfig::background_path : _background_path;
    system(("feh --bg-scale " + s).c_str());
}

static void run_startup_commands()
{
    std::ifstream config_file(EshyWMConfig::get_config_file_path());
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

static void update_config()
{
    std::ifstream config_file(EshyWMConfig::get_config_file_path());
    std::string line;

    while(std::getline(config_file, line))
    {
        parse_config_option(line, VT_STRING, &EshyWMConfig::background_path, "background:", "\"");

        parse_config_option(line, VT_UINT, &EshyWMConfig::window_frame_border_width, "window_frame_border_width:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::window_frame_border_color, "window_frame_border_color:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::window_background_color, "window_background_color:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::close_button_color, "close_button_color:");

        parse_config_option(line, VT_UINT, &EshyWMConfig::titlebar_height, "titlebar_height:");
        parse_config_option(line, VT_UINT, &EshyWMConfig::titlebar_button_size, "titlebar_button_size:");
        parse_config_option(line, VT_UINT, &EshyWMConfig::titlebar_button_padding, "titlebar_button_padding:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::titlebar_button_normal_color, "titlebar_button_normal_color:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::titlebar_button_hovered_color, "titlebar_button_hovered_color:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::titlebar_button_pressed_color, "titlebar_button_pressed_color:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::titlebar_title_color, "titlebar_title_color:");

        parse_config_option(line, VT_UINT, &EshyWMConfig::context_menu_width, "context_menu_width:");
        parse_config_option(line, VT_UINT, &EshyWMConfig::context_menu_button_height, "context_menu_button_height:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::context_menu_color, "context_menu_color:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::context_menu_secondary_color, "context_menu_secondary_color:");

        parse_config_option(line, VT_UINT, &EshyWMConfig::taskbar_height, "taskbar_height:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::taskbar_color, "taskbar_color:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::taskbar_button_hovered_color, "taskbar_button_hovered_color:");

        parse_config_option(line, VT_ULONG, &EshyWMConfig::switcher_button_width, "switcher_button_width:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::switcher_button_padding, "switcher_button_padding:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::switcher_button_color, "switcher_button_color:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::switcher_button_border_color, "switcher_button_border_color:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::switcher_color, "switcher_color:");

        parse_config_option(line, VT_UINT, &EshyWMConfig::run_menu_width, "run_menu_width:");
        parse_config_option(line, VT_UINT, &EshyWMConfig::run_menu_height, "run_menu_height:");
        parse_config_option(line, VT_UINT, &EshyWMConfig::run_menu_button_height, "run_menu_button_height:");
        parse_config_option(line, VT_ULONG, &EshyWMConfig::run_menu_color, "run_menu_color:");

        parse_config_option(line, VT_ULONG, &EshyWMConfig::double_click_time, "double_click_time:");
    }

    config_file.close();
}


bool EshyWM::initialize()
{
    update_config();
    run_startup_commands();
    update_background();

    WindowManager::initialize();

    imlib_context_set_display(DISPLAY);
    imlib_context_set_visual(DefaultVisual(DISPLAY, DefaultScreen(DISPLAY)));
    imlib_context_set_colormap(DefaultColormap(DISPLAY, DefaultScreen(DISPLAY)));

    Internal::context_menu = std::make_shared<EshyWMContextMenu>(rect{0, 0, EshyWMConfig::context_menu_width, 150}, EshyWMConfig::context_menu_color);
    Internal::switcher = std::make_shared<EshyWMSwitcher>(rect{CENTER_W(EshyWMConfig::switcher_button_width), CENTER_H((16/9) * EshyWMConfig::switcher_button_width), 50, 50}, EshyWMConfig::switcher_color);
    Internal::run_menu = std::make_shared<EshyWMRunMenu>(rect{CENTER_W(EshyWMConfig::run_menu_width), CENTER_H(EshyWMConfig::run_menu_height), EshyWMConfig::run_menu_width, EshyWMConfig::run_menu_height}, EshyWMConfig::run_menu_color);
    Internal::taskbar = std::make_shared<EshyWMTaskbar>(rect{0, DISPLAY_HEIGHT(0), DISPLAY_WIDTH(0), EshyWMConfig::taskbar_height}, EshyWMConfig::taskbar_color);
    Internal::taskbar->show();

    WindowManager::handle_preexisting_windows();
    for(;;)
    {
        WindowManager::main_loop();
    }
    return true;
}

void EshyWM::on_screen_resolution_changed(uint new_width, uint new_height)
{
    TASKBAR->update_taskbar_size(new_width, new_height);

    //Reset the background to match the screen size
    update_background();
}
