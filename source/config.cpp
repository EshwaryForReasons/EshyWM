#include "config.h"

//Window frame
uint EshyWMConfig::window_frame_border_width = 1;
ulong EshyWMConfig::window_frame_border_color = 0x3d3a5c;
ulong EshyWMConfig::window_background_color = 0x15141f;
ulong EshyWMConfig::close_button_color = 0xdb5353;
//Titlebar
uint EshyWMConfig::titlebar_height = 26;
uint EshyWMConfig::titlebar_button_size = 26;
uint EshyWMConfig::titlebar_button_padding = 4;
ulong EshyWMConfig::titlebar_button_normal_color = 0x15141f;
ulong EshyWMConfig::titlebar_button_hovered_color = 0x423d66;
ulong EshyWMConfig::titlebar_button_pressed_color = 0x0d0c14;
ulong EshyWMConfig::titlebar_title_color = 0xededed;
//Context menu
uint EshyWMConfig::context_menu_width = 250;
uint EshyWMConfig::context_menu_button_height = 20;
ulong EshyWMConfig::context_menu_color = 0xeeeeee;
ulong EshyWMConfig::context_menu_secondary_color = 0x222222;
//Taskbar
uint EshyWMConfig::taskbar_height = 30;
ulong EshyWMConfig::taskbar_color = 0x283140;
ulong EshyWMConfig::taskbar_button_hovered_color = 0x42516b;
//Switcher
uint EshyWMConfig::switcher_button_width = 250;
uint EshyWMConfig::switcher_button_padding = 20;
ulong EshyWMConfig::switcher_button_color = 0x888888;
ulong EshyWMConfig::switcher_button_border_color = 0x444444;
ulong EshyWMConfig::switcher_color = 0xaeb3bd;
//Run menu
uint EshyWMConfig::run_menu_width = 600;
uint EshyWMConfig::run_menu_height = 400;
uint EshyWMConfig::run_menu_button_height = 30;
ulong EshyWMConfig::run_menu_color = 0xaeb3bd;
//General click time
ulong EshyWMConfig::double_click_time = 500;
//Background image path
std::string EshyWMConfig::background_path = "";

std::string EshyWMConfig::get_config_file_path()
{
    return std::string(getenv("HOME")) + "/.config" + "/eshywm.conf";
}