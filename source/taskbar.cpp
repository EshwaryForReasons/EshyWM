
#include "taskbar.h"
#include "eshywm.h"
#include "window_manager.h"
#include "config.h"
#include "window.h"

EshyWMTaskbar::EshyWMTaskbar(rect _menu_geometry, Color _menu_color) : EshyWMMenuBase(_menu_geometry, _menu_color)
{
    XGrabButton(DISPLAY, Button1, AnyModifier, menu_window, false, ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, menu_window, None);
}

void EshyWMTaskbar::update_taskbar_size(uint width, uint height)
{
    set_position(0, height - CONFIG->taskbar_height);
    set_size(width, CONFIG->taskbar_height);
}

void EshyWMTaskbar::draw()
{
    int i = 0;
    for(const window_button_pair& wbp : taskbar_buttons)
    {
        wbp.button->set_position((CONFIG->taskbar_height * i) + 4, 2);
        wbp.button->draw();
        i++;
    }
}


void EshyWMTaskbar::add_button(std::shared_ptr<EshyWMWindow> associated_window)
{
    const Atom NET_WM_ICON = XInternAtom(DISPLAY, "_NET_WM_ICON", false);
    const Atom CARDINAL = XInternAtom(DISPLAY, "CARDINAL", false);

    Atom type_return;
    int format_return;
    unsigned long nitems_return;
    unsigned long bytes_after_return;
    unsigned char* data_return;

    XGetWindowProperty(DISPLAY, associated_window->get_window(), NET_WM_ICON, 0, 1, false, CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    const int width = *(int*)data_return;
    XFree(data_return);

    XGetWindowProperty(DISPLAY, associated_window->get_window(), NET_WM_ICON, 1, 1, false, CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    const int height = *(int*)data_return;
    XFree(data_return);

    XGetWindowProperty(DISPLAY, associated_window->get_window(), NET_WM_ICON, 2, width * height, false, CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    
    uint32_t* img_data = new uint32_t[width * height];
    const ulong* ul = (ulong*)data_return;

    for(int i = 0; i < nitems_return; i++)
    {
        img_data[i] = (uint32_t)ul[i];
    }

    XFree(data_return);

    const rect size = {0, 0, EshyWM::get_current_config()->taskbar_height - 4, EshyWM::get_current_config()->taskbar_height - 4};
    std::shared_ptr<ImageButton> button = std::make_shared<ImageButton>(menu_window, size, imlib_create_image_using_data(width, height, img_data));
    taskbar_buttons.push_back({associated_window, button});
}

void EshyWMTaskbar::remove_button(std::shared_ptr<EshyWMWindow> associated_window)
{
    for(int i = 0; i < taskbar_buttons.size(); i++)
    {
        if(taskbar_buttons[i].window == associated_window)
        {
            taskbar_buttons.erase(taskbar_buttons.begin() + i);
            break;
        }
    }
}


void EshyWMTaskbar::check_taskbar_button_clicked(int cursor_x, int cursor_y)
{
    for(const window_button_pair& wbp : taskbar_buttons)
    {
        if(wbp.button->is_hovered(cursor_x, cursor_y))
        {
            wbp.window->minimize_window();
            XRaiseWindow(EshyWM::get_window_manager()->get_display(), wbp.window->get_frame());
            break;
        }
    }
}