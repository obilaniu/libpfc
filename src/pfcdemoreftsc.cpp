/* Includes */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "libpfc.h"
#include <chrono>
#include <sched.h>
#include <unistd.h>
#include <x86intrin.h>


/**
 * A 1CC/iter workload.
 * 
 * Almost exactly one *core* clock cycle per iteration on modern x86 CPUs
 * capable of macro-fusion of the arithmetic+conditional jump pattern.
 */

extern "C" void add_calibration(uint64_t x);
asm volatile(
".text\n\t"
".global add_calibration\n\t"
"add_calibration:\n\t"
"sub $1, %rdi\n\t"
"jnz add_calibration\n\t"
"ret\n\t"
);


/**
 * The C++ timer, which uses rdtsc anyhow.
 */

int64_t nanos(){
	auto t = std::chrono::high_resolution_clock::now();
	return std::chrono::time_point_cast<std::chrono::nanoseconds>(t).time_since_epoch().count();
}


/**
 * Main.
 */

int main(int argc, char* argv[]){
	uint64_t CALIBRATION_NANOS = 1000000000,
	         CALIBRATION_ITERS = 100,
	         CALIBRATION_ADDS;
	int      CORE              = 0;
	FILE*    LOG               = NULL;
	int      i, ret, isterminal = isatty(1);
	uint64_t maxNonTurbo = 0;
	uint64_t miscEn = 0, platInfo = 0, tempTarget = 0,
	         throttleReason, pkgThermStatus, pkgThermInterrupt;
	PFC_CFG  cfg[7] = {7,7,7,0,0,0,0};
	PFC_CNT  cnt[7] = {0,0,0,0,0,0,0};
	const char* startDel = isterminal ? "\033[1m* " : "* ",
	          * endDel   = isterminal ? "\033[0m"   : "";
	
	
	/* El-Cheapo Argument Parsing */
	for(i=1;i<argc;i++){
		if      (strcmp(argv[i], "--help")  == 0){
			printf("%s [--help | --nanos NANOS_PER_ITER | --core CORE_NUM]\n", argv[0]);
			exit(0);
		}else if(strcmp(argv[i], "--nanos") == 0 && argv[++i]){
			CALIBRATION_NANOS = strtoull(argv[i], 0, 0);
		}else if(strcmp(argv[i], "--iters") == 0 && argv[++i]){
			CALIBRATION_ITERS = strtoull(argv[i], 0, 0);
		}else if(strcmp(argv[i], "--core")  == 0 && argv[++i]){
			CORE              = strtoull(argv[i], 0, 0);
		}else if(strcmp(argv[i], "--log")   == 0 && argv[++i]){
			LOG               = fopen(argv[i], "a");
			if(!LOG){
				printf("Could not open %s, exiting.\n", argv[i]);
				exit(-1);
			}
		}
	}
	
	
	/* Initialization of libpfc */
	pfcPinThread(CORE);
	ret = pfcInit();
	if(ret != 0){
		printf("Failed to initialize libpfc (%d)!\n", ret);
		return -1;
	}
	
	
	/* Print some relatively time-invariant MSRs up front. */
	ret = pfcRdMSR(MSR_IA32_MISC_ENABLE,        &miscEn);
	if(ret != 8){
		printf("Err %d: Failed to read MSR!\n", ret);
	}
	ret = pfcRdMSR(MSR_PLATFORM_INFO,           &platInfo);
	if(ret != 8){
		printf("Err %d: Failed to read MSR!\n", ret);
	}
	ret = pfcRdMSR(MSR_IA32_TEMPERATURE_TARGET, &tempTarget);
	if(ret != 8){
		printf("Err %d: Failed to read MSR!\n", ret);
	}
	printf("MSR_IA32_MISC_ENABLE        = %016llx\n", (unsigned long long)miscEn);
	printf("MSR_PLATFORM_INFO           = %016llx\n", (unsigned long long)platInfo);
	printf("MSR_IA32_TEMPERATURE_TARGET = %016llx\n", (unsigned long long)tempTarget);
	
	
	/* On many newer processors, XCLK runs at 100MHz. That's what we assume here. */
	maxNonTurbo      = (platInfo >> 8) & 0xFF;
	CALIBRATION_ADDS = CALIBRATION_NANOS*maxNonTurbo/10;
	
	
	/* Configure a couple counters. */
	/*   Reference XCLK */
	cfg[3]  = pfcParseCfg("cpu_clk_unhalted.ref_xclk:auk");
	/*   OS-mode Core Clocks, serves as a loose measure of kernel overhead. */
	cfg[4]  = pfcParseCfg("cpu_clk_unhalted.core_clk:k");
	/*    User-OS Transition Count */
	cfg[5]  = pfcParseCfg("*cpl_cycles.ring0>=1:uk");
	pfcWrCfgs(0, 7, cfg);
	pfcWrCnts(0, 7, cnt);
	
	
	/* Warm up the CPU. */
	for(volatile int v = 0; v < 1000000000; v++){}
	
	
	/* Periodically dump the synchronized measurements of several counters, timers and MSRs. */
	for(i=0;i<(int)CALIBRATION_ITERS;i++){
		memset(cnt, 0, sizeof(cnt));
		int64_t start = nanos(), now;
		PFCSTART(cnt);
		int64_t tsc =__rdtsc();
		add_calibration(CALIBRATION_ADDS);
		PFCEND(cnt);
		int64_t tsc_delta = __rdtsc() - tsc;
		now = nanos();
	
		int64_t delta     = now - start;
		pfcRdMSR(MSR_CORE_PERF_LIMIT_REASONS,      &throttleReason);
		pfcRdMSR(MSR_IA32_PACKAGE_THERM_STATUS,    &pkgThermStatus);
		pfcRdMSR(MSR_IA32_PACKAGE_THERM_INTERRUPT, &pkgThermInterrupt);
		printf("CPU %d, measured CLK_REF_TSC MHz        : %16.2f\n",  sched_getcpu(), 1000.0 * cnt[PFC_FIXEDCNT_CPU_CLK_REF_TSC] / delta);
		printf("CPU %d, measured rdtsc MHz              : %16.2f\n",  sched_getcpu(), 1000.0 * tsc_delta / delta);
		printf("CPU %d, measured add   MHz              : %16.2f\n",  sched_getcpu(), 1000.0 * CALIBRATION_ADDS / delta);
		printf("CPU %d, measured XREF_CLK  time (s)     : %16.8f\n",  sched_getcpu(), cnt[3]/1e8);
		printf("CPU %d, measured delta     time (s)     : %16.8f\n",  sched_getcpu(), delta/1e9);
		printf("CPU %d, measured tsc_delta time (s)     : %16.8f\n",  sched_getcpu(), tsc_delta/2.4e9);
		printf("CPU %d, ratio ref_tsc :ref_xclk         : %16.8f\n",  sched_getcpu(), (double)cnt[PFC_FIXEDCNT_CPU_CLK_REF_TSC]/cnt[3]);
		printf("CPU %d, ratio ref_core:ref_xclk         : %16.8f\n",  sched_getcpu(), (double)cnt[PFC_FIXEDCNT_CPU_CLK_UNHALTED]/cnt[3]);
		printf("CPU %d, ratio rdtsc   :ref_xclk         : %16.8f\n",  sched_getcpu(), (double)tsc_delta/cnt[3]);
		printf("CPU %d, core CLK cycles in OS           : %16llu\n",  sched_getcpu(), (unsigned long long)(cnt[4]));
		printf("CPU %d, User-OS transitions             : %16llu\n",  sched_getcpu(), (unsigned long long)(cnt[5]));
		printf("CPU %d, rdtsc-reftsc overcount          : %16lld\n",  sched_getcpu(), (signed   long long)(tsc_delta - cnt[PFC_FIXEDCNT_CPU_CLK_REF_TSC]));
		printf("CPU %d, MSR_IA32_PACKAGE_THERM_STATUS   : %016llx\n", sched_getcpu(), (unsigned long long)pkgThermStatus);
		printf("CPU %d, MSR_IA32_PACKAGE_THERM_INTERRUPT: %016llx\n", sched_getcpu(), (unsigned long long)pkgThermInterrupt);
		printf("CPU %d, MSR_CORE_PERF_LIMIT_REASONS     : %016llx\n", sched_getcpu(), (unsigned long long)throttleReason);
		printf("      %sPROCHOT%s\n",                                        throttleReason & 0x00010000 ? startDel : "  ", throttleReason & 0x00010000 ? endDel : "");
		printf("      %sThermal%s\n",                                        throttleReason & 0x00020000 ? startDel : "  ", throttleReason & 0x00020000 ? endDel : "");
		printf("      %sGraphics Driver%s\n",                                throttleReason & 0x00100000 ? startDel : "  ", throttleReason & 0x00100000 ? endDel : "");
		printf("      %sAutonomous Utilization-Based Frequency Control%s\n", throttleReason & 0x00200000 ? startDel : "  ", throttleReason & 0x00200000 ? endDel : "");
		printf("      %sVoltage Regulator Thermal Alert%s\n",                throttleReason & 0x00400000 ? startDel : "  ", throttleReason & 0x00400000 ? endDel : "");
		printf("      %sElectrical Design Point (e.g. Current)%s\n",         throttleReason & 0x01000000 ? startDel : "  ", throttleReason & 0x01000000 ? endDel : "");
		printf("      %sCore Power Limiting%s\n",                            throttleReason & 0x02000000 ? startDel : "  ", throttleReason & 0x02000000 ? endDel : "");
		printf("      %sPackage-Level PL1 Power Limiting%s\n",               throttleReason & 0x04000000 ? startDel : "  ", throttleReason & 0x04000000 ? endDel : "");
		printf("      %sPackage-Level PL2 Power Limiting%s\n",               throttleReason & 0x08000000 ? startDel : "  ", throttleReason & 0x08000000 ? endDel : "");
		printf("      %sMax Turbo Limit (Multi-Core Turbo)%s\n",             throttleReason & 0x10000000 ? startDel : "  ", throttleReason & 0x10000000 ? endDel : "");
		printf("      %sTurbo Transition Attenuation%s\n",                   throttleReason & 0x20000000 ? startDel : "  ", throttleReason & 0x20000000 ? endDel : "");
		
		fprintf(LOG, "%.2f,%.2f,%.2f,%lld,%llu,%llu\n",
		        (double)cnt[PFC_FIXEDCNT_CPU_CLK_REF_TSC]/cnt[3],
		        (double)cnt[PFC_FIXEDCNT_CPU_CLK_UNHALTED]/cnt[3],
		        (double)tsc_delta/cnt[3],
		        (signed   long long)(tsc_delta - cnt[PFC_FIXEDCNT_CPU_CLK_REF_TSC]),
		        (unsigned long long)(cnt[4]),
		        (unsigned long long)(cnt[5]));
	}
	fflush(LOG);
	fclose(LOG);
	
	return 0;
}
