#ifndef EVI_MACROS_H
#define EVI_MACROS_H

#include "preprocessor.hpp"

class State
{
	friend void initialize_state_singleton(Preprocessor* p);

	State() {}

	inline static Preprocessor* _p;
	inline static bool _initialized;

public:

	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wreturn-type"

	#define FN(signature, code) static signature { \
		if(!_initialized) THROW_INTERNAL_ERROR("during dynamic macro expansion") \
		else code } 

	FN(uint get_current_line_no(), { return _p->_current_line_no; })
	FN(string get_current_file(), { return _p->_current_file; })
	FN(uint get_apply_depth(), { return _p->_apply_depth; })

	#pragma clang diagnostic pop
};

#endif