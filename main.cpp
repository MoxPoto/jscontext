#include "GarrysMod/Lua/Interface.h"
#include <duktape.h>
#include <main.h>

#include <string>
#include <filesystem>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <cpp-httplib/httplib.h>
#include <PicoSHA2/picosha2.h>
#include <iostream>

using namespace GarrysMod::Lua;

ILuaBase* mainLua;
duk_context* mainCtx;
std::string preloadJS;

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
static bool handlePreloadJS() {
	// try getting the code over the internet

	httplib::Client codeClient("https://raw.githubusercontent.com");
	luaPrint("HTTP client setup..");

	httplib::Result codeResult = codeClient.Get(PRELOAD_URL_PATH.c_str());

	luaPrint("Requesting internet code..");

	bool internetCode = (codeResult->status == 200);

	if (std::filesystem::exists(PRELOAD_FILE_NAME)) {
		// ok, it exists so we can possibly fetch,
		// but we still need to compare the hashes

		std::ifstream codeStream(PRELOAD_FILE_NAME, std::ios::in);

		if (codeStream.is_open()) {
			// open, let's read

			std::stringstream buffer;
			buffer << codeStream.rdbuf();

			preloadJS = buffer.str();
				
			if (!internetCode) {
				codeStream.close();

				return true; // we cant compare to the online code, so just end
			}
		}
		else {
			luaPrint("Couldn't read the preload file??");
			codeStream.close();

			return false;
		}

		// this if statement is 99% redundant
		if (internetCode) {
			// there is internet code and local code, let's hash and compare

			std::string onlineCode = codeResult->body;
			std::string hashedLocal;
			std::string hashedOnline;

			picosha2::hash256_hex_string(onlineCode, hashedOnline);
			picosha2::hash256_hex_string(preloadJS, hashedLocal);

			if (hashedLocal != hashedOnline) {
				luaPrint("Detected changes from online preload code and local code, downloading new code..");
				luaPrint("Dumping new code:");
				luaPrint(onlineCode.c_str());

				preloadJS = onlineCode;

				// add new code to file

				std::ofstream newCodeFile(PRELOAD_FILE_NAME, std::ofstream::trunc);

				if (newCodeFile.is_open()) {
					newCodeFile << onlineCode;
					newCodeFile.close();
				}
				else {
					luaPrint("Couldn't update local file with new code..");
					newCodeFile.close();
					return false;
				}
				
				return true;
			}
			
			return true;
		}
	}
	else {
		// no local file

		if (!internetCode) {
			// no online code to go off of.. damn this is just unlucky

			luaPrint("Can't download preload code, can't fetch local file, aborting..");
			return false;
		}

		// ok so we have online code, lets write that then
		std::string onlineCode = codeResult->body;
		preloadJS = onlineCode;

		std::ofstream newCodeFile(PRELOAD_FILE_NAME, std::ofstream::trunc);

		if (newCodeFile.is_open()) {
			newCodeFile << onlineCode;
			newCodeFile.close();
		}
		else {
			luaPrint("Couldn't create local file with new code..");
			newCodeFile.close();
			return false;
		}

		return true;
	}

	// just if somehow all of the code above is skipped
	return false;
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

		luaPrint("Setting up preload JS code..");

		bool didWork = handlePreloadJS();

		if (!didWork) {
			duk_destroy_heap(mainCtx);
			mainCtx = nullptr;

			luaPrint("Couldn't fetch preload code, not finishing setup..");

			return 0;
		}
		else {
			luaPrint("Got preload code, loading..");

			if (duk_peval_string(mainCtx, preloadJS.c_str()) != 0) {
				// Something went wrong..
				const char* evalMsg = duk_safe_to_string(mainCtx, -1);
				std::string errorMsg = "FATAL ERROR (PRELOAD): " + std::string(evalMsg);

				duk_destroy_heap(mainCtx);
				LUA->ThrowError(errorMsg.c_str());

				return 0;
			}
		}

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