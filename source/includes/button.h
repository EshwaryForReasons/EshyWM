
#pragma once

#include "util.h"

#include <Imlib2.h>

struct button_color_data
{
    Color normal;
    Color hovered;
    Color pressed;
};

class Button
{
public:

    Button(const Drawable _drawable, const GC _drawable_graphics_context, rect _button_size_and_location, button_color_data _button_color = {0});

    virtual void draw();
    virtual void set_position(int x, int y);
    virtual void set_size(uint width, uint height);

    const bool is_hovered(int cursor_x, int cursor_y) const;

protected:

    Button()
        : drawable(0)
        , drawable_graphics_context(nullptr)
        , button_geometry({0, 0, 20, 20})
        , button_color({0})
    {}

    const Drawable drawable;
    const GC drawable_graphics_context;

    rect button_geometry;
    button_color_data button_color;
    Color current_button_color;
};

class ImageButton : public Button
{
public:

    ImageButton(Window parent_window, rect _button_geometry, const char* _image_path);
    ImageButton(Window parent_window, rect _button_geometry, Imlib_Image _image);
    ~ImageButton();

    virtual void draw() override;
    virtual void set_position(int x, int y) override;
    virtual void set_size(uint width, uint height) override;

protected:

    Imlib_Image button_image;
    Window button_window;
};