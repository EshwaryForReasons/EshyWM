
#pragma once

#include "util.h"
#include "menu_base.h"
#include "button.h"

#include <memory>
#include <vector>

struct window_button_pair
{
    std::shared_ptr<class EshyWMWindow> window;
    std::shared_ptr<Button> button;
};

class EshyWMTaskbar : public EshyWMMenuBase
{
public:

    EshyWMTaskbar(rect _menu_geometry, Color _menu_color);
    virtual void draw() override;

    void update_taskbar_size(uint width, uint height);

    void add_button(std::shared_ptr<class EshyWMWindow> associated_window);
    void remove_button(std::shared_ptr<class EshyWMWindow> associated_window);

    void check_taskbar_button_clicked(int cursor_x, int cursor_y);

private:

    std::vector<window_button_pair> taskbar_buttons;

    Window win;
};