/* Includes */
#include "libpfc.h"
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* Typedefs */
typedef unsigned long long ull;
typedef signed   long long sll;


/**
 * Code snippet to bench
 */

static inline void code1(void){
	asm volatile(
	"vzeroall\n\t"
	"xor             %%eax, %%eax\n\t"
	"mov             $1000000000, %%rsi\n\t"
	"0:\n\t"
	"vfmadd231ps     %%ymm0,  %%ymm15, %%ymm0\n\t"
	"vfmadd231ps     %%ymm1,  %%ymm15, %%ymm1\n\t"
	"vfmadd231ps     %%ymm2,  %%ymm15, %%ymm2\n\t"
	"vpaddd          %%ymm10, %%ymm15, %%ymm10\n\t"
	"vfmadd231ps     %%ymm3,  %%ymm15, %%ymm3\n\t"
	"vfmadd231ps     %%ymm4,  %%ymm15, %%ymm4\n\t"
	"vfmadd231ps     %%ymm5,  %%ymm15, %%ymm5\n\t"
	"vpaddd          %%ymm11, %%ymm15, %%ymm11\n\t"
	"vfmadd231ps     %%ymm6,  %%ymm15, %%ymm6\n\t"
	"vfmadd231ps     %%ymm7,  %%ymm15, %%ymm7\n\t"
	"vpaddd          %%ymm12, %%ymm15, %%ymm12\n\t"
	"vpaddd          %%ymm13, %%ymm15, %%ymm13\n\t"
	"vfmadd231ps     %%ymm8,  %%ymm15, %%ymm8\n\t"
	"vfmadd231ps     %%ymm9,  %%ymm15, %%ymm9\n\t"
	"vpaddd          %%ymm14, %%ymm15, %%ymm14\n\t"
	"dec             %%rsi\n\t"
	"jnz             0b\n\t"
	: /* No outputs we care about */
	: /* No inputs we care about */
	: "xmm0",  "xmm1",  "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
	  "xmm8",  "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
	  "rsi", "rax", "rbx", "rcx", "rdx",
	  "memory"
	);
}


/* Test Schedule */
static const char* const SCHEDULE[];
static const int         NUMCOUNTS;


/**
 * Main
 */

int main(int argc, char* argv[]){
	int i;
	
	int verbose = 0, dump = 0, option;

	/**
	 * Process command line arguments
	 */
	while ((option = getopt(argc, argv, "dv")) != -1) {
		switch (option) {
		case 'v':
			verbose = 1;
			break;
		case 'd':
			dump = 1;
			break;
		default:
			fprintf(stderr, "Usage: pfcdemo [-d] [-v]\n"
					"\t-d\n\t\tDump the list of available events and quit\n"
					"\t-v\n\t\tVerbose output\n");
			exit(1);
		}
	}

	(void)verbose;

	if(dump){
			pfcDumpEvts();
			exit(1);
	}

	/**
	 * Initialize library.
	 */
	
	pfcPinThread(3);
	if(pfcInit() != 0){
		printf("Could not open /sys/module/pfc/* handles; Is module loaded?\n");
		exit(1);
	}
	
	


	/**
	 * Warm up.
	 */
	
	static const PFC_CNT ZERO_CNT[7]  = {0,0,0,0,0,0,0};
	PFC_CNT  CNT[7]                   = {0,0,0,0,0,0,0};
	PFC_CFG  CFG[7]                   = {2,2,2,0,0,0,0};
	for(i=0;i<1;i++){
		code1();
	}
	
	/**
	 * Run master loop under all counter configurations in SCHEDULE.
	 */
	
	for(i=0;i<NUMCOUNTS;i+=4){
		/* Configure general-purpose counters. */
		const char* sched0 = i+0 < NUMCOUNTS ? SCHEDULE[i+0] : "";
		const char* sched1 = i+1 < NUMCOUNTS ? SCHEDULE[i+1] : "";
		const char* sched2 = i+2 < NUMCOUNTS ? SCHEDULE[i+2] : "";
		const char* sched3 = i+3 < NUMCOUNTS ? SCHEDULE[i+3] : "";
		CFG[3] = pfcParseCfg(sched0);
		CFG[4] = pfcParseCfg(sched1);
		CFG[5] = pfcParseCfg(sched2);
		CFG[6] = pfcParseCfg(sched3);
		
		/* Reconfigure PMCs and clear their counts. */
		pfcWrCfgs(0, 7,      CFG);
		pfcWrCnts(0, 7, ZERO_CNT);
		
		
		/**
		 * We benchmark the warmed-up code.
		 * 
		 * The PFCSTART()/PFCEND() macro pair must sandwich the code to be
		 * tested as accurately as possible, although frequently one or more
		 * intruder instructions appear. Their argument is the buffer of (7)
		 * PFC_CNT counters.
		 * 
		 * PFCSTART() and PFCEND() do *not* replace the old values in the array.
		 * Instead they *accumulate* into the array a (biased) difference between
		 * the values of the counters at the moment PFCSTART() and PFCEND() were
		 * called. The bias is computable more-or-less precisely, and can be
		 * removed by invoking pfcRemoveBias(CNT).
		 */
		
		memset(CNT, 0, sizeof(CNT));
		
		/************** Hot section **************/
		PFCSTART(CNT);
		code1();
		PFCEND  (CNT);
		/************ End Hot section ************/
		
		
		/**
		 * Remove bias.
		 * 
		 * The "mul" argument should exactly equal the number of times
		 * the PFCSTART()/PFCEND() pair has been executed in
		 * the hot section.
		 */
		
		pfcRemoveBias(CNT, 1);
		
		/**
		 * Print the lovely results
		 */
		
		printf("Instructions Issued                  : %20lld\n", (sll)CNT[0]);
		printf("Unhalted core cycles                 : %20lld\n", (sll)CNT[1]);
		printf("Unhalted reference cycles            : %20lld\n", (sll)CNT[2]);
		printf("%-37s: %20lld\n", sched0                        , (sll)CNT[3]);
		printf("%-37s: %20lld\n", sched1                        , (sll)CNT[4]);
		printf("%-37s: %20lld\n", sched2                        , (sll)CNT[5]);
		printf("%-37s: %20lld\n", sched3                        , (sll)CNT[6]);
	}
	
	/**
	 * Close up shop
	 */
	
	pfcFini();
	return 0;
}


