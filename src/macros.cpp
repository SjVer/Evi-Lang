#include "preprocessor.hpp"

// =====================================================

#define MACRO_GETTER(name) string macro_getter_##name(Preprocessor* _this)

MACRO_GETTER(line)
{
	// just the current line number
	return to_string(_this->_current_line_no);
}

// =====================================================

void Preprocessor::initialize_builtin_macros()
{
	#define ADD_DYNAMIC_MACRO(name, getter) _macros->insert(pair<string, MacroProperties>(\
		name, {true, "", &macro_getter_##getter}))
	#define ADD_STATIC_MACRO(name, format) _macros->insert(pair<string, MacroProperties>(\
		name, {false, format, nullptr}))

	// compiler information
	ADD_STATIC_MACRO("__COMPILER_NAME__",			"\"" APP_NAME " (official)\"");
	ADD_STATIC_MACRO("__COMPILER_VERSION__",		"\"" APP_VERSION "\"");
	ADD_STATIC_MACRO("__COMPILER_BUILD_TARGET__",	"\"" TARGET "\"");
	ADD_STATIC_MACRO("__COMPILER_BUILD_DATE__",		"\"" __DATE__ "\"");
	ADD_STATIC_MACRO("__COMPILER_BUILD_TIME__",		"\"" __TIME__ "\"");
	ADD_STATIC_MACRO("__OPERATING_SYSTEM__",		"\"" OS_NAME "\"");

	// basic macros
	ADD_DYNAMIC_MACRO("__LINE__", line);
}
