#include <X11/Xutil.h>

#include "window_manager.h"
#include "window.h"
#include "config.h"
#include "eshywm.h"
#include "button.h"

#include <algorithm>
#include <cstring>

const int majority_monitor(rect window_geometry)
{
    int monitor = WindowManager::monitor_info.size() - 1;
    for(std::vector<s_monitor_info>::reverse_iterator it = WindowManager::monitor_info.rbegin(); it != WindowManager::monitor_info.rend(); it++)
    {
        if((window_geometry.x + (window_geometry.width / 2)) > (*it).x)
        {
            break;
        }

        monitor--;
    }
    return monitor;
}

namespace WindowManager
{
void _minimize_window(std::shared_ptr<EshyWMWindow> window, void* null)
{
    if(!window->is_minimized())
    {
        XUnmapWindow(DISPLAY, window->get_frame());
    }
    else
    {
        XMapWindow(DISPLAY, window->get_frame());
        window->update_titlebar();
    }

    window->set_minimized(!window->is_minimized());
}

void _maximize_window(std::shared_ptr<EshyWMWindow> window, void* b_from_move_or_resize)
{
    if(!window->is_maximized())
    {
        window->pre_minimize_and_maximize_saved_geometry = window->get_frame_geometry();
        const int i = majority_monitor(window->get_frame_geometry());
        window->move_window_absolute(DISPLAY_X(i), DISPLAY_Y(i), true);
        window->resize_window_absolute(DISPLAY_WIDTH(i) - (EshyWMConfig::window_frame_border_width * 2), DISPLAY_HEIGHT(i) - (EshyWMConfig::window_frame_border_width * 2), true);
    }
    else
    {
        if(!(bool*)(b_from_move_or_resize))
        {
            window->move_window_absolute(window->pre_minimize_and_maximize_saved_geometry.x, window->pre_minimize_and_maximize_saved_geometry.y, true);
        }

        window->resize_window_absolute(window->pre_minimize_and_maximize_saved_geometry.width, window->pre_minimize_and_maximize_saved_geometry.height, true);
    }

    window->set_maximized(!window->is_maximized());
}

void _close_window(std::shared_ptr<EshyWMWindow> window, void* null)
{
    window->remove_grab_events();
    window->unframe_window();

    Atom* supported_protocols;
    int num_supported_protocols;

    static const Atom wm_delete_window = XInternAtom(DISPLAY, "WM_DELETE_WINDOW", false);

    if (XGetWMProtocols(DISPLAY, window->window, &supported_protocols, &num_supported_protocols) && (std::find(supported_protocols, supported_protocols + num_supported_protocols, wm_delete_window) != supported_protocols + num_supported_protocols))
    {
        static const Atom wm_protocols = XInternAtom(DISPLAY, "WM_PROTOCOLS", false);

        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xclient.type = ClientMessage;
        message.xclient.message_type = wm_protocols;
        message.xclient.window = window->window;
        message.xclient.format = 32;
        message.xclient.data.l[0] = wm_delete_window;
        XSendEvent(DISPLAY, window->window, false, 0 , &message);
    }
    else
    {
        XKillClient(DISPLAY, window->window);
    }
}
}

void EshyWMWindow::frame_window(XWindowAttributes attr)
{
    frame_geometry = {attr.x, attr.y, (uint)attr.width, (uint)(attr.height + EshyWMConfig::titlebar_height)};
    frame = XCreateSimpleWindow(DISPLAY, ROOT, frame_geometry.x, frame_geometry.y, frame_geometry.width, frame_geometry.height, EshyWMConfig::window_frame_border_width, EshyWMConfig::window_frame_border_color, EshyWMConfig::window_background_color);
    XSelectInput(DISPLAY, frame, SubstructureNotifyMask | SubstructureRedirectMask);
    XReparentWindow(DISPLAY, window, frame, 0, EshyWMConfig::titlebar_height);
    XMapWindow(DISPLAY, frame);

    titlebar = XCreateSimpleWindow(DISPLAY, ROOT, attr.x, attr.y, attr.width, EshyWMConfig::titlebar_height, 0, EshyWMConfig::window_frame_border_color, EshyWMConfig::window_background_color);
    XSelectInput(DISPLAY, titlebar, VisibilityChangeMask);
    XReparentWindow(DISPLAY, titlebar, frame, 0, 0);
    XMapWindow(DISPLAY, titlebar);
    
    const rect initial_size = {0, 0, EshyWMConfig::titlebar_button_size, EshyWMConfig::titlebar_button_size};
    const button_color_data color = {EshyWMConfig::titlebar_button_normal_color, EshyWMConfig::titlebar_button_hovered_color, EshyWMConfig::titlebar_button_pressed_color};
    const button_color_data close_button_color = {EshyWMConfig::titlebar_button_normal_color, EshyWMConfig::close_button_color, EshyWMConfig::titlebar_button_pressed_color};
    minimize_button = std::make_shared<ImageButton>(titlebar, initial_size, color, "../images/minimize_window_icon.png");
    maximize_button = std::make_shared<ImageButton>(titlebar, initial_size, color, "../images/maximize_window_icon.png");
    close_button = std::make_shared<ImageButton>(titlebar, initial_size, close_button_color, "../images/close_window_icon.png");
    minimize_button->get_data() = {shared_from_this(), &WindowManager::_minimize_window};
    maximize_button->get_data() = {shared_from_this(), &WindowManager::_maximize_window};
    close_button->get_data() = {shared_from_this(), &WindowManager::_close_window};

    graphics_context_internal = XCreateGC(DISPLAY, frame, 0, 0);
    set_size_according_to(frame_geometry.width, frame_geometry.height);
    update_titlebar();
}

