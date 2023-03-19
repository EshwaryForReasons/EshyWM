
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
    int monitor = WindowManager::Internal::monitor_info.size() - 1;
    for(std::vector<s_monitor_info>::reverse_iterator it = WindowManager::Internal::monitor_info.rbegin(); it != WindowManager::Internal::monitor_info.rend(); it++)
    {
        if((window_geometry.x + (window_geometry.width / 2)) > (*it).x)
        {
            break;
        }

        monitor--;
    }
    return monitor;
}


void EshyWMWindow::frame_window(XWindowAttributes attr)
{
    frame_geometry = {attr.x, attr.y, attr.width, attr.height + (IS_TILING_MODE() ? 0 : EshyWMConfig::titlebar_height)};
    frame = XCreateSimpleWindow(DISPLAY, ROOT, frame_geometry.x, frame_geometry.y, frame_geometry.width, frame_geometry.height, EshyWMConfig::window_frame_border_width, EshyWMConfig::window_frame_border_color, EshyWMConfig::window_background_color);
    XReparentWindow(DISPLAY, window, frame, 0, IS_TILING_MODE() ? 0 : EshyWMConfig::titlebar_height);
    XMapWindow(DISPLAY, frame);

    if(!IS_TILING_MODE())
    {
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
        WindowManager::add_button(minimize_button);
        WindowManager::add_button(maximize_button);
        WindowManager::add_button(close_button);
    }

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
        WindowManager::remove_button(minimize_button);
        WindowManager::remove_button(maximize_button);
        WindowManager::remove_button(close_button);
    }
}

