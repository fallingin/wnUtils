
#include <errno.h>
#include <iostream>
#include <cstring>


#include "wnstring.h"

using namespace std;


int main(int argc, char const *argv[])
{
    const char* const test = "hello world !!!";
    // cout << strlen(test) << endl;
    Wnstring testStr(forward<const char* const>(test),strlen(test));


    return 0;
}
