//
//  lua.c
//  caverns_bellow
//
//  Created by George Watson on 28/11/2019.
//  Copyright © 2019 George Watson. All rights reserved.
//

#include "lua.h"
#include "debugbreak.h"

void lua_set_path(lua_State* L, const char* path) {
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");
  const char* cur_path = lua_tostring(L, -1);
  size_t tmp_sz = strlen(cur_path) + strlen(path) + 8;
  char tmp[tmp_sz];
  snprintf(tmp, tmp_sz, "%s;%s?.lua", cur_path, path);
  lua_pop(L, 1);
  lua_pushstring(L, tmp);
  lua_setfield(L, -2, "path");
  lua_pop(L, 1);
}

void lua_dump_table(lua_State* L, int table_idx) {
  printf("--------------- LUA TABLE DUMP ---------------\n");
  lua_pushvalue(L, table_idx);
  lua_pushnil(L);
  int t, j = (table_idx < 0 ? -table_idx : table_idx), i = table_idx - 1;
  const char *key = NULL, *tmp = NULL;
  while ((t = lua_next(L, i))) {
    lua_pushvalue(L, table_idx - 1);
    key = lua_tostring(L, table_idx);
    switch (lua_type(L, table_idx - 1)) {
      case LUA_TSTRING:
        printf("%s (string, %d) => `%s'\n", key, j, lua_tostring(L, i));
        break;
      case LUA_TBOOLEAN:
        printf("%s (boolean, %d) => %s\n", key, j, lua_toboolean(L, i) ? "true" : "false");
        break;
      case LUA_TNUMBER:
        printf("%s (integer, %d) => %g\n", key, j, lua_tonumber(L, i));
        break;
      default:
        tmp = lua_typename(L, i);
        printf("%s (%s, %d) => %s\n", key, tmp, j, tmp);
        if (!strncmp(lua_typename(L, t), "table", 5))
          lua_dump_table(L, i);
        break;
    }
    lua_pop(L, 2);
  }
  lua_pop(L, 1);
  printf("--------------- END TABLE DUMP ---------------\n");
}

void lua_dump_stack(lua_State* L) {
  int t, i = lua_gettop(L);
  const char* tmp = NULL;
  printf("--------------- LUA STACK DUMP ---------------\n");
  for (; i; --i) {
    
    switch ((t = lua_type(L, i))) {
      case LUA_TSTRING:
        printf("%d (string): `%s'\n", i, lua_tostring(L, i));
        break;
      case LUA_TBOOLEAN:
        printf("%d (boolean): %s\n", i, lua_toboolean(L, i) ? "true" : "false");
        break;
      case LUA_TNUMBER:
        printf("%d (integer): %g\n",  i, lua_tonumber(L, i));
        break;
      default:
        tmp = lua_typename(L, t);
        printf("%d (%s): %s\n", i, lua_typename(L, t), tmp);
        break;
    }
  }
  printf("--------------- END STACK DUMP ---------------\n");
}

static void(*__error_callback)(const char*, const char*, const char*, const char*, int) = NULL;

void lua_error_handle(lua_State* L, const char* file, const char* func, int line, char* msg, ...) {
  va_list args;
  va_start(args, msg);
  static char error[1024];
  vsprintf((char*)error, msg, args);
  va_end(args);
  
  lua_dump_stack(L);
  if (__error_callback)
    __error_callback(lua_tostring(L, -1), error, file, func, line);
}

void lua_error_callback(void(*cb)(const char* lua_msg, const char* msg, const char* file, const char* func, int line)) {
  __error_callback = cb;
}

int lua_debug_break(lua_State* L) {
  printf("--------------- LUA BREAKPOINT ---------------\n");
  lua_dump_stack(L);
  debug_break();
  return 0;
}

int lua_print_stack(lua_State* L) {
  lua_dump_stack(L);
  return 0;
}
