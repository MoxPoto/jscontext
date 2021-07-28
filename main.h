#ifndef MAIN_H
#define MAIN_H

#include <duktape.h>
#include "GarrysMod/Lua/Interface.h"

using namespace GarrysMod::Lua;

extern ILuaBase* mainLua;
extern duk_context* mainCtx;
extern duk_ret_t native_print(duk_context* ctx);

#endif