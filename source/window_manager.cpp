#include "window_manager.h"
#include "eshywm.h"
#include "window.h"
#include "config.h"
#include "context_menu.h"
#include "taskbar.h"
#include "switcher.h"
#include "run_menu.h"

#include <X11/Xutil.h>
#include <glog/logging.h>
#include <cstring>
#include <iostream>
#include <algorithm>

#define CHECK_KEYSYM_TO_KEYCODE(event, key_code)      if(event.keycode == XKeysymToKeycode(DISPLAY, key_code))
#define ELSE_CHECK_KEYSYM_TO_KEYCODE(event, key_code) else if(event.keycode == XKeysymToKeycode(DISPLAY, key_code))

bool WindowManager::b_wm_detected;
std::mutex WindowManager::mutex_wm_detected;
Display* WindowManager::display;

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


std::shared_ptr<WindowManager> WindowManager::Create(const std::string& display_str)
{
    display = XOpenDisplay(display_str.empty() ? nullptr : display_str.c_str());

    if(display == nullptr)
    {
        LOG(ERROR) << "Failed to open X display " << XDisplayName(nullptr);
        return nullptr;
    }

    return std::make_shared<WindowManager>();
}

WindowManager::WindowManager()
    : root(DefaultRootWindow(display))
    , manager_data(new window_manager_data())
{
    imlib_context_set_display(display);
    imlib_context_set_visual(DefaultVisual(display, DefaultScreen(display)));
    imlib_context_set_colormap(DefaultColormap(display, DefaultScreen(display)));

    display_width = DisplayWidth(display, DefaultScreen(display));
    display_height = DisplayHeight(display, DefaultScreen(display)) - CONFIG->taskbar_height;
}

WindowManager::~WindowManager()
{
    XCloseDisplay(display);
}

