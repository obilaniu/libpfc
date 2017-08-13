/* Includes */
#include "libpfc.h"
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * Data structures.
 */

struct TBHIT;
typedef struct TBHIT TBHIT;
struct TBHIT{
	uint64_t tstart, tend, hit;
};


/* Assembler function */
extern void  pfcSampleTBHITs(TBHIT* tbhit,
                             size_t n,
                             size_t THRESH);


/**
 * Main
 */

int main(int argc, char* argv[]){
	size_t    i;
	int       CORE   = 0;
	size_t    THRESH = 1000;
	size_t    BUFFER = 1024;
	TBHIT*    hits   = NULL;
	PFC_CFG   cfg[7] = {7,7,7,0,0,0,0};
	PFC_CNT   cnt[7] = {0,0,0,0,0,0,0};
	
	
	/* Arg parse */
	for(i=1;argv[i];i++){
		if      (!strcmp(argv[i], "--core")){
			CORE   = argv[i+1] ? strtol  (argv[++i], 0, 0) : CORE;
		}else if(!strcmp(argv[i], "--thresh")){
			THRESH = argv[i+1] ? strtoull(argv[++i], 0, 0) : THRESH;
		}else if(!strcmp(argv[i], "--buf")){
			BUFFER = argv[i+1] ? strtoull(argv[++i], 0, 0) : BUFFER;
		}else{
			fprintf(stderr, "Unknown argument %s!\n", argv[i]);
			exit(1);
		}
	}
	
	/* Buffer allocate, then touch each page with memset() */
	hits  =  malloc(sizeof(*hits) * BUFFER);
	memset(hits, 0, sizeof(*hits) * BUFFER);
	
	/* libpfc init */
	pfcPinThread(CORE);
	if(pfcInit() != 0){
		printf("Could not open /sys/module/pfc/* handles; Is module loaded?\n");
		exit(1);
	}
	
	/* Setup counters */
	cfg[3]  = pfcParseCfg("*cpl_cycles.ring0>=1:uk");
	pfcWrCfgs(0, 7, cfg);
	pfcWrCnts(0, 7, cnt);
	
	/* Iterate endlessly. */
	while(1){
		pfcSampleTBHITs(hits, BUFFER, THRESH);
		for(i=0;i<BUFFER;i++){
			printf("Tstart: %12llu   Tend: %12llu   Delta: %12llu   Hit: %12llu\n",
			       (unsigned long long)hits[i].tstart,
			       (unsigned long long)hits[i].tend,
			       (unsigned long long)(hits[i].tend-hits[i].tstart),
			       (unsigned long long)hits[i].hit);
		}
		fflush(stdout);
	}
	
	
	return 0;
}
