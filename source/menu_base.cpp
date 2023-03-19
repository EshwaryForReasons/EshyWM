
#include "menu_base.h"
#include "eshywm.h"
#include "window_manager.h"

#include <X11/Xutil.h>

EshyWMMenuBase::EshyWMMenuBase(rect _menu_geometry, Color _menu_color) : menu_geometry(_menu_geometry), menu_color(_menu_color)
{
    menu_window = XCreateSimpleWindow(DISPLAY, ROOT, menu_geometry.x, menu_geometry.y, menu_geometry.width, menu_geometry.height, 0, 0, menu_color);
    XSelectInput(DISPLAY, menu_window, SubstructureRedirectMask | SubstructureNotifyMask | VisibilityChangeMask);
    
    graphics_context_internal = XCreateGC(DISPLAY, menu_window, 0, 0);
}

void EshyWMMenuBase::show()
{
    XMapWindow(DISPLAY, menu_window);
    raise();
    b_menu_active = true;
}

void EshyWMMenuBase::remove()
{
    XUnmapWindow(DISPLAY, menu_window);
    b_menu_active = false;
}

void EshyWMMenuBase::raise(bool b_set_focus)
{
    XRaiseWindow(DISPLAY, menu_window);

    if(b_set_focus)
    {
        XSetInputFocus(DISPLAY, menu_window, RevertToPointerRoot, CurrentTime);
    }
}

void EshyWMMenuBase::draw()
{

}

void EshyWMMenuBase::set_position(int x, int y)
{
    menu_geometry.x = x;
    menu_geometry.y = y;
    XMoveWindow(DISPLAY, menu_window, x, y);
}

void EshyWMMenuBase::set_size(uint width, uint height)
{
    menu_geometry.width = width;
    menu_geometry.height = height;
    XResizeWindow(DISPLAY, menu_window, width, height);
}
