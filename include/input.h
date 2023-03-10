//
//  input.h
//  colony
//
//  Created by George Watson on 10/03/2023.
//

#ifndef input_h
#define input_h
#include "sokol_app.h"
#include "maths.h"

bool IsKeyDown(sapp_keycode key);
bool IsKeyUp(sapp_keycode key);
bool WasKeyPressed(sapp_keycode key);
bool IsButtonDown(sapp_mousebutton button);
bool IsButtonUp(sapp_mousebutton button);
bool WasButtonPressed(sapp_mousebutton button);
bool WasMouseScrolled(void);
bool WasMouseMoved(void);
Vec2 MousePosition(void);
Vec2 LastMousePosition(void);
Vec2 MouseScrollDelta(void);
Vec2 MouseMoveDelta(void);

void InputHandler(const sapp_event* e);
void ResetInput(void);

#endif /* input_h */
