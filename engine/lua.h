//
//  lua.h
//  caverns_bellow
//
//  Created by George Watson on 28/11/2019.
//  Copyright © 2019 George Watson. All rights reserved.
//

#ifndef lua_wrapper_h
#define lua_wrapper_h

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

void lua_set_path(lua_State* L, const char* path);
void lua_dump_table(lua_State* L, int table_idx);
void lua_dump_stack(lua_State* L);
#define LUA_ERROR(L, ...) lua_error_handle((L), __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
void lua_error_handle(lua_State* L, const char*, const char*, int, char* msg, ...);
void lua_error_callback(void(*cb)(const char*, const char*, const char*, const char*, int));

#define DEF_LUA_GET(T, LT, R) \
static T lua_get_##LT##_field(lua_State* L, const char* key, int table_idx) { \
  if (!lua_istable(L, table_idx)) \
    return R; \
  lua_pushstring(L, key); \
  if (!lua_istable(L, table_idx - 1)) \
    return R; \
  lua_gettable(L, table_idx - 1); \
  if (!lua_is##LT(L, table_idx)) \
    return R; \
  T ret = (T)lua_to##LT(L, table_idx); \
  lua_pop(L, 1); \
  return ret; \
}
DEF_LUA_GET(float, number, -1.f);
DEF_LUA_GET(int, integer, -1);
DEF_LUA_GET(const char*, string, NULL);
DEF_LUA_GET(bool, boolean, false);
#define LUA_PUSH_C_FN(name) \
  lua_pushcfunction(L, lua_##name); \
  lua_setglobal(L, #name)
#define LUA_TRY(X, ...) \
  if ((X)) \
    LUA_ERROR(L, __VA_ARGS__);
#define LUA_LOAD_FN(X) \
  lua_getfield(L, -1, (X)); \
  LUA_TRY(!lua_isfunction(L, -1), "lua_get_mod_func() failed: couldn't find function \"%s\" in module", (X)) \

int lua_debug_break(lua_State* L);
int lua_print_stack(lua_State* L);

#endif /* lua_h */
