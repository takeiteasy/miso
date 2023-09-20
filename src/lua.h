//
//  lua.h
//  miso
//
//  Created by George Watson on 20/09/2023.
//

#ifndef mlua_h
#define mlua_h
#include "minilua.h"
#include "luacstruct.h"

void PrintStackAt(lua_State *L, int idx);
int LuaDumpTable(lua_State* L);
int LuaDumpStack(lua_State* L);

#endif /* mlua_h */
