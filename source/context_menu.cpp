
#include "context_menu.h"
#include "eshywm.h"
#include "window_manager.h"
#include "button.h"

EshyWMContextMenu::EshyWMContextMenu(rect _menu_geometry, Color _menu_color) : EshyWMMenuBase(_menu_geometry, _menu_color)
{
    XGrabButton(DISPLAY, Button1, AnyModifier, menu_window, false, ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, Button3, AnyModifier, ROOT, false, ButtonPressMask, GrabModeSync, GrabModeAsync, ROOT, None);
}

void EshyWMContextMenu::draw()
{
    int i = 0;
    for(std::shared_ptr<Button> button : context_menu_buttons)
    {
        button->set_position(10, (EshyWMConfig::context_menu_button_height * i) + 10);
        button->draw();
        i++;
    }
}
