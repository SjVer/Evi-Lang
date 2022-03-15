#ifndef EVI_ERROR_H
#define EVI_ERROR_H

#include "scanner.hpp"

class ErrorDispatcher
{
    private:

    typedef const char* ccp;
    // ccp _source;

    // void __dispatch(bool at, Token* t, ccp c, 
    //                 ccp p, ccp m,
    //                 uint line = 0, ccp file = NULL);

    void __dispatch(ccp color, ccp prompt, ccp message);
    void __dispatch_at_token(ccp color, Token *token, ccp prompt, ccp message);
    void __dispatch_at_line(ccp color, uint line, ccp filename, ccp prompt, ccp message);    

    public:

    ErrorDispatcher() {}
    // ErrorDispatcher(ccp source):
    //     _source(source) {}

    void print_token_marked(Token *token, ccp color);

    void error(ccp prompt, ccp message);
    void error_at_token(Token *token, ccp prompt, ccp message);
    void error_at_line(uint line, ccp filename, ccp prompt, ccp message);

    void warning(ccp prompt, ccp message);
    void warning_at_token(Token *token, ccp prompt, ccp message);
    void warning_at_line(uint line, ccp filename, ccp prompt, ccp message);

    void note(ccp message);
    void note_at_token(Token *token, ccp message);
    void note_at_line(uint line, ccp filename, ccp message);
};

#endif