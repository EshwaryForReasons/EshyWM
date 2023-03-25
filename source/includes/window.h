#pragma once

#include "util.h"

namespace WindowManager
{
void _minimize_window(std::shared_ptr<class EshyWMWindow> window, void* null = nullptr);
void _maximize_window(std::shared_ptr<class EshyWMWindow> window, void* b_from_move_or_resize = (void*)false);
void _close_window(std::shared_ptr<class EshyWMWindow> window, void* null = nullptr);
};

/**
 * Handles everything about an individual window
*/
class EshyWMWindow : public std::enable_shared_from_this<EshyWMWindow>
{
public:

    EshyWMWindow(Window _window)
        : window(_window)
        , b_is_maximized(false)
        , b_is_minimized(false)
    {}

    void frame_window(XWindowAttributes attr);
    void unframe_window();
    void setup_grab_events();
    void remove_grab_events();

    void move_window_absolute(int new_position_x, int new_position_y, bool b_from_maximize = false);
    void resize_window_absolute(uint new_size_x, uint new_size_y, bool b_from_maximize = false);

    void recalculate_all_window_size_and_location();
    void set_size_according_to(uint new_width, uint new_height);

    /**@return 0 = none; 1 = minimize; 2 = maximize; 3 = close*/
    void update_titlebar();

    /**Getters*/
    Window get_window() const {return window;}
    Window get_frame() const {return frame;}
    Window get_titlebar() const {return titlebar;}
    rect get_frame_geometry() const {return frame_geometry;}
    rect get_titlebar_geometry() const {return titlebar_geometry;}
    rect get_window_geometry() const {return window_geometry;}
    bool is_minimized() const {return b_is_minimized;}
    bool is_maximized() const {return b_is_maximized;}
    std::shared_ptr<class WindowButton> get_minimize_button() const {return minimize_button;}
    std::shared_ptr<class WindowButton> get_maximize_button() const {return maximize_button;}

    void set_minimized(bool b_new_minimized) {b_is_minimized = b_new_minimized;}
    void set_maximized(bool b_new_maximized) {b_is_maximized = b_new_maximized;}
    std::shared_ptr<class WindowButton> get_close_button() const {return close_button;}

public:

    Window window;
    Window frame;
    Window titlebar;

    rect frame_geometry;
    rect titlebar_geometry;
    rect window_geometry;
    rect pre_minimize_and_maximize_saved_geometry;

    bool b_is_maximized;
    bool b_is_minimized;

    GC graphics_context_internal;

    std::shared_ptr<class WindowButton> minimize_button;
    std::shared_ptr<class WindowButton> maximize_button;
    std::shared_ptr<class WindowButton> close_button;
};