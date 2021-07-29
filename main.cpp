#include "GarrysMod/Lua/Interface.h"
#include <duktape.h>
#include <main.h>

#include <string>
#include <filesystem>

#include <cpp-httplib/httplib.h>
#include <PicoSHA2/picosha2.h>


using namespace GarrysMod::Lua;

ILuaBase* mainLua;
duk_context* mainCtx;

static void luaPrint(const char* string) {
	if (mainLua) {
		mainLua->PushSpecial(SPECIAL_GLOB);
		mainLua->GetField(-1, "print");
		mainLua->PushString(string);
		mainLua->Call(1, 0);
		mainLua->Pop(1); // Pop off the global environment
	}
}
duk_ret_t native_print(duk_context* ctx) {
	const char* wantedString = duk_to_string(ctx, 0);
	luaPrint(wantedString);
		
	return 0;
}

static void setupDuktape() {
	duk_push_c_function(mainCtx, native_print, 1);
	duk_put_global_string(mainCtx, "print");
}
// Handles fetching the preload code
static void handlePreloadJS() {
	// try getting the code over the internet

	httplib::Client codeClient("")

	if (std::filesystem::exists(PRELOAD_FILE_NAME)) {
		// ok, it exists so we can possibly fetch,
		// but we still need to compare the hashes

	}
}

// Called when the module is loaded
#define ADD_LUA_FUNC(luafuncname, nameInTable) LUA->PushCFunction(luafuncname); LUA->SetField(-2, nameInTable);

LUA_FUNCTION(EvaluateJS) {
	LUA->CheckType(-1, Type::String); // Source Code

	const char* sourceCode = LUA->GetString(-1);

	if (duk_peval_string(mainCtx, sourceCode) != 0) {
		// Something went wrong..
		const char* evalMsg = duk_safe_to_string(mainCtx, -1);
		std::string errorMsg = "Error while running: " + std::string(evalMsg);

		LUA->ThrowError(errorMsg.c_str());

		return 0;
	}

	return 0;
}

GMOD_MODULE_OPEN()
{
	mainCtx = duk_create_heap_default();
	mainLua = LUA;

	if (mainCtx != nullptr) {
		setupDuktape();

		LUA->PushSpecial(SPECIAL_GLOB);
		LUA->CreateTable();
		
		ADD_LUA_FUNC(EvaluateJS, "Evaluate");

		LUA->SetField(-2, "javascript"); // Sets the table in the _G environment

		LUA->Pop(); // Remove the global environment
	}

	return 0;
}

// Called when the module is unloaded
GMOD_MODULE_CLOSE()
{
	mainLua = nullptr;

	if (mainCtx != nullptr) {
		duk_destroy_heap(mainCtx);

		LUA->PushSpecial(SPECIAL_GLOB);
		LUA->PushNil();
		LUA->SetField(-2, "javascript"); // Remove the javascript table

		LUA->Pop();
	}

	return 0;
}