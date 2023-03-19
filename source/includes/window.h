#pragma once

#include "util.h"

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
    void minimize_window();
    void maximize_window(bool b_from_move_or_resize = false);
    void close_window();

    void move_window(Vector2D<int> delta) {move_window(delta.x, delta.y);}
    void move_window_absolute(Vector2D<int> new_position) {move_window_absolute(new_position.x, new_position.y);}
    void move_window(int delta_x, int delta_y);
    void move_window_absolute(int new_position_x, int new_position_y);
    void resize_window(Vector2D<int> delta) {resize_window(delta.x, delta.y);}
    void resize_window_absolute(Vector2D<uint> new_size) {resize_window_absolute(new_size.x, new_size.y);}
    void resize_window(int delta_x, int delta_y);
    void resize_window_absolute(uint new_size_x, uint new_position_y);
    void motion_modify_ended();
    void recalculate_all_window_size_and_location();
    void set_size_according_to(uint new_width, uint new_height);

    /**@return 0 = none; 1 = minimize; 2 = maximize; 3 = close*/
    int is_cursor_on_titlebar_buttons(Window window, int cursor_x, int cursor_y);
    void update_titlebar();

    /**Getters*/
    Window get_window() const {return window;}
    Window get_frame() const {return frame;}
    Window get_titlebar() const {return titlebar;}
    rect get_frame_geometry() const {return frame_geometry;}
    rect get_titlebar_geometry() const {return titlebar_geometry;}
    rect get_window_geometry() const {return window_geometry;}
    bool is_minimized() const {return b_is_minimized;}

private:

    Window window;
    Window frame;
    Window titlebar;

    rect frame_geometry;
    rect titlebar_geometry;
    rect window_geometry;
    rect pre_minimize_and_maximize_saved_geometry;
    rect temp_move_and_resize_geometry;

    bool b_is_maximized;
    bool b_is_minimized;
    bool b_is_currently_moving_or_resizing;

    GC graphics_context_internal;

    std::shared_ptr<class Button> minimize_button;
    std::shared_ptr<class Button> maximize_button;
    std::shared_ptr<class Button> close_button;
};