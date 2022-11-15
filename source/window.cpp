
#include <X11/Xutil.h>

#include "window_manager.h"
#include "window.h"
#include "config.h"
#include "eshywm.h"
#include "button.h"

#include <glog/logging.h>

#include <algorithm>

void EshyWMWindow::frame_window(bool b_was_created_before_window_manager)
{
    //Retrieve attributes of window to frame
    XWindowAttributes attr = {0};
    XGetWindowAttributes(DISPLAY, window, &attr);

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (attr.override_redirect || attr.map_state != IsViewable))
    {
        return;
    }

    //Create frame and titlebar
    frame = XCreateSimpleWindow(
        DISPLAY,
        ROOT,
        attr.x,
        attr.y,
        attr.width,
        attr.height + (IS_TILING_MODE() ? 0 : CONFIG->titlebar_height),
        CONFIG->window_frame_border_width,
        CONFIG->window_frame_border_color,
        CONFIG->window_background_color
    );
    XReparentWindow(DISPLAY, window, frame, 0, IS_TILING_MODE() ? 0 : CONFIG->titlebar_height);
    XMapWindow(DISPLAY, frame);

    if(!IS_TILING_MODE())
    {
        titlebar = XCreateSimpleWindow(DISPLAY, ROOT, attr.x, attr.y, attr.width, CONFIG->titlebar_height, 0, CONFIG->window_frame_border_color, CONFIG->window_background_color);
        XReparentWindow(DISPLAY, titlebar, frame, 0, 0);
        XMapWindow(DISPLAY, titlebar);

        const rect initial_size = {0, 0, CONFIG->titlebar_button_size, CONFIG->titlebar_button_size};
        minimize_button = std::make_shared<ImageButton>(titlebar, initial_size, "../images/minimize_window_normal_image.png");
        maximize_button = std::make_shared<ImageButton>(titlebar, initial_size, "../images/minimize_window_normal_image.png");
        close_button = std::make_shared<ImageButton>(titlebar, initial_size, "../images/close_window_hovered_image.png");
    }

    graphics_context_internal = XCreateGC(DISPLAY, frame, 0, 0);
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

    if(!IS_TILING_MODE() && titlebar)
    {
        XUnmapWindow(DISPLAY, titlebar);
        XDestroyWindow(DISPLAY, titlebar);
    }
}

void EshyWMWindow::setup_grab_events(bool b_was_created_before_window_manager)
{
    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    XGetWindowAttributes(DISPLAY, window, &x_window_attributes);

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
    {
        return;
    }

    //Basic movement and resizing
    XGrabButton(DISPLAY, Button1, AnyModifier, frame, false, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, Button1, Mod4Mask | ShiftMask, frame, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, Button3, Mod4Mask | ShiftMask, frame, false, ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    if(!IS_TILING_MODE())
    {
        XGrabButton(DISPLAY, Button1, AnyModifier, titlebar, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, titlebar, None);
    }

    //Kill windows with alt + shift + c
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_C), Mod4Mask, frame, false, GrabModeAsync, GrabModeSync);

    //Resize windows with alt + arrow keys. Move windows with alt + shift + arrow keys
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Left), Mod4Mask, frame, false, GrabModeAsync, GrabModeSync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Right), Mod4Mask, frame, false, GrabModeAsync, GrabModeSync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Up), Mod4Mask, frame, false, GrabModeAsync, GrabModeSync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Down), Mod4Mask, frame, false, GrabModeAsync, GrabModeSync);

    //Basic functions
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_f), Mod4Mask, frame, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_a), Mod4Mask, frame, false, GrabModeAsync, GrabModeSync);
}

void EshyWMWindow::remove_grab_events()
{
    //Basic movement and resizing
    XUngrabButton(DISPLAY, Button1, 0, frame);
    XUngrabButton(DISPLAY, Button1, Mod1Mask, frame);
    XUngrabButton(DISPLAY, Button3, Mod1Mask, frame);
    XUngrabButton(DISPLAY, Button1, 0, titlebar);

    //Kill windows with alt + shift + c
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_C), Mod1Mask, frame);

    //Resize windows with alt + arrow keys. Move windows with alt + shift + arrow keys
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Left), Mod1Mask | ControlMask, frame);
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Right), Mod1Mask | ControlMask, frame);
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Up), Mod1Mask | ControlMask, frame);
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Down), Mod1Mask | ControlMask, frame);

    //Basic functions
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_f | XK_F), Mod1Mask, frame);
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_a | XK_A), Mod1Mask, frame);
}

