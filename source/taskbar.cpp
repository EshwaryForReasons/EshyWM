
#include "taskbar.h"
#include "eshywm.h"
#include "window_manager.h"
#include "config.h"
#include "window.h"
#include "button.h"

#include <algorithm>

EshyWMTaskbar::EshyWMTaskbar(rect _menu_geometry, Color _menu_color) : EshyWMMenuBase(_menu_geometry, _menu_color)
{
    XGrabButton(DISPLAY, Button1, AnyModifier, menu_window, false, ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, menu_window, None);
}

void EshyWMTaskbar::update_taskbar_size(uint width, uint height)
{
    set_position(0, height - EshyWMConfig::taskbar_height);
    set_size(width, EshyWMConfig::taskbar_height);
}

void EshyWMTaskbar::update_button_positions()
{
    for(int i = 0; i < taskbar_buttons.size(); i++)
    {
        taskbar_buttons[i].button->set_position((EshyWMConfig::taskbar_height * i) + 4, 2);
        taskbar_buttons[i].button->draw();
    }
}


void EshyWMTaskbar::add_button(std::shared_ptr<EshyWMWindow> associated_window, const Imlib_Image& icon)
{
    const std::shared_ptr<ImageButton> button = std::make_shared<ImageButton>(
        menu_window, 
        rect{0, 0, EshyWMConfig::taskbar_height - 4, EshyWMConfig::taskbar_height - 4}, 
        button_color_data{EshyWMConfig::taskbar_color, EshyWMConfig::taskbar_button_hovered_color, EshyWMConfig::taskbar_color}, 
        icon
    );
    taskbar_buttons.push_back({associated_window, button});
    WindowManager::add_button(button);
    update_button_positions();
}

void EshyWMTaskbar::remove_button(std::shared_ptr<EshyWMWindow> associated_window)
{
    for(int i = 0; i < taskbar_buttons.size(); i++)
    {
        if(taskbar_buttons[i].window == associated_window)
        {
            WindowManager::remove_button(taskbar_buttons[i].button);
            taskbar_buttons.erase(taskbar_buttons.begin() + i);
            update_button_positions();
            break;
        }
    }
}


void EshyWMTaskbar::check_taskbar_button_clicked(int cursor_x, int cursor_y)
{
    for(const window_button_pair& wbp : taskbar_buttons)
    {
        if(wbp.button->check_hovered(cursor_x, cursor_y))
        {
            Window returned_root;
            Window returned_parent;
            Window* top_level_windows;
            unsigned int num_top_level_windows;
            XQueryTree(DISPLAY, ROOT, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

            if(wbp.window->get_frame() != top_level_windows[num_top_level_windows - 1])
            {
                if(wbp.window->is_minimized())
                {
                    wbp.window->minimize_window();
                }

                XRaiseWindow(DISPLAY, wbp.window->get_frame());
                wbp.window->update_titlebar();
            }
            else wbp.window->minimize_window();

            XFree(top_level_windows);
            break;
        }
    }
}