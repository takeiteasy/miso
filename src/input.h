//
//  input.h
//  colony
//
//  Created by George Watson on 15/02/2023.
//

#ifndef input_h
#define input_h
#include "sokol_app.h"
#include "linalgb.h"

void InputHandler(const sapp_event *e);
void ResetInputHandler(void);

bool IsKeyDown(sapp_keycode key);
bool IsKeyUp(sapp_keycode key);
bool WasKeyClicked(sapp_keycode key);
bool IsButtonDown(sapp_mousebutton button);
bool IsButtonUp(sapp_mousebutton button);
bool WasButtonPressed(sapp_mousebutton button);

#endif /* input_h */
