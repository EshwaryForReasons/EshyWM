
#pragma once

#include <Imlib2.h>
#include <memory>
#include <mutex>
#include <string>
#include <map>

#include "util.h"
#include "container.h"

struct double_click_data
{
    Window window;
    Time first_click_time;
    Time last_double_click_time;
};

struct s_monitor_info
{
    bool b_primary;
    int x;
    int y;
    int width;
    int height;
};

class Button;
class EshyWMWindow;

namespace WindowManager
{
    void initialize();
    void main_loop();
    void handle_preexisting_windows();

    void add_button(std::shared_ptr<Button> button);
    void remove_button(std::shared_ptr<Button> button);

namespace Internal
{
    extern Display* display;

    extern Vector2D<int> click_cursor_position;
    extern std::shared_ptr<class container> root_container;

    extern bool b_tiling_mode;
    extern std::vector<s_monitor_info> monitor_info;

    extern organized_container_map_t frame_list;
    extern organized_container_map_t titlebar_list;
    extern std::vector<std::shared_ptr<Button>> button_list;

    extern double_click_data titlebar_double_click;
};
};