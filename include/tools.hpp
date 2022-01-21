#ifndef TOOLS_H
#define TOOLS_H

#include <string>

using namespace std;

namespace tools {

    // string ops
    string fstr(string format, ...);
    string escstr(string str);
    string unescstr(string str, bool ign_s_quotes = false, bool ign_d_quotes = false);

    // file ops
    string readf(string path);
}
#endif