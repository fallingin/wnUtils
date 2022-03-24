
#include <errno.h>
#include <iostream>
#include <cstring>


#include "wnstring.h"

using namespace std;


int main(int argc, char const *argv[])
{
    const char* const test = "helloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworld";
    cout << strlen(test) << endl;
    Wnstring testStr(forward<const char* const>(test),strlen(test));
    cout << testStr.size() << endl;
    cout << testStr.c_str();

    return 0;
}
