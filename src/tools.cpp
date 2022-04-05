#include "tools.hpp"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

// ========= string ops =========

// split string by delimeter
vector<string> tools::split_string(const string& str, const string& delimiter)
{
    vector<string> strings;

    string::size_type pos = 0;
    string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != string::npos)
    {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + 1;
    }

    // To get the last substring (or only, if delimiter is not found)
    strings.push_back(str.substr(prev));

    return strings;
}

// replace substring
string tools::replacestr(string source, string oldstring, string newstring)
{
    // https://stackoverflow.com/a/7724536
    string retval;
    string::const_iterator end     = source.end();
    string::const_iterator current = source.begin();
    string::const_iterator next    = search( current, end, oldstring.begin(), oldstring.end() );
    while (next != end)
    {
        retval.append(current, next);
        retval.append(newstring);
        current = next + oldstring.size();
        next = search( current, end, oldstring.begin(), oldstring.end() );
    }
    retval.append(current, next);
    return retval;
}

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

// e.g. turns 'n' into an actual newline (returns -1 if invalid)
char tools::escchr(char ogchar)
{
    switch(ogchar)
    {
        case 'a': return '\a';
        case 'b': return '\b';
        case 'e': return '\e';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case '\\': return '\\';
        case '\'': return '\'';
        case '\"': return '\"';
	case '0': return '\0';
	default: return -1;
    }
}

// turns e.g. a newline into "\\n"
const char* tools::unescchr(char escchar)
{
    switch(escchar)
    {
        case '\a': return "\\a";
        case '\b': return "\\b";
        case '\e': return "\\e";
        case '\f': return "\\f";
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
        case '\v': return "\\v";
        case '\\': return "\\\\";
        case '\'': return "\\\'";
        case '\"': return "\\\"";
	case '\0': return "\\0";
	default: return new char[2]{escchar, '\0'};
    }
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
            // CASE('\0', "\\0");
            default: ret << str[i];
        }
        #undef CASE
    }
    return ret.str();
}

string tools::escstr(string str)
{
    stringstream ret;
    bool isesc = false;
    for(char& ch : str)
    {
        if(ch == '\\') isesc = true;
        else if(isesc)
        {
            ret << escchr(ch);
            isesc = false;
        }
        else ret << ch;
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

// write string to file
void tools::writef(string path, string text)
{
    fstream file_out(path, ios_base::out);

    if (!file_out.is_open())
    {
        cerr << "failed to open " << path << '\n';
        exit(74);
    }
    
    file_out << text;
    file_out.close();
}
