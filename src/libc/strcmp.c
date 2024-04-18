#include <string.h>


size_t strcmp(char* str1, char* str2)
{
	size_t res=0;
	while (!(res = *(unsigned char*)str1 - *(unsigned char*)str2) && *str2)
		++str1, ++str2;

	return res;
}