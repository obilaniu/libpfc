/* Includes */
#include "libpfc.h"
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * Main
 */

int main(int argc, char* argv[]){
	if(pfcInit() != 0){
		printf("Could not open /sys/module/pfc/* handles; Is module loaded?\n");
		exit(1);
	}
	
	unsigned msrNum = MSR_IA32_ENERGY_PERF_BIAS;
	uint64_t msr    = 0;
	if(pfcRdMSR(msrNum, &msr) == sizeof(msr)){
		printf("MSR %03x = %016llx\n", msrNum, (unsigned long long)msr);
	}else{
		printf("Failed to read MSR %03x!\n", msrNum);
	}
	
	
	return 0;
}
