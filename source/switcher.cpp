
#include "switcher.h"
#include "eshywm.h"
#include "button.h"

#include <X11/Xutil.h>

EshyWMSwitcher::EshyWMSwitcher(rect _menu_geometry, Color _menu_color) : EshyWMMenuBase(_menu_geometry, _menu_color)
{
    XGrabButton(DISPLAY, Button1, 0, menu_window, false, ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Up), AnyModifier, menu_window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Down), AnyModifier, menu_window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Alt_L), AnyModifier, menu_window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Tab), Mod4Mask, ROOT, false, GrabModeAsync, GrabModeAsync);
}

void EshyWMSwitcher::draw()
{
    int i = 0;
    for(auto const& [button, window] : switcher_window_options)
    {
        button->set_position(10, (CONFIG->switcher_button_height * i) + 10);
        button->draw();
        i++;
    }
}

void EshyWMSwitcher::next_option()
{
    std::cout << "next option" << std::endl;
}

void EshyWMSwitcher::add_window_option(std::shared_ptr<class EshyWMWindow> associated_window)
{
    const rect size = {0, 0, CONFIG->switcher_width - 20, CONFIG->switcher_button_height};
    std::shared_ptr<Button> button = std::make_shared<Button>(menu_window, graphics_context_internal, size);
    switcher_window_options.emplace(button, associated_window);
}

void EshyWMSwitcher::confirm_choice()
{
    remove();
}