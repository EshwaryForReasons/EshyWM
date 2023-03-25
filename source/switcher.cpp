
#include "switcher.h"
#include "eshywm.h"
#include "button.h"
#include "window.h"

#include <X11/Xutil.h>
#include <algorithm>

const int get_button_x_position(const int i)
{
    return ((EshyWMConfig::switcher_button_width + EshyWMConfig::switcher_button_padding) * i) + EshyWMConfig::switcher_button_padding;
}

const uint height_from_width(const uint width)
{
    return (9 * width) / 16;
}


EshyWMSwitcher::EshyWMSwitcher(rect _menu_geometry, Color _menu_color) : EshyWMMenuBase(_menu_geometry, _menu_color), selected_option(0)
{
    XGrabButton(DISPLAY, Button1, AnyModifier, menu_window, false, ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Tab), Mod4Mask, ROOT, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Alt_L), AnyModifier, ROOT, false, GrabModeAsync, GrabModeAsync);
}

void EshyWMSwitcher::show()
{
    XMapWindow(DISPLAY, menu_window);
    raise(true);
    b_menu_active = true;
}

void EshyWMSwitcher::raise(bool b_set_focus)
{
    EshyWMMenuBase::raise(b_set_focus);
    update_button_positions();
}

void EshyWMSwitcher::button_clicked(int x_root, int y_root)
{
    for(int i = 0; i < switcher_window_options.size(); i++)
    {
        if(switcher_window_options[i].button->check_hovered(x_root, y_root))
        {
            selected_option = i;
            confirm_choice();
            break;
        }
    }
}

void EshyWMSwitcher::update_button_positions()
{
    const uint width = (EshyWMConfig::switcher_button_width * switcher_window_options.size()) + (EshyWMConfig::switcher_button_padding * 3);
    const uint height = height_from_width(EshyWMConfig::switcher_button_width) + (EshyWMConfig::switcher_button_padding * 2);
    set_size(width, height);
    set_position(CENTER_W(width), CENTER_H(height));
    for(int i = 0; i < switcher_window_options.size(); i++)
    {
        switcher_window_options[i].button->set_position(get_button_x_position(i), EshyWMConfig::switcher_button_padding);
        switcher_window_options[i].button->draw();
    }
}


void EshyWMSwitcher::add_window_option(std::shared_ptr<EshyWMWindow> associated_window, const Imlib_Image& icon)
{
    const rect size = {0, 0, EshyWMConfig::switcher_button_width, height_from_width(EshyWMConfig::switcher_button_width)};
    const button_color_data color = {EshyWMConfig::switcher_button_color, EshyWMConfig::switcher_button_color, EshyWMConfig::switcher_button_color};

    std::shared_ptr<ImageButton> button = std::make_shared<ImageButton>(menu_window, size, color, icon);
    button->set_border_color(EshyWMConfig::switcher_button_border_color);
    
    switcher_window_options.push_back({associated_window, button});
    update_button_positions();
}

void EshyWMSwitcher::remove_window_option(std::shared_ptr<EshyWMWindow> associated_window)
{
    for(int i = 0; i < switcher_window_options.size(); i++)
    {
        if(switcher_window_options[i].window == associated_window)
        {
            switcher_window_options.erase(switcher_window_options.begin() + i);
            update_button_positions();
            break;
        }
    }
}

void EshyWMSwitcher::next_option()
{
    selected_option = std::max(selected_option + 1, 0);
    if(selected_option >= switcher_window_options.size())
    {
        selected_option = 0;
    }
    select_option(selected_option);
}

void EshyWMSwitcher::select_option(int i)
{
    //Deselect previous
    if(i == 0 && switcher_window_options[switcher_window_options.size() - 1].button)
    {
        XMoveWindow(DISPLAY, switcher_window_options[switcher_window_options.size() - 1].button->get_window(), get_button_x_position(switcher_window_options.size() - 1), EshyWMConfig::switcher_button_padding);
        XSetWindowBorderWidth(DISPLAY, switcher_window_options[switcher_window_options.size() - 1].button->get_window(), 0);
    }
    else if(i > 0 && switcher_window_options[i - 1].button)
    {
        XMoveWindow(DISPLAY, switcher_window_options[i - 1].button->get_window(), get_button_x_position(i - 1), EshyWMConfig::switcher_button_padding);
        XSetWindowBorderWidth(DISPLAY, switcher_window_options[i - 1].button->get_window(), 0);
    }

    if(switcher_window_options[i].button)
    {
        XMoveWindow(DISPLAY, switcher_window_options[i].button->get_window(), get_button_x_position(i), EshyWMConfig::switcher_button_padding);
        XSetWindowBorderWidth(DISPLAY, switcher_window_options[i].button->get_window(), 2);
    }
}

void EshyWMSwitcher::handle_option_chosen(const int i)
{
    Window returned_root;
    Window returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(DISPLAY, ROOT, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

    if(switcher_window_options[i].window->get_frame() != top_level_windows[num_top_level_windows - 1])
    {
        if(switcher_window_options[i].window->is_minimized())
        {
            WindowManager::_minimize_window(switcher_window_options[i].window);
        }

        XRaiseWindow(DISPLAY, switcher_window_options[i].window->get_frame());
        XSetInputFocus(DISPLAY, switcher_window_options[i].window->get_window(), RevertToPointerRoot, CurrentTime);
        switcher_window_options[i].window->update_titlebar();
    }

    XFree(top_level_windows);

    auto it = switcher_window_options.begin() + i;
    std::rotate(switcher_window_options.begin(), it, it + 1);
}


void EshyWMSwitcher::confirm_choice()
{
    handle_option_chosen(selected_option);
    remove();
    selected_option = 0;
}