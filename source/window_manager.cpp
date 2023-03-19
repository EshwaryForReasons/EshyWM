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

Display* WindowManager::Internal::display;

Vector2D<int> WindowManager::Internal::click_cursor_position;
std::shared_ptr<class container> WindowManager::Internal::root_container;

bool WindowManager::Internal::b_tiling_mode;
std::vector<s_monitor_info> WindowManager::Internal::monitor_info;

organized_container_map_t WindowManager::Internal::frame_list;
organized_container_map_t WindowManager::Internal::titlebar_list;
std::vector<std::shared_ptr<Button>> WindowManager::Internal::button_list;

double_click_data WindowManager::Internal::titlebar_double_click;

static bool b_window_manager_detected;

bool is_key_down(Display* display, char* target_string)
{
    char keys_return[32] = {0};
    KeySym target_sym = XStringToKeysym(target_string);
    KeyCode target_code = XKeysymToKeycode(display, target_sym);

    int target_byte = target_code / 8;
    int target_bit = target_code % 8;
    int target_mask = 0x01 << target_bit;

    XQueryKeymap(display, keys_return);
    return keys_return[target_byte] & target_mask;
}

void check_titlebar_button_pressed(Window window, int cursor_x, int cursor_y)
{
    //This will check if the window is a frame (so we can't check the same area, but for a client window)
    if(WindowManager::Internal::titlebar_list.count(window))
    {
        const int button_pressed = WindowManager::Internal::titlebar_list.at(window)->get_window()->is_cursor_on_titlebar_buttons(window, cursor_x, cursor_y);

        if(button_pressed == 1)
        WindowManager::Internal::titlebar_list.at(window)->get_window()->minimize_window();
        else if(button_pressed == 2)
        WindowManager::Internal::titlebar_list.at(window)->get_window()->maximize_window();
        else if(button_pressed == 3)
        WindowManager::Internal::titlebar_list.at(window)->get_window()->close_window();
    }
}

void update_monitor_info()
{
    int n_monitors;
    XRRMonitorInfo* monitors = XRRGetMonitors(DISPLAY, ROOT, false, &n_monitors);

    for(int i = 0; i < n_monitors; i++)
    {
        if(i < WindowManager::Internal::monitor_info.size())
        WindowManager::Internal::monitor_info[i] = {(bool)monitors[i].primary, monitors[i].x, monitors[i].y, monitors[i].width, monitors[i].height};
        else WindowManager::Internal::monitor_info.push_back({(bool)monitors[i].primary, monitors[i].x, monitors[i].y, monitors[i].width, monitors[i].height});
    }

    XRRFreeMonitors(monitors);
}

int OnWMDetected(Display* display, XErrorEvent* event)
{
    ensure(static_cast<int>(event->error_code) == BadAccess)
    b_window_manager_detected = true;
    return 0;
}

int OnXError(Display* display, XErrorEvent* event)
{
    const int MAX_ERROR_TEXT_LEGTH = 1024;
    char error_text[MAX_ERROR_TEXT_LEGTH];
    XGetErrorText(display, event->error_code, error_text, sizeof(error_text));
    return 0;
}

