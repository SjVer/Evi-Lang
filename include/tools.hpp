#ifndef TOOLS_H
#define TOOLS_H

#include <string>
#include <vector>

using namespace std;

namespace tools {

    // string ops
    vector<string> split_string(const string& str, const string& delimiter);
    string fstr(string format, ...);
    string replacestr(string source, string oldstring, string newstring);
    char escchr(char ogchar);
    const char* unescchr(char escchar);
    string escstr(string str);
    string unescstr(string str, bool ign_s_quotes = false, bool ign_d_quotes = false);

    // file ops
    int execbin(const char* executable, const char** argv);
    string readf(string path);
    void writef(string path, string text);
}
#endif