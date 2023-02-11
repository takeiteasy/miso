//
//  lua_wrapper.h
//  colony
//
//  Created by George Watson on 09/02/2023.
//

#ifndef lua_wrapper_h
#define lua_wrapper_h
#include "minilua.h"
#include <string.h>

void SetLuaPath(lua_State* L, const char* path);
void LuaDumpTable(lua_State* L, int table_idx);
void LuaDumpStack(lua_State* L);
int LuaDebugBreak(lua_State* L);

#endif /* lua_wrapper_h */