std::shared_ptr<class EshyWMWindow> register_window(Window window, bool b_was_created_before_window_manager)
{
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

    //Do this before worrying about the icons
    std::shared_ptr<container> leaf_container = std::make_shared<container>(EOrientation::O_Horizontal, EContainerType::CT_Leaf);
    leaf_container->create_window(window, x_window_attributes);
    WindowManager::Internal::root_container->add_internal_container(leaf_container);

    uint32_t* img_data = nullptr;
    Imlib_Image icon;

    //Try to retrieve icon from application
    const Atom NET_WM_ICON = XInternAtom(DISPLAY, "_NET_WM_ICON", false);
    const Atom CARDINAL = XInternAtom(DISPLAY, "CARDINAL", false);

    Atom type_return;
    int format_return;
    unsigned long nitems_return;
    unsigned long bytes_after_return;
    unsigned char* data_return = nullptr;

    XGetWindowProperty(DISPLAY, window, NET_WM_ICON, 0, 1, false, CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    if (data_return)
    {
        const int width = *(unsigned int*)data_return;
        XFree(data_return);

        XGetWindowProperty(DISPLAY, window, NET_WM_ICON, 1, 1, false, CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
        const int height = *(unsigned int*)data_return;
        XFree(data_return);

        XGetWindowProperty(DISPLAY, window, NET_WM_ICON, 2, width * height, false, CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
        img_data = new uint32_t[width * height];
        const ulong* ul = (ulong*)data_return;

        for(int i = 0; i < nitems_return; i++)
        {
            img_data[i] = (uint32_t)ul[i];
        }

        XFree(data_return);

        icon = imlib_create_image_using_data(width, height, img_data);
    }
    
    //Load default icon if icon does not exist
    if (!img_data)
    {
        icon = imlib_load_image("../images/application_icon.png");
    }
    TASKBAR->add_button(leaf_container->get_window(), icon);
    SWITCHER->add_window_option(leaf_container->get_window(), icon);
    return leaf_container->get_window();
}


void OnDestroyNotify(const XDestroyWindowEvent& event)
{
    if(WindowManager::Internal::frame_list.count(event.window))
    {
        TASKBAR->remove_button(WindowManager::Internal::frame_list.at(event.window)->get_window());
        SWITCHER->remove_window_option(WindowManager::Internal::frame_list.at(event.window)->get_window());
        WindowManager::Internal::root_container->remove_internal_container(WindowManager::Internal::frame_list.at(event.window));
    }
}

void OnUnmapNotify(const XUnmapEvent& event)
{
    /**If we manage window then unframe. Need to check
     * because we will receive an UnmapNotify event for
     * a frame window we just destroyed. Ignore event
     * if it is triggered by reparenting a window that
     * was mapped before the window manager started*/
    if(WindowManager::Internal::frame_list.count(event.window) && event.event != ROOT)
    {
        WindowManager::Internal::frame_list.at(event.window)->get_window()->unframe_window();
    }
    // else
    // {
    //     for(auto const& [window, c] : WindowManager::Internal::frame_list)
    //     {
    //         const Window found_window = c->get_window()->get_window();
    //         if(found_window == event.window)
    //         {
    //             WindowManager::Internal::frame_list.at(c->get_window()->get_frame())->get_window()->unframe_window();
    //             break;
    //         }
    //     }
    // }
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

void OnMapRequest(const XMapRequestEvent& event)
{
    register_window(event.window, false);
    //Map window
    XMapWindow(DISPLAY, event.window);
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

    if(WindowManager::Internal::frame_list.count(event.window))
    {
        const Window frame = WindowManager::Internal::frame_list.at(event.window)->get_window()->get_frame();
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
        {
            TASKBAR->raise();
        }
        else
        {
            for(const window_button_pair& wbp : TASKBAR->get_taskbar_buttons())
            {
                if(wbp.button && wbp.button->get_window() == event.window)
                {
                    wbp.button->draw();
                }
            }
        }
    }

    if(SWITCHER && event.window == SWITCHER->get_menu_window())
    {
        SWITCHER->raise();
    }
    else if(RUN_MENU && event.window == RUN_MENU->get_menu_window())
    {
        RUN_MENU->raise();
    }
    else if(WindowManager::Internal::titlebar_list.count(event.window))
    {
        WindowManager::Internal::titlebar_list.at(event.window)->get_window()->update_titlebar();
    }
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
    else if(event.window == TASKBAR->get_menu_window())
    {
        TASKBAR->check_taskbar_button_clicked(event.x, event.y);
        return;
    }
    else if(event.window == ROOT)
    {
        bool b_make_context_menu = true;
        for(auto const& [window, c] : WindowManager::Internal::frame_list)
        {
            const rect geo = c->get_window()->get_frame_geometry();
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

    if(WindowManager::Internal::frame_list.count(event.window))
    {
        WindowManager::Internal::frame_list.at(event.window)->get_window()->recalculate_all_window_size_and_location();
        XSetInputFocus(DISPLAY, WindowManager::Internal::frame_list.at(event.window)->get_window()->get_window(), RevertToPointerRoot, event.time);
    }
    else if (WindowManager::Internal::titlebar_list.count(event.window))
    {
        WindowManager::Internal::titlebar_list.at(event.window)->get_window()->recalculate_all_window_size_and_location();
        check_titlebar_button_pressed(event.window, event.x, event.y);
        XSetInputFocus(DISPLAY, WindowManager::Internal::titlebar_list.at(event.window)->get_window()->get_window(), RevertToPointerRoot, event.time);

        if(event.time - WindowManager::Internal::titlebar_double_click.first_click_time < EshyWMConfig::double_click_time
        && WindowManager::Internal::titlebar_double_click.window == event.window)
        {
            WindowManager::Internal::titlebar_list.at(event.window)->get_window()->maximize_window();
            WindowManager::Internal::titlebar_double_click = {event.window, 0, event.time};
        }
        else WindowManager::Internal::titlebar_double_click = {event.window, event.time, 0};
    }
    else return;

    //Save cursor position on click
    WindowManager::Internal::click_cursor_position = Vector2D<int>(event.x_root, event.y_root);
    XRaiseWindow(DISPLAY, event.window);
}

void OnButtonRelease(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(DISPLAY, ReplayPointer, event.time);

    if(event.state & Button1Mask || event.state & Button3Mask)
    {
        if(event.window == ROOT && event.state & Button3Mask)
        {
            CONTEXT_MENU->set_position(event.x, event.y);
            CONTEXT_MENU->show();
        }

        if(WindowManager::Internal::frame_list.count(event.window))
        {
            WindowManager::Internal::frame_list.at(event.window)->get_window()->motion_modify_ended();
        }
        else if (WindowManager::Internal::titlebar_list.count(event.window))
        {
            WindowManager::Internal::titlebar_list.at(event.window)->get_window()->motion_modify_ended();
        }
    }
}

void OnMotionNotify(const XMotionEvent& event)
{
    const Vector2D<int> delta = Vector2D<int>(event.x_root, event.y_root) - WindowManager::Internal::click_cursor_position;

    if(event.state & Mod4Mask && event.state & ShiftMask)
    {
        if(event.state & Button1Mask)
        WindowManager::Internal::frame_list.at(event.window)->get_window()->move_window(delta);
        else if(event.state & Button3Mask)
        WindowManager::Internal::frame_list.at(event.window)->get_window()->resize_window(delta);
    }
    else if(WindowManager::Internal::titlebar_list.count(event.window)
    && event.time - WindowManager::Internal::titlebar_double_click.last_double_click_time > 10)
    {
        WindowManager::Internal::titlebar_list.at(event.window)->get_window()->move_window(delta);
    }
}

void OnKeyPress(const XKeyEvent& event)
{
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
    else if(event.state & Mod4Mask && WindowManager::Internal::frame_list.count(event.window))
    {
        CHECK_KEYSYM_PRESSED(event, XK_C)
        WindowManager::Internal::frame_list.at(event.window)->get_window()->close_window();
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_f | XK_F)
        WindowManager::Internal::frame_list.at(event.window)->get_window()->maximize_window();
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_a | XK_A)
        WindowManager::Internal::frame_list.at(event.window)->get_window()->minimize_window();
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

void OnEnterNotify(const XAnyEvent& event)
{
    for(std::shared_ptr<Button> button : WindowManager::Internal::button_list)
    {
        if(std::shared_ptr<WindowButton> window_button = std::dynamic_pointer_cast<WindowButton>(button))
        {
            if(window_button->get_window() == event.window)
            {
                window_button->set_hovered(true);
                break;
            }
        }
        else if(std::shared_ptr<ImageButton> image_button = std::dynamic_pointer_cast<ImageButton>(button))
        {
            if(image_button->get_window() == event.window)
            {
                image_button->set_hovered(true);
                break;
            }
        }
        else
        {
            Window root_return;
            Window child_return;
            int root_x_return;
            int root_y_return;
            int win_x_return;
            int win_y_return;
            uint mask_return;
            XQueryPointer(DISPLAY, ROOT , &root_return, &child_return, &root_x_return, &root_y_return, &win_x_return, &win_y_return, &mask_return);

            if(button->check_hovered(root_x_return, root_y_return))
            {
                window_button->set_hovered(true);
                break;
            }
        }
    }
}

void OnLeaveNotify(const XAnyEvent& event)
{
    for(std::shared_ptr<Button> button : WindowManager::Internal::button_list)
    {
        if(std::shared_ptr<WindowButton> window_button = std::dynamic_pointer_cast<WindowButton>(button))
        {
            if(window_button->get_window() == event.window)
            {
                window_button->set_hovered(false);
                break;
            }
        }
        else
        {
            Window root_return;
            Window child_return;
            int root_x_return;
            int root_y_return;
            int win_x_return;
            int win_y_return;
            uint mask_return;
            XQueryPointer(DISPLAY, ROOT , &root_return, &child_return, &root_x_return, &root_y_return, &win_x_return, &win_y_return, &mask_return);

            if(button->check_hovered(root_x_return, root_y_return))
            {
                window_button->set_hovered(false);
                break;
            }
        }
    }
}

namespace WindowManager
{
void initialize()
{
    Internal::display = XOpenDisplay(nullptr);
    ensure(Internal::display)

    update_monitor_info();

    Internal::titlebar_double_click = {0, 0, 0};

    b_window_manager_detected = false;
    XSetErrorHandler(&OnWMDetected);
    XSelectInput(DISPLAY, ROOT, SubstructureRedirectMask | StructureNotifyMask | SubstructureNotifyMask);
    XSync(DISPLAY, false);
    ensure(!b_window_manager_detected)

    Internal::root_container = std::make_shared<container>(EOrientation::O_Horizontal, EContainerType::CT_Root);
    Internal::root_container->set_size(DISPLAY_WIDTH(0), DISPLAY_HEIGHT(0));
}

void main_loop()
{
    XEvent event;
    XNextEvent(DISPLAY, &event);
    
    //Make sure the lists are up to date
    Internal::frame_list = Internal::root_container->get_all_container_map_by_frame();
    Internal::titlebar_list = Internal::root_container->get_all_container_map_by_titlebar();

    switch (event.type)
    {
    case DestroyNotify:
        OnDestroyNotify(event.xdestroywindow);
        break;
    case UnmapNotify:
        OnUnmapNotify(event.xunmap);
        break;
    case ConfigureNotify:
        OnConfigureNotify(event.xconfigure);
        break;
    case MapRequest:
        OnMapRequest(event.xmaprequest);
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
        OnEnterNotify(event.xany);
        break;
    case LeaveNotify:
        OnLeaveNotify(event.xany);
        break;
    }
}

void handle_preexisting_windows()
{
    XSetErrorHandler(&OnXError);
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

void add_button(std::shared_ptr<Button> button)
{
    Internal::button_list.push_back(button);
}

void remove_button(std::shared_ptr<Button> button)
{
    const std::vector<std::shared_ptr<Button>>::const_iterator it = std::find(Internal::button_list.cbegin(), Internal::button_list.cend(), button);
    if(it != Internal::button_list.cend())
    {
        Internal::button_list.erase(it);
    }
}
};