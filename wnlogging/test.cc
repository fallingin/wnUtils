#include <iostream>
#include "wnlogging.h"


int main()
{
	errno = 1;
	LOG_SYSERR ;
	LOG_DEBUG << "test";

	return 0;
}