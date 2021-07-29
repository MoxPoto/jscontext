#ifndef MAIN_H
#define MAIN_H

#include <duktape.h>
#include "GarrysMod/Lua/Interface.h"

using namespace GarrysMod::Lua;

// This will contain javascript that implements some classes (such as vectors)

const std::string PRELOAD_FILE_NAME = "jscontext_preload.js";
extern std::string preloadJS;
extern ILuaBase* mainLua;
extern duk_context* mainCtx;
extern duk_ret_t native_print(duk_context* ctx);

#endif