void EshyWMWindow::unframe_window()
{
    if(frame)
    {
        XUnmapWindow(DISPLAY, frame);
        XReparentWindow(DISPLAY, window, ROOT, 0, 0);
        XRemoveFromSaveSet(DISPLAY, window);
        XDestroyWindow(DISPLAY, frame);
    }

    if(titlebar)
    {
        XUnmapWindow(DISPLAY, titlebar);
        XDestroyWindow(DISPLAY, titlebar);
    }
}

void EshyWMWindow::setup_grab_events()
{
    //Basic movement and resizing
    XGrabButton(DISPLAY, Button1, AnyModifier, frame, false, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, Button1, Mod4Mask | ShiftMask, frame, false, ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, Button3, Mod4Mask | ShiftMask, frame, false, ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    XGrabButton(DISPLAY, Button1, AnyModifier, titlebar, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, titlebar, None);

    //Basic functions
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_c | XK_C), AnyModifier, frame, false, GrabModeAsync, GrabModeSync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_f | XK_F), AnyModifier, frame, false, GrabModeAsync, GrabModeSync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_a | XK_A), AnyModifier, frame, false, GrabModeAsync, GrabModeSync);
}

void EshyWMWindow::remove_grab_events()
{
    //Basic movement and resizing
    XUngrabButton(DISPLAY, Button1, AnyModifier, frame);
    XUngrabButton(DISPLAY, Button3, Mod1Mask, frame);
    XUngrabButton(DISPLAY, Button1, AnyModifier, titlebar);

    //Basic functions
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_c | XK_C), AnyModifier, frame);
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_f | XK_F), AnyModifier, frame);
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_a | XK_A), AnyModifier, frame);
}


void EshyWMWindow::move_window_absolute(int new_position_x, int new_position_y, bool b_from_maximize)
{
    if(b_is_maximized && !b_from_maximize)
    {
        WindowManager::_maximize_window(shared_from_this(), (void*)true);
    }

    frame_geometry.x = new_position_x;
    frame_geometry.y = new_position_y;
    XMoveWindow(DISPLAY, frame, new_position_x, new_position_y);
    update_titlebar();
}

void EshyWMWindow::resize_window_absolute(uint new_size_x, uint new_size_y, bool b_from_maximize)
{
    if(b_is_maximized && !b_from_maximize)
    {
        WindowManager::_maximize_window(shared_from_this(), (void*)true);
    }

    set_size_according_to(new_size_x, new_size_y);
    XResizeWindow(DISPLAY, frame, frame_geometry.width, frame_geometry.height);
    XResizeWindow(DISPLAY, window, window_geometry.width, window_geometry.height);
    XResizeWindow(DISPLAY, titlebar, titlebar_geometry.width, titlebar_geometry.height);
    update_titlebar();
}

void EshyWMWindow::set_size_according_to(uint new_width, uint new_height)
{
    frame_geometry.width = window_geometry.width = titlebar_geometry.width = new_width;
    frame_geometry.height = new_height;

    window_geometry.height = new_height - EshyWMConfig::titlebar_height;
}

void EshyWMWindow::recalculate_all_window_size_and_location()
{
    Window return_window;
    int x;
    int y;
    unsigned int unsigned_null;
    unsigned int width;
    unsigned int height;

    XGetGeometry(DISPLAY, frame, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    frame_geometry = {x, y, width, height};
    
    XGetGeometry(DISPLAY, window, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    window_geometry = {x, y, width, height};

    XGetGeometry(DISPLAY, titlebar, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    titlebar_geometry = {x, y, width, height};
}


void EshyWMWindow::update_titlebar()
{
    XClearWindow(DISPLAY, titlebar);

    XTextProperty name;
    XGetWMName(DISPLAY, window, &name);
    
    //Draw title
    XSetForeground(DISPLAY, graphics_context_internal, EshyWMConfig::titlebar_title_color);
    XDrawString(DISPLAY, titlebar, graphics_context_internal, 10, 16, reinterpret_cast<char*>(name.value), name.nitems);

    const int button_y_offset = (EshyWMConfig::titlebar_height - EshyWMConfig::titlebar_button_size) / 2;
    const auto update_button_position = [this, button_y_offset](std::shared_ptr<Button> button, int i)
    {
        button->set_position((get_window_geometry().width - EshyWMConfig::titlebar_button_size * i) - (EshyWMConfig::titlebar_button_padding * (i - 1)) - button_y_offset, button_y_offset);
        button->draw();
    };

    update_button_position(minimize_button, 3);
    update_button_position(maximize_button, 2);
    update_button_position(close_button, 1);
}