#ifndef EVI_ERROR_H
#define EVI_ERROR_H

#include "scanner.hpp"
#include "common.hpp"

class ErrorDispatcher
{
    private:

    const char* _source;
    const char* _infile;

    void __dispatch(bool at, Token* t, const char* c, 
                    const char* p, const char* m);

    public:

    ErrorDispatcher() {}
    ErrorDispatcher(const char* source, const char* infile):
        _source(source), _infile(infile) {}

    void dispatch_token_marked(Token *token);
    void dispatch_error(const char* prompt, const char* message);
    void dispatch_error_at(Token *token, const char* prompt, const char* message);
    void dispatch_warning(const char* prompt, const char* message);
    void dispatch_warning_at(Token *token, const char* prompt, const char* message);
};

#endif