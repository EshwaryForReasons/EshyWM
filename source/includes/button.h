
#pragma once

#include "util.h"

#include <functional>
#include <memory>
#include <Imlib2.h>

class EshyWMWindow;

struct button_color_data
{
    Color normal;
    Color hovered;
    Color pressed;
};

enum EButtonState : uint8_t
{
    S_NONE,
    S_Normal,
    S_Hovered,
    S_Pressed
};

struct button_clicked_data
{
    std::shared_ptr<EshyWMWindow> associated_window;
    void (*callback_function)(std::shared_ptr<EshyWMWindow> window, void*);
};

class Button
{
public:

    virtual void draw() {};
    virtual void set_position(int x, int y);
    virtual void set_size(uint width, uint height);

    const bool check_hovered(int cursor_x, int cursor_y) const;
    void set_button_state(EButtonState new_button_state) {button_state = new_button_state; on_update_state();}
    const EButtonState& get_button_state() const {return button_state;}
    const rect get_button_geometry() const {return button_geometry;}
    button_clicked_data& get_data() {return data;}

    virtual void on_update_state() {};

    void click();

protected:

    Button()
        : button_geometry({0, 0, 20, 20})
        , button_color({0})
        , data({nullptr, nullptr})
    {}

    EButtonState button_state;
    rect button_geometry;
    button_color_data button_color;
    button_clicked_data data;
};

class WindowButton : public Button
{
public:

    WindowButton(Window parent_window, const rect& _button_geometry, const button_color_data& _window_background_color);
    ~WindowButton();

    virtual void draw() override {}
    virtual void set_position(int x, int y) override;
    virtual void set_size(uint width, uint height) override;
    virtual void set_border_color(Color border_color);
    virtual void on_update_state() override;

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