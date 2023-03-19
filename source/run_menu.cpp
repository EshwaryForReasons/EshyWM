
#include "run_menu.h"
#include "eshywm.h"

#include <X11/Xutil.h>

EshyWMRunMenu::EshyWMRunMenu(rect _menu_geometry, Color _menu_color) : EshyWMMenuBase(_menu_geometry, _menu_color)
{
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_R), Mod4Mask, ROOT, false, GrabModeAsync, GrabModeAsync);

    XGrabButton(DISPLAY, Button1, AnyModifier, menu_window, false, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Up), AnyModifier, menu_window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Down), AnyModifier, menu_window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_KP_Enter), AnyModifier, menu_window, false, GrabModeAsync, GrabModeAsync);
}