
#pragma once

#include "util.h"

#include <memory>

/**Used in switcher and taskbar*/
struct window_button_pair
{
    std::shared_ptr<class EshyWMWindow> window;
    std::shared_ptr<class WindowButton> button;
};

class EshyWMMenuBase
{
public:

    EshyWMMenuBase(rect _menu_geometry, Color _menu_color);

    virtual void show();
    virtual void remove();
    virtual void raise(bool b_set_focus = false);
    virtual void draw();

    virtual void set_position(int x, int y);
    virtual void set_size(uint width, uint height);

    Window get_menu_window() const {return menu_window;}
    const bool get_menu_active() const {return b_menu_active;}

protected:

    Window menu_window;
    GC graphics_context_internal;
    rect menu_geometry;
    Color menu_color;
    bool b_menu_active;
};