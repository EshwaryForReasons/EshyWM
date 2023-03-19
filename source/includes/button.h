
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

    Button(const Drawable _drawable, const GC _drawable_graphics_context, const rect& _button_geometry, const button_color_data& _button_color = {0})
        : drawable(_drawable)
        , drawable_graphics_context(_drawable_graphics_context)
        , button_geometry(_button_geometry)
        , button_color(_button_color)
        , b_hovered(false)
    {}

    virtual void draw();
    virtual void set_position(int x, int y);
    virtual void set_size(uint width, uint height);
    virtual void set_hovered(bool b_new_hovered) {b_hovered = b_new_hovered;}

    const bool check_hovered(int cursor_x, int cursor_y) const;
    const bool is_hovered() const {return b_hovered;}

    const rect get_button_geometry() const {return button_geometry;}

protected:

    Button()
        : drawable(0)
        , drawable_graphics_context(nullptr)
        , button_geometry({0, 0, 20, 20})
        , button_color({0})
    {}

    const Drawable drawable;
    const GC drawable_graphics_context;

    bool b_hovered;
    rect button_geometry;
    button_color_data button_color;
};

class WindowButton : public Button
{
public:

    WindowButton(Window parent_window, const rect& _button_geometry, const button_color_data& _window_background_color);
    ~WindowButton();

    virtual void draw() override {}
    virtual void set_position(int x, int y) override;
    virtual void set_size(uint width, uint height) override;
    virtual void set_hovered(bool b_new_hovered) override;
    virtual void set_border_color(Color border_color);

    const Window get_window() const {return button_window;}

protected:

    WindowButton() {}

    Window button_window;
};

class ImageButton : public WindowButton
{
public:

    ImageButton(Window parent_window, const rect& _button_geometry, const button_color_data& _background_color, const Imlib_Image& _image)
        : WindowButton(parent_window, _button_geometry, _background_color)
        , button_image(_image)
    {}
    ImageButton(Window parent_window, const rect& _button_geometry, const button_color_data& _background_color, const char* _image_path)
        : ImageButton(parent_window, _button_geometry, _background_color, imlib_load_image(_image_path))
    {}
    ~ImageButton();

    virtual void draw() override;
    void set_image(const Imlib_Image& new_image);

protected:

    Imlib_Image button_image;
};