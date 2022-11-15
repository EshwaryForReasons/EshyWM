#pragma once

#include <string>

class EshyWMConfig
{
public:

    /**Window frame*/
    uint window_frame_border_width;
    ulong window_frame_border_color;
    ulong window_background_color;

    /**Titlebar*/
    uint titlebar_height;
    uint titlebar_button_size;
    uint titlebar_button_padding;
    ulong titlebar_button_normal_color;
    ulong titlebar_button_hovered_color;
    ulong titlebar_button_pressed_color;
    ulong titlebar_title_color;

    /**Context menu*/
    uint context_menu_width;
    ulong context_menu_color;
    ulong context_menu_secondary_color;

    /**Taskbar*/
    uint taskbar_height;
    ulong taskbar_color;

    /**Switcher*/
    uint switcher_width;
    uint switcher_height;
    uint switcher_button_height;
    ulong switcher_color;

    /**Run menu*/
    uint run_menu_width;
    uint run_menu_height;
    uint run_menu_button_height;
    ulong run_menu_color;

    /**General double click time*/
    ulong double_click_time;

    /**Background image path*/
    std::string background_path;

    std::string get_config_file_path()
    {
        return std::string(getenv("HOME")) + "/.config" + "/eshywm.conf";
    }

    EshyWMConfig()
        //Window frame
        : window_frame_border_width(2)
        , window_frame_border_color(0x3d3a5c)
        , window_background_color(0x15141f)
        //Titlebar
        , titlebar_height(22)
        , titlebar_button_size(18)
        , titlebar_button_padding(4)
        , titlebar_button_normal_color(0x47436b)
        , titlebar_button_hovered_color(0x47436b)
        , titlebar_title_color(0xededed)
        //Context menu
        , context_menu_width(250)
        , context_menu_color(0xeeeeee)
        , context_menu_secondary_color(0x222222)
        //Taskbar
        , taskbar_height(30)
        , taskbar_color(0x283140)
        //Switcher
        , switcher_width(300)
        , switcher_height(500)
        , switcher_button_height(30)
        , switcher_color(0xaeb3bd)
        //Run menu
        , run_menu_width(600)
        , run_menu_height(400)
        , run_menu_button_height(30)
        , run_menu_color(0xaeb3bd)
        //General click time
        , double_click_time(500)
        //Background image path
        , background_path("")
    {}
};