void EshyWMWindow::minimize_window()
{
    if(!b_is_minimized)
    {
        XUnmapWindow(WINDOW_MANAGER->get_display(), frame);
    }
    else
    {
        XMapWindow(WINDOW_MANAGER->get_display(), frame);
    }

    b_is_minimized = !b_is_minimized;
}

void EshyWMWindow::maximize_window(bool b_from_move_or_resize)
{
    if(!b_is_maximized)
    {
        recalculate_all_window_size_and_location();
        pre_minimize_and_maximize_saved_geometry = get_frame_geometry();
        int w = 0;
        //Check which monitor we are on
        if((pre_minimize_and_maximize_saved_geometry.x + pre_minimize_and_maximize_saved_geometry.width) > 1920)
        {
            w = 1920;
        }
        move_window_absolute(w, 0);
        resize_window_absolute(DISPLAY_WIDTH - (CONFIG->window_frame_border_width * 2), DISPLAY_HEIGHT - (CONFIG->window_frame_border_width * 2));
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
    unframe_window();
    remove_grab_events();

    Atom* supported_protocols;
    int num_supported_protocols;

    static const Atom wm_delete_window = XInternAtom(DISPLAY, "WM_DELETE_WINDOW", false);

    if (XGetWMProtocols(DISPLAY, window, &supported_protocols, &num_supported_protocols) && (std::find(supported_protocols, supported_protocols + num_supported_protocols, wm_delete_window) != supported_protocols + num_supported_protocols))
    {
        static const Atom wm_protocols = XInternAtom(DISPLAY, "WM_PROTOCOLS", false);

        LOG(INFO) << "Gracefully deleting window " << window;
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
        LOG(INFO) << "Killing window " << window;
        XKillClient(DISPLAY, window);
    }

    WINDOW_MANAGER->close_window(shared_from_this());
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
    XMoveWindow(DISPLAY, frame, new_position_x, new_position_y);
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
    frame_geometry.width = new_size_x;
    frame_geometry.height = new_size_y;
    XResizeWindow(DISPLAY, frame, frame_geometry.width, frame_geometry.height);

    window_geometry.width = new_size_x;
    window_geometry.height = new_size_y - (IS_TILING_MODE() ? 0 : CONFIG->titlebar_height);
    XResizeWindow(DISPLAY, window, window_geometry.width, window_geometry.height);

    if(!IS_TILING_MODE())
    {
        titlebar_geometry.width = new_size_x;
        XResizeWindow(DISPLAY, titlebar, titlebar_geometry.width, titlebar_geometry.height);
    }
}

void EshyWMWindow::motion_modify_ended()
{
    b_is_currently_moving_or_resizing = false;
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


void EshyWMWindow::draw()
{
    if(IS_TILING_MODE())
    {
        return;
    }

    XClearWindow(DISPLAY, titlebar);

    const int button_y_offset = (CONFIG->titlebar_height - CONFIG->titlebar_button_size) / 2;
    const auto calc_x = [this, button_y_offset](int i)
    {
        return (get_window_geometry().width - CONFIG->titlebar_button_size * i) - (CONFIG->titlebar_button_padding * (i - 1)) - button_y_offset;
    };

    //Draw buttons
    minimize_button->set_position(calc_x(3), button_y_offset);
    minimize_button->draw();
    maximize_button->set_position(calc_x(2), button_y_offset);
    maximize_button->draw();
    close_button->set_position(calc_x(1), button_y_offset);
    close_button->draw();
    
    XTextProperty name;
    XGetWMName(DISPLAY, window, &name);
    
    //Draw title
    XSetForeground(DISPLAY, graphics_context_internal, CONFIG->titlebar_title_color);
    XDrawString(DISPLAY, titlebar, graphics_context_internal, 10, 13, reinterpret_cast<char*>(name.value), name.nitems);
}

int EshyWMWindow::is_cursor_on_titlebar_buttons(Window window, int cursor_x, int cursor_y)
{
    if(IS_TILING_MODE() || window != titlebar)
    {
        return 0;
    }

    if(minimize_button->is_hovered(cursor_x, cursor_y))
    return 1;
    else if(maximize_button->is_hovered(cursor_x, cursor_y))
    return 2;
    else if(close_button->is_hovered(cursor_x, cursor_y))
    return 3;

    return 0;
}