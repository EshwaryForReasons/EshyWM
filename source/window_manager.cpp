#include "window_manager.h"
#include "eshywm.h"
#include "window.h"
#include "config.h"
#include "context_menu.h"
#include "taskbar.h"
#include "switcher.h"
#include "run_menu.h"
#include "button.h"

#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <cstring>
#include <iostream>
#include <algorithm>

#define CHECK_KEYSYM_PRESSED(event, key_sym)                    if(event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
#define ELSE_CHECK_KEYSYM_PRESSED(event, key_sym)               else if(event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
#define CHECK_KEYSYM_AND_MOD_PRESSED(event, mod, key_sym)       if(event.state & mod && event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
#define ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, mod, key_sym)  else if(event.state & mod && event.keycode == XKeysymToKeycode(DISPLAY, key_sym))

Display* WindowManager::display;
std::vector<std::shared_ptr<EshyWMWindow>> WindowManager::window_list;
std::vector<s_monitor_info> WindowManager::monitor_info;

static Vector2D<int> click_cursor_position;
static struct double_click_data
{
    Window window;
    Time first_click_time;
    Time last_double_click_time;
} titlebar_double_click;
static std::shared_ptr<Button> currently_hovered_button;
static rect manipulating_window_geometry;

static void* b_window_manager_detected = malloc(sizeof(bool));

std::shared_ptr<EshyWMWindow> window_list_contains_frame(Window frame)
{
    std::shared_ptr<EshyWMWindow> parent_window = nullptr;
    for(std::shared_ptr<EshyWMWindow> window : WindowManager::window_list)
    {
        if(window->get_frame() == frame)
        {
            parent_window = window;
            break;
        }
    }
    
    return parent_window;
}

std::shared_ptr<EshyWMWindow> window_list_contains_titlebar(Window titlebar)
{
    std::shared_ptr<EshyWMWindow> parent_window = nullptr;
    for(std::shared_ptr<EshyWMWindow> window : WindowManager::window_list)
    {
        if(window->get_titlebar() == titlebar)
        {
            parent_window = window;
            break;
        }
    }

    return parent_window;
}

std::shared_ptr<EshyWMWindow> window_list_contains_window(Window internal_window)
{
    std::shared_ptr<EshyWMWindow> parent_window = nullptr;
    for(std::shared_ptr<EshyWMWindow> window : WindowManager::window_list)
    {
        if(window->get_window() == internal_window)
        {
            parent_window = window;
            break;
        }
    }

    return parent_window;
}

void remove_from_window_list(std::shared_ptr<EshyWMWindow> window)
{
    WindowManager::window_list.erase(find(WindowManager::window_list.begin(), WindowManager::window_list.end(), window));
}


void update_monitor_info()
{
    int n_monitors;
    XRRMonitorInfo* monitors = XRRGetMonitors(DISPLAY, ROOT, false, &n_monitors);

    for(int i = 0; i < n_monitors; i++)
    {
        if(i < WindowManager::monitor_info.size())
        {
            WindowManager::monitor_info[i] = {(bool)monitors[i].primary, monitors[i].x, monitors[i].y, (uint)monitors[i].width, (uint)monitors[i].height};
        }
        else WindowManager::monitor_info.push_back({(bool)monitors[i].primary, monitors[i].x, monitors[i].y, (uint)monitors[i].width, (uint)monitors[i].height});
    }

    XRRFreeMonitors(monitors);
}

