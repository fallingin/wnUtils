#include "threadpool.h"
#include <errno.h>
#include <iostream>


using namespace std;
void func(int a, int b)
{
    cout << a << b << endl;
}

int main()
{
    ThreadPool threadPool("wangning");
    threadPool.setMaxQueueSize(100);
    threadPool.setThreadInitCallback([]() { std::cout << "线程启动" << std::endl; });
    threadPool.start(10);
    // threadPool.stop();
    // std::cout << "qidongjieshu" << std::endl;
    int a = 1;
    int bn = 2;
    // threadpool::Task tasks(bind(func,a,bn));
    // while (true) {
    //     threadPool.run(bind(func, a, bn));
    // }

    return 0;
}