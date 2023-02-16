//
//  input.c
//  colony
//
//  Created by George Watson on 15/02/2023.
//

#include "input.h"

static struct {
    bool button_down[SAPP_MAX_KEYCODES];
    bool button_clicked[SAPP_MAX_KEYCODES];
    bool mouse_down[SAPP_MAX_MOUSEBUTTONS];
    bool mouse_clicked[SAPP_MAX_MOUSEBUTTONS];
    Vec2 mouse_pos, last_mouse_pos;
    Vec2 mouse_scroll_delta, mouse_delta;
} Input;

void InputHandler(const sapp_event *e) {
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
#if defined(DEBUG)
            if (e->modifiers & SAPP_MODIFIER_SUPER && e->key_code == SAPP_KEYCODE_W)
                sapp_quit();
#endif
            Input.button_down[e->key_code] = true;
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            Input.button_down[e->key_code] = false;
            Input.button_clicked[e->key_code] = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            Input.mouse_down[e->mouse_button] = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            Input.mouse_down[e->mouse_button] = false;
            Input.mouse_clicked[e->mouse_button] = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            Input.last_mouse_pos = Input.mouse_pos;
            Input.mouse_pos = (Vec2){e->mouse_x, e->mouse_y};
            Input.mouse_delta = (Vec2){e->mouse_dx, e->mouse_dy};
            break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            Input.mouse_scroll_delta = (Vec2){e->scroll_x, e->scroll_y};
            break;
        default:
            break;
    }
}

void ResetInputHandler(void) {
    Input.mouse_delta = Input.mouse_scroll_delta = (Vec2){0};
    for (int i = 0; i < SAPP_MAX_KEYCODES; i++)
        if (Input.button_clicked[i])
            Input.button_clicked[i] = false;
    for (int i = 0; i < SAPP_MAX_MOUSEBUTTONS; i++)
        if (Input.mouse_clicked[i])
            Input.mouse_clicked[i] = false;
}

bool IsKeyDown(sapp_keycode key) {
    return Input.button_down[key];
}

bool IsKeyUp(sapp_keycode key) {
    return !Input.button_down[key];
}

bool WasKeyClicked(sapp_keycode key) {
    return Input.button_clicked[key];
}

bool IsButtonDown(sapp_mousebutton button) {
    return Input.mouse_down[button];
}

bool IsButtonUp(sapp_mousebutton button) {
    return !Input.mouse_down[button];
}

bool WasButtonPressed(sapp_mousebutton button) {
    return Input.mouse_clicked[button];
}