#pragma once

#include "util.h"
#include "menu_base.h"

#include <vector>
#include <memory>

class EshyWMContextMenu : public EshyWMMenuBase
{
public:

    EshyWMContextMenu(rect _menu_geometry, Color _menu_color);
    virtual void draw() override;

private:

    std::vector<std::shared_ptr<class Button>> context_menu_buttons;
};