std::shared_ptr<EshyWMWindow> register_window(Window window, bool b_was_created_before_window_manager)
{
    //Do not frame already framed windows
    if(window_list_contains_window(window))
    {
        return nullptr;
    }
    
    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    XGetWindowAttributes(DISPLAY, window, &x_window_attributes);

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
    {
        return nullptr;
    }

    //Add so we can restore if we crash
    XAddToSaveSet(DISPLAY, window);

    //Create window
    std::shared_ptr<EshyWMWindow> new_window = std::make_shared<EshyWMWindow>(window);
    new_window->frame_window(x_window_attributes);
    new_window->setup_grab_events();
    WindowManager::window_list.push_back(new_window);

    Imlib_Image icon;

    //Try to retrieve icon from application
    static const Atom ATOM_NET_WM_ICON = XInternAtom(DISPLAY, "_NET_WM_ICON", false);
    static const Atom ATOM_CARDINAL = XInternAtom(DISPLAY, "CARDINAL", false);

    Atom type_return;
    int format_return;
    unsigned long nitems_return;
    unsigned long bytes_after_return;
    unsigned char* data_return = nullptr;

    XGetWindowProperty(DISPLAY, window, ATOM_NET_WM_ICON, 0, 1, false, ATOM_CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    if (data_return)
    {
        const int width = *(unsigned int*)data_return;
        XFree(data_return);

        XGetWindowProperty(DISPLAY, window, ATOM_NET_WM_ICON, 1, 1, false, ATOM_CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
        const int height = *(unsigned int*)data_return;
        XFree(data_return);

        XGetWindowProperty(DISPLAY, window, ATOM_NET_WM_ICON, 2, width * height, false, ATOM_CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
        uint32_t* img_data = new uint32_t[width * height];
        const ulong* ul = (ulong*)data_return;

        for(int i = 0; i < nitems_return; i++)
        {
            img_data[i] = (uint32_t)ul[i];
        }

        XFree(data_return);

        icon = imlib_create_image_using_data(width, height, img_data);
    }
    else icon = imlib_load_image("../images/application_icon.png");

    TASKBAR->add_button(new_window, icon);
    SWITCHER->add_window_option(new_window, icon);
    return new_window;
}


void handle_button_hovered(Window hovered_window, bool b_hovered, int mode)
{
    if (currently_hovered_button)
    {
        //Going from hovered to normal
        if (currently_hovered_button->get_button_state() == EButtonState::S_Hovered && !b_hovered && mode == NotifyNormal)
        {
            currently_hovered_button->set_button_state(EButtonState::S_Normal);
            currently_hovered_button = nullptr;
        }
        //Going from hovered to pressed
        else if (currently_hovered_button->get_button_state() == EButtonState::S_Hovered && !b_hovered && mode == NotifyGrab)
            currently_hovered_button->set_button_state(EButtonState::S_Pressed);
        //Going from pressed to hovered
        else if (currently_hovered_button->get_button_state() == EButtonState::S_Pressed && b_hovered && mode == NotifyUngrab)
            currently_hovered_button->set_button_state(EButtonState::S_Hovered);
    }

    //Going from normal to hovered (need extra logic because our currently_hovered_button will change here)
    if (((!currently_hovered_button) || (currently_hovered_button && currently_hovered_button->get_button_state() == EButtonState::S_Normal)) && b_hovered)
    {
        if(currently_hovered_button)
            currently_hovered_button->set_button_state(EButtonState::S_Normal);

        for(std::shared_ptr<WindowButton>& button : TASKBAR->get_taskbar_buttons())
        {
            if(button->get_window() == hovered_window)
            {
                currently_hovered_button = button;
                break;
            }
        }

        if(!currently_hovered_button)
            for(std::shared_ptr<EshyWMWindow>& window : WindowManager::window_list)
            {
                if(window->get_minimize_button()->get_window() == hovered_window)
                    currently_hovered_button = window->get_minimize_button();
                else if(window->get_maximize_button()->get_window() == hovered_window)
                    currently_hovered_button = window->get_maximize_button();
                else if(window->get_close_button()->get_window() == hovered_window)
                    currently_hovered_button = window->get_close_button();

                if(currently_hovered_button)
                    break;
            }

        if(currently_hovered_button)
            currently_hovered_button->set_button_state(EButtonState::S_Hovered);
    }
}


void OnDestroyNotify(const XDestroyWindowEvent& event)
{
    if(std::shared_ptr<EshyWMWindow> window = window_list_contains_frame(event.window))
    {
        TASKBAR->remove_button(window);
        SWITCHER->remove_window_option(window);
        remove_from_window_list(window);
    }
}

void OnUnmapNotify(const XUnmapEvent& event)
{
    safe_ensure(event.event != ROOT);

    /**If we manage window then unframe. Need to check
     * because we will receive an UnmapNotify event for
     * a frame window we just destroyed. Ignore event
     * if it is triggered by reparenting a window that
     * was mapped before the window manager started*/
    if(std::shared_ptr<EshyWMWindow> window = window_list_contains_frame(event.window))
    {
        window->unframe_window();
    }
    else if(std::shared_ptr<EshyWMWindow> window = window_list_contains_window(event.window))
    {
        window->unframe_window();
    }
}

void OnMapRequest(const XMapRequestEvent& event)
{
    register_window(event.window, false);
    //Map window
    XMapWindow(DISPLAY, event.window);
}

void OnConfigureNotify(const XConfigureEvent& event)
{
    if(event.window == ROOT && event.display == DISPLAY)
    {
        update_monitor_info();

        //Notify screen resolution changed
        EshyWM::on_screen_resolution_changed(event.width, event.height);
    }
}

void OnConfigureRequest(const XConfigureRequestEvent& event)
{
    XWindowChanges changes;
    changes.x = event.x;
    changes.y = event.y;
    changes.width = event.width;
    changes.height = event.height;
    changes.border_width = event.border_width;
    changes.sibling = event.above;
    changes.stack_mode = event.detail;

    if(std::shared_ptr<EshyWMWindow> window = window_list_contains_frame(event.window))
    {
        const Window frame = window->get_frame();
        XConfigureWindow(DISPLAY, frame, event.value_mask, &changes);
    }

    //Grant request
    XConfigureWindow(DISPLAY, event.window, event.value_mask, &changes);
}

void OnVisibilityNotify(const XVisibilityEvent& event)
{
    if(TASKBAR)
    {
        if(event.window == TASKBAR->get_menu_window())
            TASKBAR->raise();
        else
        {
            for(const std::shared_ptr<WindowButton>& button : TASKBAR->get_taskbar_buttons())
            {
                if(button && button->get_window() == event.window)
                {
                    button->draw();
                }
            }
        }
    }

    if(SWITCHER && event.window == SWITCHER->get_menu_window())
        SWITCHER->raise();
    else if(RUN_MENU && event.window == RUN_MENU->get_menu_window())
        RUN_MENU->raise();
    else if(std::shared_ptr<EshyWMWindow> window = window_list_contains_titlebar(event.window))
        window->update_titlebar();
}

void OnButtonPress(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(DISPLAY, ReplayPointer, event.time);
    
    if(event.window != CONTEXT_MENU->get_menu_window())
    {
        CONTEXT_MENU->remove();
    }

    if(event.window == SWITCHER->get_menu_window())
    {
        SWITCHER->button_clicked(event.x, event.y);
        return;
    }
    else if(event.window == ROOT)
    {
        //Only make context menu if we click on the desktop, not within the bounds of any window
        bool b_make_context_menu = true;
        for(const std::shared_ptr<EshyWMWindow>& window : WindowManager::window_list)
        {
            const rect geo = window->get_frame_geometry();
            if(event.x_root > geo.x && event.x_root < geo.x + geo.width && event.y_root > geo.y && event.y_root < geo.y + geo.height)
            {
                b_make_context_menu = false;
                break;
            }
        }

        if(b_make_context_menu)
        {
            CONTEXT_MENU->set_position(event.x_root, event.y_root);
            CONTEXT_MENU->show();
        }

        return;
    }

    if(std::shared_ptr<EshyWMWindow> fwindow = window_list_contains_frame(event.window))
    {
        fwindow->recalculate_all_window_size_and_location();
        XSetInputFocus(DISPLAY, fwindow->get_window(), RevertToPointerRoot, event.time);
        manipulating_window_geometry = fwindow->get_frame_geometry();
    }
    else if (std::shared_ptr<EshyWMWindow> twindow = window_list_contains_titlebar(event.window))
    {
        twindow->recalculate_all_window_size_and_location();
        XSetInputFocus(DISPLAY, twindow->get_window(), RevertToPointerRoot, event.time);
        manipulating_window_geometry = twindow->get_frame_geometry();

        //Handle double click for maximize in the titlebar
        if(titlebar_double_click.window == event.window
        && event.time - titlebar_double_click.first_click_time < EshyWMConfig::double_click_time)
        {
            WindowManager::_maximize_window(twindow);
            titlebar_double_click = {event.window, 0, event.time};
        }
        else titlebar_double_click = {event.window, event.time, 0};
    }
    else return;

    //Save cursor position on click
    click_cursor_position = {event.x_root, event.y_root};
    XRaiseWindow(DISPLAY, event.window);
}

void OnButtonRelease(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(DISPLAY, ReplayPointer, event.time);

    if(currently_hovered_button)
        currently_hovered_button->click();
}

void OnMotionNotify(const XMotionEvent& event)
{
    const Vector2D<int> delta = {event.x_root - click_cursor_position.x, event.y_root - click_cursor_position.y};

    std::shared_ptr<EshyWMWindow> fwindow = window_list_contains_frame(event.window);
    std::shared_ptr<EshyWMWindow> twindow = window_list_contains_titlebar(event.window);
    if(fwindow && event.state & Mod4Mask && event.state & ShiftMask)
    {        
        if(event.state & Button1Mask)
            fwindow->move_window_absolute(manipulating_window_geometry.x + delta.x, manipulating_window_geometry.y + delta.y);
        else if(event.state & Button3Mask)
            fwindow->resize_window_absolute(std::max((int)manipulating_window_geometry.width + delta.x, 10), std::max((int)manipulating_window_geometry.height + delta.y, 10));
    }
    else if(twindow && event.time - titlebar_double_click.last_double_click_time > 10)
        twindow->move_window_absolute(manipulating_window_geometry.x + delta.x, manipulating_window_geometry.y + delta.y);
}

void OnKeyPress(const XKeyEvent& event)
{
    std::shared_ptr<EshyWMWindow> window = window_list_contains_frame(event.window);
    if(event.window == ROOT)
    {
        CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod1Mask, XK_Tab)
        {
            SWITCHER->show();
            SWITCHER->next_option();
        }
        ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_R)
        RUN_MENU->show();
    }
    else if(event.state & Mod4Mask && window)
    {
        CHECK_KEYSYM_PRESSED(event, XK_C)
        WindowManager::_close_window(window);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_f | XK_F)
        WindowManager::_maximize_window(window);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_a | XK_A)
        WindowManager::_minimize_window(window);
    }

    XAllowEvents(DISPLAY, ReplayKeyboard, event.time);
}

