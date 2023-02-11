//
//  lua_wrapper.c
//  colony
//
//  Created by George Watson on 09/02/2023.
//

#include "lua.h"
#include "debugbreak.h"

void SetLuaPath(lua_State* L, const char* path) {
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

void LuaDumpTable(lua_State* L, int table_idx) {
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
            LuaDumpTable(L, i);
        break;
    }
    lua_pop(L, 2);
  }
  lua_pop(L, 1);
  printf("--------------- END TABLE DUMP ---------------\n");
}

void LuaDumpStack(lua_State* L) {
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

int lua_debug_break(lua_State* L) {
  printf("--------------- LUA BREAKPOINT ---------------\n");
  LuaDumpStack(L);
  debug_break();
  return 0;
}
