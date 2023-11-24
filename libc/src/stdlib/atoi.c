#include "string.h"
#include <stdlib.h>

int atoi(const char* str){
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i)
        res = res * 10 + str[i] - '0';
    return res;
}

void* itoa(int n, char* s, int radix) {
     int i, sign;
 
     if ((sign = n) < 0)
		 n = -n;          
     i = 0;
     do {       
		 if(radix < 11) {
            s[i++] = n % radix + '0';   
		 } else {
            s[i++] = n % radix + 'a';   
		 }
     } while ((n /= 10) > 0);     
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
	 return s;
}
