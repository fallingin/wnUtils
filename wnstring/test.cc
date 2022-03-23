
#include <errno.h>
#include <iostream>


using namespace std;
union test {
    int a;
    char b;
} c;

int main(int argc, char const *argv[])
{
    c.a = 1;
    cout << (c.b & 1 ? "小端" : "大端") << endl;
    return 0;
}
