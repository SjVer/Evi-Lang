#include "tools.hpp"

#include <cstdarg>
#include <cstdio>
#include <string>
#include <sstream>

using namespace std;

// ==============================

// ========= string ops =========

// format the given strign
string tools::fstr(string format, ...)
{
    va_list args;
    va_start(args, format.c_str());

    int smallSize = sizeof(char) * 1024;
    char *smallBuffer = (char*)malloc(smallSize);

    int size = vsnprintf(smallBuffer, smallSize, format.c_str(), args);

    va_end(args);

    if (size < sizeof smallBuffer) return string(smallBuffer);

    int bigSize = sizeof(char) * (size + 1);
    char *buffer = (char*)malloc(bigSize);

    va_start(args, format.c_str());
    vsnprintf(buffer, bigSize, format.c_str(), args);
    va_end(args);

    return string(buffer);
}

// turn shit like '\n' into '\\n'
string tools::unescstr(string str, bool ign_s_quotes, bool ign_d_quotes)
{
    stringstream ret;
    for(int i = 0; i < str.length(); i++)
    {
        #define CASE(ch, res) case ch: ret << res; break
        switch(str[i])
        {
            CASE('\a', "\\a");
            CASE('\b', "\\b");
            CASE('\e', "\\e");
            CASE('\f', "\\f");
            CASE('\n', "\\n");
            CASE('\r', "\\r");
            CASE('\t', "\\t");
            CASE('\v', "\\v");
            CASE('\\', "\\\\");
            CASE('\'', (ign_s_quotes ? "\'" : "\\\'"));
            CASE('\"', (ign_d_quotes ? "\"" : "\\\""));
            default: ret << str[i];
        }
        #undef CASE
    }
    return ret.str();
}

// ========== file ops ========== 

// read contents of file to string
string tools::readf(string path)
{
    FILE *file = fopen(path.c_str(), "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path.c_str());
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path.c_str());
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path.c_str());
        exit(74);
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}