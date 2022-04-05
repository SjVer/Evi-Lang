#ifndef EVI_ERROR_H
#define EVI_ERROR_H

#include "scanner.hpp"

class ErrorDispatcher
{
    private:

    void __dispatch(ccp color, ccp prompt, ccp message);
    void __dispatch_at_token(ccp color, Token *token, ccp prompt, ccp message);
    void __dispatch_at_line(ccp color, uint line, ccp filename, ccp prompt, ccp message);    

    public:

    ErrorDispatcher() {}

    void print_token_marked(Token *token, ccp color);
    void print_line_marked(uint line_no, string line, ccp color);

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