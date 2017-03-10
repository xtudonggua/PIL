
#include <stdio.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
lua_State *L;

// 打印栈信息
static void stack_dump(lua_State *L) {	
	int n = lua_gettop(L);
	for (int i = n; i >= 1; i--) {
		int t = lua_type(L, i);
		switch (t) {
			case LUA_TSTRING:
				printf("'%s'", lua_tostring(L, i));
				break;
			case LUA_TBOOLEAN:
				printf(lua_toboolean(L, i) ? "true" : "false");
				break;
			case LUA_TNUMBER:
				printf("%g", lua_tonumber(L, i));
				break;
			default:
				printf("%s", lua_typename(L, t));
				break;
		}
		printf(" ");
	}
	printf("\n");
}

// 测试函数接口
int test_function(int x, int y) {
	int sum;
	lua_getglobal(L, "add");	// 获取全局lua函数
	lua_pushnumber(L, x);		// push参数
	lua_pushnumber(L, y);
	lua_call(L, 2, 3);	// 执行参数, 2:参数个数，3:返回值个数
	sum = (int)lua_tonumber(L, -1);	//获取返回结果
	int a = lua_tonumber(L, -2);
	int b = lua_tonumber(L, -3);
	lua_pop(L, 3);	// 
	printf("a, b = %d, %d\n", a,	b);
	printf("The sum is %d\n", sum);
}

// 测试table
int test_table() {
	lua_getglobal(L, "t");
	if (!lua_istable(L, -1))
		luaL_error(L, "t is not table!!!");

	int result = 0;
	lua_pushstring(L, "k1");	// "k1"入栈，table位于-2
	lua_gettable(L, -2);
	result = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);	// 移除掉"k1"
	printf("k1 is %d\n", result);

	lua_pushstring(L, "k2");
	lua_gettable(L, -2);
	result = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);
	printf("k2 is %d\n", result);
	
	lua_getfield(L, -1, "k1");	// 等同于 lua_pushstring(L, "k1") lua_gettable(L, -2);
	result = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);
	printf("k1 is %d\n", result);
	lua_getfield(L, -1, "k2");
	result = (int)lua_tonumber(L, -1);
	printf("k2 is %d\n", result);
	lua_pop(L, 1);
	lua_pop(L, 1);	// 移除table
}

// 遍历table方法1
int test_iter1() {
	lua_getglobal(L, "t1");
	if (!lua_istable(L, -1))
		luaL_error(L, "t is not table!!!");
	int size = lua_rawlen(L, -1);	// 获取table长度
	printf("table size = %d\n", size);
	for(int i = 1; i <= size; i++) {
		lua_pushnumber(L, i);
		lua_gettable(L, -2);
		int value = (int)lua_tonumber(L, -1);
		printf("key, value = %d, %d\n", i, value);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

// 遍历table方法2
int test_iter2() {
	lua_getglobal(L, "t");
	if (!lua_istable(L, -1))
		luaL_error(L, "t is not table!!!");
	lua_pushnil(L);
	while(lua_next(L, -2) != 0) {
		const char *t_key = lua_typename(L, lua_type(L, -2));
		const char *t_value = lua_typename(L, lua_type(L, -1));
		printf("%s-%s\n", t_key, t_value);
		lua_pop(L, 1);
	}
}

int main(void) {
	L = luaL_newstate();
	// luaL_openlibs(L);
	luaL_dofile(L, "../lua/add.lua");
	test_function(10, 15);
	test_table();
	test_iter1();
	test_iter2();
	
	lua_close(L);
	return 0;
}