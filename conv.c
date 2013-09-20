/* routine to convert 8bit ascii to similar-looking 7bit ascii */

#include <stdio.h>
#include "conv.h"

void ConvertAscii(unsigned char *p) {
	while (*p != '\0') {
		*p = ConvTable[*p];
		p++;
	}
}
