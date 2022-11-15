
#include "button.h"
#include "eshywm.h"
#include "window_manager.h"
#include "config.h"

#include <Imlib2.h>
#include <X11/Xutil.h>

Button::Button(const Drawable _drawable, const GC _drawable_graphics_context, rect _button_geometry, button_color_data _button_color)
    : drawable(_drawable)
    , drawable_graphics_context(_drawable_graphics_context)
    , button_geometry(_button_geometry)
    , button_color(_button_color)
{
    current_button_color = button_color.normal;
}

void Button::draw()
{
    XSetForeground(DISPLAY, drawable_graphics_context, current_button_color);
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

const bool Button::is_hovered(int cursor_x, int cursor_y) const
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


ImageButton::ImageButton(Window parent_window, rect _button_geometry, const char* _image_path)
{
    button_geometry = _button_geometry;
    button_color = {0};

    button_image = imlib_load_image(_image_path);
    
    button_window = XCreateSimpleWindow(DISPLAY, parent_window, 0, 0, button_geometry.width, button_geometry.height, 0, 0, 0);
    XSelectInput(DISPLAY, button_window, StructureNotifyMask);
    XMapWindow(DISPLAY, button_window);
    XSync(DISPLAY, false);
}

ImageButton::ImageButton(Window parent_window, rect _button_geometry, Imlib_Image _image)
{
    button_geometry = _button_geometry;
    button_color = {0};

    button_image = _image;

    button_window = XCreateSimpleWindow(DISPLAY, parent_window, 0, 0, button_geometry.width, button_geometry.height, 0, 0, 0xff0000);
    XSelectInput(DISPLAY, button_window, StructureNotifyMask);
    XMapWindow(DISPLAY, button_window);
    XSync(DISPLAY, false);

    /**
    XVisualInfo vinfo;
    XMatchVisualInfo(DISPLAY, DefaultScreen(DISPLAY), 32, TrueColor, &vinfo);
    
    XSetWindowAttributes attr;
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(DISPLAY, ROOT, vinfo.visual, AllocNone);

    button_window = XCreateWindow(
        DISPLAY,
        parent_window,
        0,
        0,
        button_geometry.width,
        button_geometry.height,
        0,
        32,
        InputOutput,
        vinfo.visual,
        CWBackPixmap | CWBackPixel | CWBorderPixel,
        &attr
    );
    XSelectInput(DISPLAY, button_window, StructureNotifyMask);
    XMapWindow(DISPLAY, button_window);
    XSync(DISPLAY, false);
    */
}

ImageButton::~ImageButton()
{
    XUnmapWindow(DISPLAY, button_window);
    
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

void ImageButton::set_position(int x, int y)
{
    button_geometry.x = x;
    button_geometry.y = y;
    XMoveWindow(DISPLAY, button_window, x, y);
}

void ImageButton::set_size(uint width, uint height)
{
    button_geometry.width = width;
    button_geometry.height = height;
    XResizeWindow(DISPLAY, button_window, width, height);
}