
#include "button.h"
#include "eshywm.h"
#include "window_manager.h"
#include "config.h"

#include <Imlib2.h>
#include <X11/Xutil.h>

void Button::draw()
{
    XSetForeground(DISPLAY, drawable_graphics_context, b_hovered ? button_color.normal : button_color.normal);
    XFillRectangle(DISPLAY, drawable, drawable_graphics_context, button_geometry.x, button_geometry.y, button_geometry.width, button_geometry.height);
}

void Button::set_position(int x, int y)
{
    button_geometry.x = x;
    button_geometry.y = y;
}

void Button::set_size(uint width, uint height)
{
    button_geometry.width = width;
    button_geometry.height = height;
}

const bool Button::check_hovered(int cursor_x, int cursor_y) const
{
    if(cursor_x > button_geometry.x
    && cursor_x < button_geometry.x + button_geometry.width
    && cursor_y > button_geometry.y
    && cursor_y < button_geometry.y + button_geometry.height)
    {
        return true;
    }

    return false;
}


WindowButton::WindowButton(Window parent_window, const rect& _button_geometry, const button_color_data& _background_color)
{
    button_geometry = _button_geometry;
    button_color = _background_color;

    button_window = XCreateSimpleWindow(DISPLAY, parent_window, 0, 0, button_geometry.width, button_geometry.height, 0, 0, _background_color.normal);
    XSelectInput(DISPLAY, button_window, StructureNotifyMask | VisibilityChangeMask | EnterWindowMask | LeaveWindowMask);
    XMapWindow(DISPLAY, button_window);
    XSync(DISPLAY, false);
}

WindowButton::~WindowButton()
{
    XUnmapWindow(DISPLAY, button_window);
    XDestroyWindow(DISPLAY, button_window);
}

void WindowButton::set_border_color(Color border_color)
{
    XSetWindowBorder(DISPLAY, button_window, border_color);
    XClearWindow(DISPLAY, button_window);
    draw();
}

void WindowButton::set_position(int x, int y)
{
    button_geometry.x = x;
    button_geometry.y = y;
    XMoveWindow(DISPLAY, button_window, x, y);
}

void WindowButton::set_size(uint width, uint height)
{
    button_geometry.width = width;
    button_geometry.height = height;
    XResizeWindow(DISPLAY, button_window, width, height);
}

void WindowButton::set_hovered(bool b_new_hovered)
{
    if(b_new_hovered != b_hovered)
    {
        Button::set_hovered(b_new_hovered);
        XSetWindowBackground(DISPLAY, button_window, b_hovered ? button_color.hovered : button_color.normal);
        XClearWindow(DISPLAY, button_window);
        draw();
    }
}


ImageButton::~ImageButton()
{
    imlib_context_set_image(button_image);
    imlib_free_image();
}

void ImageButton::draw()
{
    imlib_context_set_drawable(button_window);
    imlib_context_set_image(button_image);
    imlib_image_set_has_alpha(1);
    imlib_render_image_on_drawable_at_size(0, 0, button_geometry.width, button_geometry.height);
}

void ImageButton::set_image(const Imlib_Image& new_image)
{
    button_image = new_image;
    draw();
}