void EshyWMWindow::setup_grab_events()
{
    //Basic movement and resizing
    XGrabButton(DISPLAY, Button1, AnyModifier, frame, false, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, Button1, Mod4Mask | ShiftMask, frame, false, ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, Button3, Mod4Mask | ShiftMask, frame, false, ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    if(!IS_TILING_MODE())
    {
        XGrabButton(DISPLAY, Button1, AnyModifier, titlebar, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, titlebar, None);
    }

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

void EshyWMWindow::minimize_window()
{
    if(!b_is_minimized)
    {
        XUnmapWindow(DISPLAY, frame);
    }
    else
    {
        XMapWindow(DISPLAY, frame);
        update_titlebar();
    }

    b_is_minimized = !b_is_minimized;
}

void EshyWMWindow::maximize_window(bool b_from_move_or_resize)
{
    if(!b_is_maximized)
    {
        pre_minimize_and_maximize_saved_geometry = get_frame_geometry();
        const int i = majority_monitor(get_frame_geometry());
        move_window_absolute(DISPLAY_X(i), DISPLAY_Y(i));
        resize_window_absolute(DISPLAY_WIDTH(i) - (EshyWMConfig::window_frame_border_width * 2), DISPLAY_HEIGHT(i) - (EshyWMConfig::window_frame_border_width * 2));
    }
    else
    {
        if(!b_from_move_or_resize)
        {
            move_window_absolute(pre_minimize_and_maximize_saved_geometry.x, pre_minimize_and_maximize_saved_geometry.y);
        }

        resize_window_absolute(pre_minimize_and_maximize_saved_geometry.width, pre_minimize_and_maximize_saved_geometry.height);
    }

    b_is_maximized = !b_is_maximized;
}

void EshyWMWindow::close_window()
{
    remove_grab_events();
    unframe_window();

    Atom* supported_protocols;
    int num_supported_protocols;

    static const Atom wm_delete_window = XInternAtom(DISPLAY, "WM_DELETE_WINDOW", false);

    if (XGetWMProtocols(DISPLAY, window, &supported_protocols, &num_supported_protocols) && (std::find(supported_protocols, supported_protocols + num_supported_protocols, wm_delete_window) != supported_protocols + num_supported_protocols))
    {
        static const Atom wm_protocols = XInternAtom(DISPLAY, "WM_PROTOCOLS", false);

        //LOG(INFO) << "Gracefully deleting window " << window;
        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xclient.type = ClientMessage;
        message.xclient.message_type = wm_protocols;
        message.xclient.window = window;
        message.xclient.format = 32;
        message.xclient.data.l[0] = wm_delete_window;
        XSendEvent(DISPLAY, window, false, 0 , &message);
    }
    else
    {
        //LOG(INFO) << "Killing window " << window;
        XKillClient(DISPLAY, window);
    }

    //WindowManager::close_window(shared_from_this());
}


void EshyWMWindow::move_window(int delta_x, int delta_y)
{
    if(b_is_maximized)
    {
        maximize_window(true);
    }

    if(!b_is_currently_moving_or_resizing)
    {
        b_is_currently_moving_or_resizing = true;
        temp_move_and_resize_geometry = get_frame_geometry();
    }

    move_window_absolute(temp_move_and_resize_geometry.x + delta_x, temp_move_and_resize_geometry.y + delta_y);
}

void EshyWMWindow::move_window_absolute(int new_position_x, int new_position_y)
{
    frame_geometry.x = new_position_x;
    frame_geometry.y = new_position_y;
    XMoveWindow(DISPLAY, frame, new_position_x, new_position_y);
    update_titlebar();
}

void EshyWMWindow::resize_window(int delta_x, int delta_y)
{
    if(b_is_maximized)
    {
        maximize_window(true);
    }

    if(!b_is_currently_moving_or_resizing)
    {
        b_is_currently_moving_or_resizing = true;
        temp_move_and_resize_geometry = get_frame_geometry();
    }

    const Vector2D<uint> size_delta(std::max(delta_x, -(int)temp_move_and_resize_geometry.width), std::max(delta_y, -(int)temp_move_and_resize_geometry.height));
    const Vector2D<uint> final_frame_size = Vector2D<uint>(temp_move_and_resize_geometry.width, temp_move_and_resize_geometry.height) + size_delta;
    resize_window_absolute(final_frame_size);
}

void EshyWMWindow::resize_window_absolute(uint new_size_x, uint new_size_y)
{
    set_size_according_to(new_size_x, new_size_y);
    XResizeWindow(DISPLAY, frame, frame_geometry.width, frame_geometry.height);
    XResizeWindow(DISPLAY, window, window_geometry.width, window_geometry.height);

    if(!IS_TILING_MODE())
    {
        XResizeWindow(DISPLAY, titlebar, titlebar_geometry.width, titlebar_geometry.height);
    }

    update_titlebar();
}

void EshyWMWindow::motion_modify_ended()
{
    b_is_currently_moving_or_resizing = false;
}

void EshyWMWindow::set_size_according_to(uint new_width, uint new_height)
{
    frame_geometry.width = window_geometry.width = titlebar_geometry.width = new_width;
    frame_geometry.height = new_height;

    window_geometry.height = new_height - (IS_TILING_MODE() ? 0 : EshyWMConfig::titlebar_height);
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

    if(!IS_TILING_MODE())
    {
        XGetGeometry(DISPLAY, titlebar, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
        titlebar_geometry = {x, y, width, height};
    }
}


void EshyWMWindow::update_titlebar()
{
    if(IS_TILING_MODE())
    {
        return;
    }

    XClearWindow(DISPLAY, titlebar);

    XTextProperty name;
    XGetWMName(DISPLAY, window, &name);
    
    //Draw title
    XSetForeground(DISPLAY, graphics_context_internal, EshyWMConfig::titlebar_title_color);
    XDrawString(DISPLAY, titlebar, graphics_context_internal, 10, 16, reinterpret_cast<char*>(name.value), name.nitems);

    const int button_y_offset = (EshyWMConfig::titlebar_height - EshyWMConfig::titlebar_button_size) / 2;
    const auto calc_x = [this, button_y_offset](int i)
    {
        return (get_window_geometry().width - EshyWMConfig::titlebar_button_size * i) - (EshyWMConfig::titlebar_button_padding * (i - 1)) - button_y_offset;
    };

    minimize_button->set_position(calc_x(3), button_y_offset);
    maximize_button->set_position(calc_x(2), button_y_offset);
    close_button->set_position(calc_x(1), button_y_offset);
    minimize_button->draw();
    maximize_button->draw();
    close_button->draw();
}

int EshyWMWindow::is_cursor_on_titlebar_buttons(Window window, int cursor_x, int cursor_y)
{
    if(IS_TILING_MODE() || window != titlebar)
    {
        return 0;
    }

    if(minimize_button->check_hovered(cursor_x, cursor_y))
    return 1;
    else if(maximize_button->check_hovered(cursor_x, cursor_y))
    return 2;
    else if(close_button->check_hovered(cursor_x, cursor_y))
    return 3;

    return 0;
}