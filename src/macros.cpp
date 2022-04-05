#include "macros.hpp"
#include <iomanip>

// =====================================================

static uint counter_value = 0;

// =====================================================

#define MACRO_GETTER(name) string macro_getter_##name()

MACRO_GETTER(line)
{
	return to_string(State::get_current_line_no());
}

MACRO_GETTER(file)
{
	return '"' + State::get_current_file() + '"';
}

MACRO_GETTER(time_)
{
	time_t t = time(nullptr);
	struct tm* tm = localtime(&t);

	// make sure that when 32 bytes isn't enough we try again and again
	size_t max_size = 32;
	size_t size = max_size;
	while(size == max_size)
	{
		size = strftime(nullptr, max_size, "%H:%M:%S", tm) + 1;
		max_size += 8;
	}

	// get the actual string
	char* date = (char*)malloc(size);
	strftime(date, size, "%H:%M:%S", tm);

	return '"' + string(date) + '"';
}

MACRO_GETTER(date)
{
	time_t t = time(nullptr);
	struct tm* tm = localtime(&t);

	// make sure that when 32 bytes isn't enough we try again and again
	size_t max_size = 32;
	size_t size = max_size;
	while(size == max_size)
	{
		size = strftime(nullptr, max_size, "%b %d %Y", tm) + 1;
		max_size += 8;
	}

	// get the actual string
	char* date = (char*)malloc(size);
	strftime(date, size, "%b %d %Y", tm);

	return '"' + string(date) + '"';
}

MACRO_GETTER(counter)
{
	return to_string(counter_value++);
}

MACRO_GETTER(apply_depth)
{
	return to_string(State::get_apply_depth());
}

// =====================================================

void initialize_state_singleton(Preprocessor* p)
{
	State::_p = p;
	State::_initialized = true;
}

void Preprocessor::initialize_builtin_macros()
{
	#define ADD_DYNAMIC_MACRO(name, getter) _macros->insert(pair<string, MacroProperties>(\
		name, {true, "", &macro_getter_##getter}))
	#define ADD_STATIC_MACRO(name, format) _macros->insert(pair<string, MacroProperties>(\
		name, {false, format, nullptr}))

	// compiler information
	ADD_STATIC_MACRO("__COMPILER_NAME__",			"\"" APP_NAME_INTERNAL "\"");
	ADD_STATIC_MACRO("__COMPILER_VERSION__",		"\"" APP_VERSION "\"");
	ADD_STATIC_MACRO("__COMPILER_BUILD_TARGET__",	"\"" TARGET "\"");
	ADD_STATIC_MACRO("__COMPILER_BUILD_DATE__",		"\"" __DATE__ "\"");
	ADD_STATIC_MACRO("__COMPILER_BUILD_TIME__",		"\"" __TIME__ "\"");
	ADD_STATIC_MACRO("__OPERATING_SYSTEM__",		"\"" OS_NAME "\"");

	// basic macros
	ADD_DYNAMIC_MACRO("__LINE__", line);
	ADD_DYNAMIC_MACRO("__FILE__", file);
	ADD_DYNAMIC_MACRO("__TIME__", time_);
	ADD_DYNAMIC_MACRO("__DATE__", date);
	ADD_DYNAMIC_MACRO("__COUNTER__", counter);
	ADD_DYNAMIC_MACRO("__APPLY_DEPTH__", apply_depth);
}
