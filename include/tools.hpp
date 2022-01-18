#ifndef TOOLS_H
#define TOOLS_H

#include <string>

using namespace std;

namespace tools {

    // string ops
    string fstr(string format, ...);

    // file ops
    string readf(string path);
}
#endif