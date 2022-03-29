// lexcast.cpp -- simple cast from float to string
//#define _CRT_SECURE_NO_WARNINGS


#include <string>
#include "wnmd5.h"
#include "wncrc32.h"

#include <iostream>
#include <string>  
#include <fstream>  
#include <streambuf>  
/*
int main111()
{
	//std::cout << "md5 of 'grape': " << md5("grape") << std::endl;
	std::string test = md5("grape");
	std::cout << crc32((const unsigned char*)test.c_str(), test.size()) << std::endl;
	system("pause");
	return 0;
}*/
std::string encrypt_file(std::string file_path)
{
	std::ifstream file_stream(file_path);
	std::string file_content((std::istreambuf_iterator<char>(file_stream)),
		std::istreambuf_iterator<char>());
	std::string MD5_ = md5(file_content);

	int CRC_int = crc32((const unsigned char*)file_content.c_str(), MD5_.size());
	std::string CRC_ = std::to_string(CRC_int);
	
	std::string file_size = std::to_string(file_content.size());
	return *(new std::string(MD5_ + CRC_ + file_size));
}
int main()
{

	std::string &&aa = encrypt_file("N:\\地址_密码.txt");
	std::cout << aa << std::endl;

	return 0;
}