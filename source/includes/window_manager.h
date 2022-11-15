
#pragma once

#include <Imlib2.h>
#include <memory>
#include <mutex>
#include <string>
#include <map>

#include "util.h"
#include "container.h"

enum class EWindowTileMode
{
    WTM_None,
    WTM_Equal,          //Space everything equally
    WTM_Adjustive,      //Changing size only affects focused window, rest only compensate (make space, claim space)
    WTM_Seperate        //Every window is its own entity and do not effect each other
};

struct window_manager_data
{
    bool b_tiling_mode;
    EWindowTileMode window_tile_mode;

    window_manager_data()
        : b_tiling_mode(false)
        , window_tile_mode(EWindowTileMode::WTM_Equal)
    {}
};

struct double_click_data
{
    Window first_click_window;
    Time first_click_time;
};

class WindowManager
{
public:

    static std::shared_ptr<WindowManager> Create(const std::string& display_str = std::string());

    WindowManager();
    ~WindowManager();
    
    void initialize();
    void main_loop();
    void handle_preexisting_windows();

    void close_window(std::shared_ptr<class EshyWMWindow> closed_window);

    /**Getters*/
    static Display* get_display() {return display;}
    const Window get_root() {return root;}
    window_manager_data* get_manager_data() const {return manager_data;}
    uint get_display_width() const {return display_width;}
    uint get_display_height() const {return display_height;}

private:

    static Display* display;
    const Window root;
    static std::mutex mutex_wm_detected;

    Vector2D<int> click_cursor_position;
    std::shared_ptr<class container> root_container;
    window_manager_data* manager_data;

    uint display_width;
    uint display_height;

    organized_container_map_t frame_list;
    organized_container_map_t titlebar_list;

    double_click_data titlebar_double_click;

    /**Event handlers*/
    void OnUnmapNotify(const XUnmapEvent& event);
    void OnConfigureNotify(const XConfigureEvent& event);
    void OnMapRequest(const XMapRequestEvent& event);
    void OnConfigureRequest(const XConfigureRequestEvent& event);
    void OnVisibilityNotify(const XVisibilityEvent& event);
    void OnButtonPress(const XButtonEvent& event);
    void OnButtonRelease(const XButtonEvent& event);
    void OnMotionNotify(const XMotionEvent& event);
    void OnKeyPress(const XKeyEvent& event);
    void OnKeyRelease(const XKeyEvent& event);

    /**Xlib error handler*/
    static int OnXError(Display* display, XErrorEvent* e);
    static int OnWMDetected(Display* display, XErrorEvent* e);
    static bool b_wm_detected;

    /**Helper functions*/
    std::shared_ptr<class EshyWMWindow> register_window(Window window, bool b_was_created_before_window_manager);
    void rescale_windows(uint old_width, uint old_height);
    void check_titlebar_button_pressed(Window window, int cursor_x, int cursor_y);
};