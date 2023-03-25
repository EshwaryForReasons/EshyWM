#pragma once

#include <Imlib2.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "util.h"

struct s_monitor_info
{
    bool b_primary;
    int x;
    int y;
    uint width;
    uint height;
};

class Button;
class EshyWMWindow;

namespace WindowManager
{
    void initialize();
    void main_loop();
    void handle_preexisting_windows();

    extern Display* display;
    extern std::vector<std::shared_ptr<class EshyWMWindow>> window_list;
    extern std::vector<s_monitor_info> monitor_info;
};