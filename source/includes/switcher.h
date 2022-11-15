
#pragma once

#include "util.h"
#include "menu_base.h"

#include <memory>
#include <unordered_map>

class EshyWMSwitcher : public EshyWMMenuBase
{
public:

    EshyWMSwitcher(rect _menu_geometry, Color _menu_color);
    virtual void draw() override;

    void add_window_option(std::shared_ptr<class EshyWMWindow> associated_window);
    void next_option();
    void confirm_choice();
   
private:

    std::unordered_map<std::shared_ptr<class Button>, std::shared_ptr<class EshyWMWindow>> switcher_window_options;
};