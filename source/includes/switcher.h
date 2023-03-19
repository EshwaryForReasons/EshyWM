
#pragma once

#include "util.h"
#include "menu_base.h"

#include <Imlib2.h>
#include <memory>
#include <vector>

class EshyWMSwitcher : public EshyWMMenuBase
{
public:

    EshyWMSwitcher(rect _menu_geometry, Color _menu_color);
    virtual void show() override;
    virtual void raise(bool b_set_focus = false) override;
    void update_button_positions();
    void button_clicked(int x_root, int y_root);

    void add_window_option(std::shared_ptr<class EshyWMWindow> associated_window, const Imlib_Image& icon);
    void remove_window_option(std::shared_ptr<class EshyWMWindow> associated_window);
    void next_option();
    void confirm_choice();

    const std::vector<window_button_pair> get_switcher_window_options() const {return switcher_window_options;}

private:

    std::vector<window_button_pair> switcher_window_options;
    int selected_option;

    void select_option(int i);
    void handle_option_chosen(const int i);
};