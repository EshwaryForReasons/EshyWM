
#include "taskbar.h"
#include "eshywm.h"
#include "window_manager.h"
#include "config.h"
#include "window.h"
#include "button.h"

#include <algorithm>

void on_taskbar_button_clicked(std::shared_ptr<EshyWMWindow> window, void* null)
{
    Window returned_root;
    Window returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(DISPLAY, ROOT, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

    if(window->get_frame() != top_level_windows[num_top_level_windows - 1])
    {
        if(window->is_minimized())
        {
            WindowManager::_minimize_window(window);
        }

        XRaiseWindow(DISPLAY, window->get_frame());
        window->update_titlebar();
    }
    else WindowManager::_minimize_window(window);

    XFree(top_level_windows);
}


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
        taskbar_buttons[i]->set_position((EshyWMConfig::taskbar_height * i) + 4, 2);
        taskbar_buttons[i]->draw();
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
    button->get_data() = {associated_window, &on_taskbar_button_clicked};
    taskbar_buttons.push_back(button);
    update_button_positions();
}

void EshyWMTaskbar::remove_button(std::shared_ptr<EshyWMWindow> associated_window)
{
    for(int i = 0; i < taskbar_buttons.size(); i++)
    {
        if(taskbar_buttons[i]->get_data().associated_window == associated_window)
        {
            taskbar_buttons.erase(taskbar_buttons.begin() + i);
            update_button_positions();
            break;
        }
    }
}