static const char* const SCHEDULE[] = {
	/* Batch */
	"uops_issued.any",
	"uops_issued.any<1",
	"uops_issued.any>=1",
	"uops_issued.any>=2",
	/* Batch */
	"uops_issued.any>=3",
	"uops_issued.any>=4",
	"uops_issued.any>=5",
	"uops_issued.any>=6",
	/* Batch */
	"uops_executed_port.port_0",
	"uops_executed_port.port_1",
	"uops_executed_port.port_2",
	"uops_executed_port.port_3",
	/* Batch */
	"uops_executed_port.port_4",
	"uops_executed_port.port_5",
	"uops_executed_port.port_6",
	"uops_executed_port.port_7",
	/* Batch */
	"resource_stalls.any",
	"resource_stalls.rs",
	"resource_stalls.sb",
	"resource_stalls.rob",
	/* Batch */
	"uops_retired.all",
	"uops_retired.all<1",
	"uops_retired.all>=1",
	"uops_retired.all>=2",
	/* Batch */
	"uops_retired.all>=3",
	"uops_retired.all>=4",
	"uops_retired.all>=5",
	"uops_retired.all>=6",
	/* Batch */
	"inst_retired.any_p",
	"inst_retired.any_p<1",
	"inst_retired.any_p>=1",
	"inst_retired.any_p>=2",
	/* Batch */
	"inst_retired.any_p>=3",
	"inst_retired.any_p>=4",
	"inst_retired.any_p>=5",
	"inst_retired.any_p>=6",
	/* Batch */
	"idq_uops_not_delivered.core",
	"idq_uops_not_delivered.core<1",
	"idq_uops_not_delivered.core>=1",
	"idq_uops_not_delivered.core>=2",
	/* Batch */
	"idq_uops_not_delivered.core>=3",
	"idq_uops_not_delivered.core>=4",
	"rs_events.empty",
	"idq.empty",
	/* Batch */
	"idq.mite_uops",
	"idq.dsb_uops",
	"idq.ms_dsb_uops",
	"idq.ms_mite_uops",
	/* Batch */
	"idq.mite_all_uops",
	"idq.mite_all_uops<1",
	"idq.mite_all_uops>=1",
	"idq.mite_all_uops>=2",
	/* Batch */
	"idq.mite_all_uops>=3",
	"idq.mite_all_uops>=4",
	"move_elimination.int_not_eliminated",
	"move_elimination.simd_not_eliminated",
	/* Batch */
	"lsd.uops",
	"lsd.uops<1",
	"lsd.uops>=1",
	"lsd.uops>=2",
	/* Batch */
	"lsd.uops>=3",
	"lsd.uops>=4",
	"ild_stall.lcp",
	"ild_stall.iq_full",
	/* Batch */
	"br_inst_exec.all_branches",
	"br_inst_exec.0x81",
	"br_inst_exec.0x82",
	"icache.misses",
	/* Batch */
	"br_misp_exec.all_branches",
	"br_misp_exec.0x81",
	"br_misp_exec.0x82",
	"fp_assist.any",
	/* Batch */
	"cpu_clk_unhalted.core_clk",
	"cpu_clk_unhalted.ref_xclk",
	"baclears.any",
	"idq.ms_uops"
};
static const int NUMCOUNTS = sizeof(SCHEDULE)/sizeof(*SCHEDULE);

