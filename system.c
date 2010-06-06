
#include <stdio.h>
#include "src/system.h"

int main ( int argc, char ** argv )
{
    printf(
	"SYSTEM\t\t:= %s\n"
	"SYSTEMID\t:= 0x%x\n",
	SYSTEM, SYSTEMID );
    return 0;
}

