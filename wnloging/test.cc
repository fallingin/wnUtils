#include <iostream>
#include "wnlogging.h"


int main()
{
	errno = 1;
	LOG_SYSERR ;
	LOG_INFO << "test";

	system("pause");
	return 0;
}