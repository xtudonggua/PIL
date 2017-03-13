
#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

extern int add(lua_State* L)   //去掉了原代码中的extern "C"，后面也是类似去掉了
{
    double op1 = luaL_checknumber(L, 1);
    double op2 = luaL_checknumber(L, 2);
    lua_pushnumber(L, op1 + op2);
    return 1;
}

int sub(lua_State* L)
{
    double op1 = luaL_checknumber(L, 1);
    double op2 = luaL_checknumber(L, 2);
    lua_pushnumber(L, op1 - op2);
    return 1;
}
//luaL_Reg结构体的第一个字段为字符串，在注册时用于通知Lua该函数的名字。
//第一个字段为C函数指针。
//结构体数组中的最后一个元素的两个字段均为NULL，用于提示Lua注册函数已经到达数组的末尾。
static luaL_Reg mylibs[] ={
    {"add", add},
    {"sub", sub},
    {NULL, NULL}
};
//该C库的唯一入口函数。其函数签名等同于上面的注册函数。见如下几点说明：
//1. 我们可以将该函数简单的理解为模块的工厂函数。
//2. 其函数名必须为luaopen_xxx，其中xxx表示library名称。Lua代码require "xxx"需要与之对应。
//3. 在luaL_setfuncs的调用中，其第二个参数为待注册函数的数组。
//4. 需要强调的是，所有需要用到"xxx"的代码，不论C还是Lua，都必须保持一致，这是Lua的约定，
//   否则将无法调用。
int luaopen_mylib(lua_State* L)
{
    //const char* libName = "mylib"; //
    //luaL_register(L, libName, mylibs); //由于在lua-5.3中已没有luaL_register这个函数，所以换成下面两行代码
    lua_newtable(L);
    luaL_setfuncs(L, mylibs, 0); 

    return 1;
}