void WindowManager::initialize()
{
    std::lock_guard<std::mutex> lock(mutex_wm_detected);

    b_wm_detected = false;
    XSetErrorHandler(&WindowManager::OnWMDetected);
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | StructureNotifyMask);
    XSync(display, false);

    if(b_wm_detected)
    {
        LOG(ERROR) << "Detected another window manager on display " << XDisplayString(display);
        return;
    }

    //Create root container
    root_container = std::make_shared<container>(EOrientation::O_Horizontal, EContainerType::CT_Root);
    root_container->set_size(DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

void WindowManager::handle_preexisting_windows()
{
    XSetErrorHandler(&WindowManager::OnXError);
    XGrabServer(display);

    Window returned_root;
    Window returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(display, root, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

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
    XUngrabServer(display);
}

int WindowManager::OnWMDetected(Display* display, XErrorEvent* event)
{
    CHECK_EQ(static_cast<int>(event->error_code), BadAccess);
    b_wm_detected = true;
    return 0;
}

int WindowManager::OnXError(Display* display, XErrorEvent* event)
{
    const int MAX_ERROR_TEXT_LEGTH = 1024;
    char error_text[MAX_ERROR_TEXT_LEGTH];

    XGetErrorText(display, event->error_code, error_text, sizeof(error_text));

    return 0;
}

void WindowManager::main_loop()
{
    //Make sure the lists are up to date
    frame_list = root_container->get_all_container_map_by_frame();
    titlebar_list = root_container->get_all_container_map_by_titlebar();

    XEvent event;
    XNextEvent(display, &event);

    switch (event.type)
    {
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
        while (XCheckTypedWindowEvent(display, event.xmotion.window, MotionNotify, &event)) {}
        OnMotionNotify(event.xmotion);
        break;
    case KeyPress:
        OnKeyPress(event.xkey);
        break;
    case KeyRelease:
        OnKeyRelease(event.xkey);
        break;
    }

    for(auto &[titlebar, container] : titlebar_list)
    {
        container->get_window()->draw();
    }

    TASKBAR->draw();
    SWITCHER->draw();
    CONTEXT_MENU->draw();
}

void WindowManager::OnConfigureNotify(const XConfigureEvent& event)
{
    if(event.window == root && event.display == display)
    {
        const uint previous_width = DISPLAY_WIDTH;
        const uint previous_height = DISPLAY_HEIGHT;
        display_width = event.width;
        display_height = event.height - CONFIG->taskbar_height;
     
        //Notify screen resolution changed
        EshyWM::Get().on_screen_resolution_changed(event.width, event.height);
        rescale_windows(previous_width, previous_height);
    }
}

void WindowManager::OnUnmapNotify(const XUnmapEvent& event)
{
    //If we manage window then unframe. Need to check bcause we will receive an UnmapNotify event for a frame window we just destroyed
    if(!frame_list.count(event.window))
    {
        LOG(INFO) << "Ignored UnmapNotify for non-client window " << event.window;
        return;
    }

    //Ignore event if it is triggered by reparenting a window that was mapped before the window manager started
    if(event.event == root)
    {
        LOG(INFO) << "Ignored UnmapNotify for reparented pre-existing window " << event.window;
        return;
    }

    frame_list.at(event.window)->get_window()->unframe_window();
}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent& event)
{
    XWindowChanges changes;
    changes.x = event.x;
    changes.y = event.y;
    changes.width = event.width;
    changes.height = event.height;
    changes.border_width = event.border_width;
    changes.sibling = event.above;
    changes.stack_mode = event.detail;

    if(frame_list.count(event.window))
    {
        const Window frame = event.window;
        XConfigureWindow(display, frame, event.value_mask, &changes);
    }

    //Grant request
    XConfigureWindow(display, event.window, event.value_mask, &changes);
}

void WindowManager::OnMapRequest(const XMapRequestEvent& event)
{
    register_window(event.window, false);
    //Map window
    XMapWindow(display, event.window);
}

void WindowManager::OnVisibilityNotify(const XVisibilityEvent& event)
{
    if(TASKBAR && event.window == TASKBAR->get_menu_window())
    {
        TASKBAR->raise();
    }
    else if(SWITCHER && event.window == SWITCHER->get_menu_window())
    {
        SWITCHER->raise();
    }
    else if(RUN_MENU && event.window == RUN_MENU->get_menu_window())
    {
        RUN_MENU->raise();
    }
}

void WindowManager::OnButtonPress(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(display, ReplayPointer, event.time);

    if(event.window == ROOT)
    {
        Window returned_root;
        Window returned_parent;
        Window* top_level_windows;
        unsigned int num_top_level_windows;
        XQueryTree(display, root, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

        bool b_is_above_window = false;

        for(unsigned int i = 0; i < num_top_level_windows; i++)
        {
            if(top_level_windows[i] == ROOT)
            {
                continue;
            }

            XWindowAttributes attr;
            XGetWindowAttributes(DISPLAY, top_level_windows[i], &attr);

            if(event.x_root > attr.x && event.x_root < attr.x + attr.width && event.y_root > attr.y && event.y_root < attr.y + attr.height)
            {
                b_is_above_window = true;
                break;
            }
        }

        XFree(top_level_windows);

        if(!b_is_above_window)
        {
            CONTEXT_MENU->set_position(event.x, event.y);
            CONTEXT_MENU->show();
        }

        return;
    }

    if(event.window != CONTEXT_MENU->get_menu_window())
    {
        CONTEXT_MENU->remove();
    }

    if(event.window == TASKBAR->get_menu_window())
    {
        TASKBAR->check_taskbar_button_clicked(event.x, event.y);
        return;
    }

    if(frame_list.count(event.window))
    {
        frame_list.at(event.window)->get_window()->recalculate_all_window_size_and_location();
    }
    else if (titlebar_list.count(event.window))
    {
        titlebar_list.at(event.window)->get_window()->recalculate_all_window_size_and_location();
        check_titlebar_button_pressed(event.window, event.x, event.y);

        if(event.time - titlebar_double_click.first_click_time < CONFIG->double_click_time && titlebar_double_click.first_click_window == event.window)
        titlebar_list.at(event.window)->get_window()->maximize_window();
        else titlebar_double_click = {event.window, event.time};
    }
    else return;

    //Save cursor position on click
    click_cursor_position = Vector2D<int>(event.x_root, event.y_root);
    XRaiseWindow(display, event.window);
    XSetInputFocus(display, event.window, RevertToPointerRoot, event.time);
}

void WindowManager::OnButtonRelease(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(display, ReplayPointer, event.time);

    if(event.state & Button1Mask || event.state & Button3Mask)
    {
        if(event.window == root && event.state & Button3Mask)
        {
            CONTEXT_MENU->set_position(event.x, event.y);
            CONTEXT_MENU->show();
        }

        if(frame_list.count(event.window))
        {
            frame_list.at(event.window)->get_window()->motion_modify_ended();
        }
        else if (titlebar_list.count(event.window))
        {
            titlebar_list.at(event.window)->get_window()->motion_modify_ended();
        }
    }
}

void WindowManager::OnMotionNotify(const XMotionEvent& event)
{
    const Vector2D<int> delta = Vector2D<int>(event.x_root, event.y_root) - click_cursor_position;

    if(event.state & Mod4Mask && event.state & ShiftMask)
    {
        if(event.state & Button1Mask)
        {
           frame_list.at(event.window)->get_window()->move_window(delta);
           frame_list.at(event.window)->get_window()->draw();
        }
        else if(event.state & Button3Mask)
        {
           frame_list.at(event.window)->get_window()->resize_window(delta);
        }

        return;
    }

    if(titlebar_list.count(event.window))
    {
        titlebar_list.at(event.window)->get_window()->move_window(delta);
        titlebar_list.at(event.window)->get_window()->draw();
    }
}

void WindowManager::OnKeyPress(const XKeyEvent& event)
{
    XAllowEvents(display, ReplayKeyboard, event.time);

    if(!(event.state & Mod4Mask))
    {
        return;
    }

    if(event.window == root)
    {
        if(event.keycode == XKeysymToKeycode(DISPLAY, XK_Tab))
        {
            SWITCHER->show();
            SWITCHER->next_option();
        }
        else if (event.keycode == XKeysymToKeycode(DISPLAY, XK_R))
        {
            RUN_MENU->show();
        }
    }

    CHECK_KEYSYM_TO_KEYCODE(event, XK_C)
    frame_list.at(event.window)->get_window()->close_window();
    else if(event.state & ControlMask)
    {
        CHECK_KEYSYM_TO_KEYCODE(event, XK_Left)
        frame_list.at(event.window)->move_window_horizontal_left_arrow();
        ELSE_CHECK_KEYSYM_TO_KEYCODE(event, XK_Right)
        frame_list.at(event.window)->move_window_horizontal_right_arrow();
        ELSE_CHECK_KEYSYM_TO_KEYCODE(event, XK_Up)
        frame_list.at(event.window)->move_window_vertical_up_arrow();
        ELSE_CHECK_KEYSYM_TO_KEYCODE(event, XK_Down)
        frame_list.at(event.window)->move_window_vertical_down_arrow();
    }
    ELSE_CHECK_KEYSYM_TO_KEYCODE(event, XK_f | XK_F)
    frame_list.at(event.window)->get_window()->maximize_window();
    ELSE_CHECK_KEYSYM_TO_KEYCODE(event, XK_a | XK_A)
    frame_list.at(event.window)->get_window()->minimize_window();
}

void WindowManager::OnKeyRelease(const XKeyEvent& event)
{
    if(!is_key_down(display, XKeysymToString(XK_Alt_L)))
    {
        SWITCHER->confirm_choice();
    }
}


std::shared_ptr<EshyWMWindow> WindowManager::register_window(Window window, bool b_was_created_before_window_manager)
{
    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    XGetWindowAttributes(display, window, &x_window_attributes);

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
    {
        return nullptr;
    }

    //Add so we can restore if we crash
    XAddToSaveSet(get_display(), window);

    std::shared_ptr<container> leaf_container = std::make_shared<container>(EOrientation::O_Horizontal, EContainerType::CT_Leaf);
    leaf_container->create_window(window);
    root_container->add_internal_container(leaf_container);
    TASKBAR->add_button(leaf_container->get_window());
    SWITCHER->add_window_option(leaf_container->get_window());
    return leaf_container->get_window();
}

void WindowManager::close_window(std::shared_ptr<class EshyWMWindow> closed_window)
{
    TASKBAR->remove_button(closed_window);
}

void WindowManager::rescale_windows(uint old_width, uint old_height)
{
   
}

void WindowManager::check_titlebar_button_pressed(Window window, int cursor_x, int cursor_y)
{
    //This will check if the window is a frame (so we can't check the same area, but for a client window)
    if(titlebar_list.count(window))
    {
        const int button_pressed = titlebar_list.at(window)->get_window()->is_cursor_on_titlebar_buttons(window, cursor_x, cursor_y);

        if(button_pressed == 1)
        titlebar_list.at(window)->get_window()->minimize_window();
        else if(button_pressed == 2)
        titlebar_list.at(window)->get_window()->maximize_window();
        else if(button_pressed == 3)
        titlebar_list.at(window)->get_window()->close_window();
    }
}