void OnKeyRelease(const XKeyEvent& event)
{
    if(event.window == ROOT && SWITCHER && SWITCHER->get_menu_active())
    {
        CHECK_KEYSYM_PRESSED(event, XK_Alt_L)
        SWITCHER->confirm_choice();
    }
}

namespace WindowManager
{
void initialize()
{
    display = XOpenDisplay(nullptr);
    ensure(display)

    auto OnWMDetected = [](Display* display, XErrorEvent* event)
    {
        ensure(static_cast<int>(event->error_code) == BadAccess)
        *(bool*)b_window_manager_detected = true;
        return 0;
    };

    XSetErrorHandler(OnWMDetected);
    XSelectInput(DISPLAY, ROOT, SubstructureRedirectMask | StructureNotifyMask | SubstructureNotifyMask);
    XSync(DISPLAY, false);
    ensure(!(*(bool*)(b_window_manager_detected)))
    b_window_manager_detected = malloc(0);

    update_monitor_info();
    titlebar_double_click = {0, 0, 0};
}

void main_loop()
{
    XEvent event;
    XNextEvent(DISPLAY, &event);
    LOG_EVENT_INFO(LS_Verbose, event)
    
    switch (event.type)
    {
    case DestroyNotify:
        OnDestroyNotify(event.xdestroywindow);
        break;
    case MapRequest:
        OnMapRequest(event.xmaprequest);
        break;
    case UnmapNotify:
        OnUnmapNotify(event.xunmap);
        break;
    case ConfigureNotify:
        OnConfigureNotify(event.xconfigure);
        break;
    case ConfigureRequest:
        OnConfigureRequest(event.xconfigurerequest);
        break;
    case VisibilityNotify:
        OnVisibilityNotify(event.xvisibility);
        break;
    case ButtonPress:
        OnButtonPress(event.xbutton);
        break;
    case ButtonRelease:
        OnButtonRelease(event.xbutton);
        break;
    case MotionNotify:
        while (XCheckTypedWindowEvent(DISPLAY, event.xmotion.window, MotionNotify, &event)) {}
        OnMotionNotify(event.xmotion);
        break;
    case KeyPress:
        OnKeyPress(event.xkey);
        break;
    case KeyRelease:
        OnKeyRelease(event.xkey);
        break;
    case EnterNotify:
        handle_button_hovered(event.xcrossing.window, true, event.xcrossing.mode);
        break;
    case LeaveNotify:
        handle_button_hovered(event.xcrossing.window, false, event.xcrossing.mode);
        break;
    };
}

void handle_preexisting_windows()
{
    auto OnXError = [](Display* display, XErrorEvent* event)
    {
        const int MAX_ERROR_TEXT_LEGTH = 1024;
        char error_text[MAX_ERROR_TEXT_LEGTH];
        XGetErrorText(display, event->error_code, error_text, sizeof(error_text));
        return 0;
    };

    XSetErrorHandler(OnXError);
    XGrabServer(DISPLAY);

    Window returned_root;
    Window returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(DISPLAY, ROOT, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

    for(unsigned int i = 0; i < num_top_level_windows; ++i)
    {
        if(top_level_windows[i] != TASKBAR->get_menu_window()
        && top_level_windows[i] != SWITCHER->get_menu_window()
        && top_level_windows[i] != CONTEXT_MENU->get_menu_window())
        {
            register_window(top_level_windows[i], true);
        }
    }

    XFree(top_level_windows);
    XUngrabServer(DISPLAY);
}
};