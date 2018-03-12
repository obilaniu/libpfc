/* Includes */
#include <asm/processor.h>
#include <asm/msr.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/smp.h>
#include "libpfcmsr.h"


/* Defines */

/**
 * Maximum number of PMCs (fixed-function and general-purpose combined)
 */

#define MAXPMC                             25

/**
 * Conditional logging.
 */

#define PRINTV(...) if (verbose) { printk(KERN_INFO __VA_ARGS__); }


/* Data Structure Typedefs */
struct CPUID_LEAF;
typedef struct CPUID_LEAF CPUID_LEAF;


/* Data Structure Definitions */
struct CPUID_LEAF{
	uint32_t a, b, c, d;
};


/* Forward Declarations */
static ssize_t pfcCfgRd(struct file*          f,
                        struct kobject*       kobj,
                        struct bin_attribute* binattr,
                        char*                 buf,
                        loff_t                off,
                        size_t                len);
static ssize_t pfcCfgWr(struct file*          f,
                        struct kobject*       kobj,
                        struct bin_attribute* binattr,
                        char*                 buf,
                        loff_t                off,
                        size_t                len);
static ssize_t pfcMskRd(struct file*          f,
                        struct kobject*       kobj,
                        struct bin_attribute* binattr,
                        char*                 buf,
                        loff_t                off,
                        size_t                len);
static ssize_t pfcCntRd(struct file*          f,
                        struct kobject*       kobj,
                        struct bin_attribute* binattr,
                        char*                 buf,
                        loff_t                off,
                        size_t                len);
static ssize_t pfcCntWr(struct file*          f,
                        struct kobject*       kobj,
                        struct bin_attribute* binattr,
                        char*                 buf,
                        loff_t                off,
                        size_t                len);
static ssize_t pfcMsrRd (struct file*          f,
                         struct kobject*       kobj,
                         struct bin_attribute* binattr,
                         char*                 buf,
                         loff_t                off,
                         size_t                len);
static ssize_t pfcVerboseRd(struct kobject*        kobj,
                            struct kobj_attribute* attr,
                            char*                  buf);
static ssize_t pfcVerboseWr(struct kobject*        kobj,
                            struct kobj_attribute* attr,
                            const char*            buf,
                            size_t                 count);
static ssize_t pfcCR4PceRd(struct kobject*         kobj,
                           struct kobj_attribute*  attr,
                           char*                   buf);
static int  __init pfcInit(void);
static void        pfcExit(void);



/* Global Variables & Constants */
static CPUID_LEAF leaf0                = {0,0,0,0}, /* Highest Value for Basic Processor Information and the Vendor Identification String */
                  leaf1                = {0,0,0,0}, /* Model, Family, Stepping Information */
                  leaf6                = {0,0,0,0}, /* Thermal and Power Management Features */
                  leafA                = {0,0,0,0}, /* Architectural Performance Monitoring Features */
                  leaf80000000         = {0,0,0,0}, /* Highest Value for Extended Processor Information */
                  leaf80000001         = {0,0,0,0}, /* Extended Processor Signature & Feature Bits */
                  leaf80000002         = {0,0,0,0}, /* Processor Brand String 0 */
                  leaf80000003         = {0,0,0,0}, /* Processor Brand String 1 */
                  leaf80000004         = {0,0,0,0}; /* Processor Brand String 2 */
static unsigned   family               = 0;
static unsigned   model                = 0;
static unsigned   stepping             = 0;
static unsigned   exfamily             = 0;
static unsigned   exmodel              = 0;
static unsigned   dispFamily           = 0;
static unsigned   dispModel            = 0;
static uint32_t   maxLeaf              = 0;
static uint32_t   maxExtendedLeaf      = 0;
static char       procBrandString[49]  = {0};
static int        pmcArchVer           = 0;
static int        pmcFf                = 0;
static int        pmcGp                = 0;
static int        pmcFfBitwidth        = 0;
static int        pmcGpBitwidth        = 0;
static uint64_t   pmcFfMask            = 0;
static uint64_t   pmcGpMask            = 0;
static int        pmcStartFf           = 0;
static int        pmcEndFf             = 0;
static int        pmcStartGp           = 0;
static int        pmcEndGp             = 0;
static int        fullWidthWrites      = 0;
static int        verbose              = 0;

/**
 * The counters consist in the following MSRs on Core i7:
 * [0   ] IA32_FIXED_CTR0:             Fixed-function  - Retired Instructions
 * [1   ] IA32_FIXED_CTR1:             Fixed-function  - Unhalted Core CCs
 * [2   ] IA32_FIXED_CTR2:             Fixed-function  - Unhalted Reference CCs
 * [3+  ] IA32_A_PMCx:                 General-purpose - Configurable
 */


/* Attribute Hierarchy */
/* Binary attributes */
static const struct bin_attribute   PFC_ATTR_config     = {
	.attr    = {.name="config", .mode=0660},
	.size    = MAXPMC*sizeof(uint64_t),
	.read    = pfcCfgRd,
	.write   = pfcCfgWr
};
static const struct bin_attribute   PFC_ATTR_masks      = {
	.attr    = {.name="masks",  .mode=0440},
	.size    = MAXPMC*sizeof(uint64_t),
	.read    = pfcMskRd,
};
static const struct bin_attribute   PFC_ATTR_counts     = {
	.attr    = {.name="counts", .mode=0660},
	.size    = MAXPMC*sizeof(uint64_t),
	.read    = pfcCntRd,
	.write   = pfcCntWr
};
static const struct bin_attribute   PFC_ATTR_msr        = {
	.attr    = {.name="msr",    .mode=0660},
	.size    = 0,
	.read    = pfcMsrRd
};
static const struct bin_attribute*  PFC_BIN_ATTR_GRP_LIST[] = {
	&PFC_ATTR_config,
	&PFC_ATTR_masks,
	&PFC_ATTR_counts,
	&PFC_ATTR_msr,
	NULL
};

/* string attributes */
static struct kobj_attribute PFC_ATTR_verbose = {
		.attr  = {.name="verbose", .mode=0660},
		.show  = pfcVerboseRd,
		.store = pfcVerboseWr

};

static struct kobj_attribute PFC_ATTR_cr4pce = {
        .attr  = {.name="cr4.pce", .mode=0440},
        .show  = pfcCR4PceRd

};

static struct attribute*  PFC_STR_ATTR_GRP_LIST[] = {
		&PFC_ATTR_verbose.attr,
		&PFC_ATTR_cr4pce.attr,
		NULL
};

/* the attribute group, which points to all binary and string attributes */
static const struct attribute_group PFC_ATTR_GRP        = {
	.name       = NULL,
	.attrs      = PFC_STR_ATTR_GRP_LIST,
	.bin_attrs  = (struct bin_attribute	**)PFC_BIN_ATTR_GRP_LIST
};




/* Static Function Definitions */

/***************** UTILITIES *****************/

/**
 * @brief Ones Vector.
 *
 * Generate a 64-bit bitvector of ones with
 * 
 * val[63   :n+k] = 0
 * val[n+k-1:  k] = 1
 * val[  k-1:  0] = 0
 * 
 * i.e. where the n bits starting at k counting from the LSB are all set, and
 * all other bits are 0.
 */

static uint64_t OV(int n, int k){
	uint64_t v = n >= 64 ? ~0 : ((uint64_t)1 << n) - 1;
	return v << k;
}

/**
 * @brief Zeros Vector.
 *
 * Generate a 64-bit bitvector of zeros with
 * 
 * val[63   :n+k] = 1
 * val[n+k-1:  k] = 0
 * val[  k-1:  0] = 1
 * 
 * i.e. where the n bits starting at k counting from the LSB are all clear, and
 * all other bits are 1.
 */

static uint64_t ZV(int n, int k){
	return ~OV(n, k);
}

/**
 * @brief Bit Vector.
 *
 * Generate a 64-bit bitvector with
 * 
 * val[63   :n+k] = 0
 * val[n+k-1:  k] = v[n-1:0]
 * val[  k-1:  0] = 0
 * 
 * i.e. where the n bits starting at k counting from the LSB are taken from
 * the LSBs of v, and all other bits are 0.
 */

static uint64_t BV(uint64_t v, int n, int k){
	v &= OV(n, 0);
	return v << k;
}

/**
 * @brief Clear Vector.
 *
 * Generate a 64-bit bitvector with
 * 
 * val            = v
 * val[n+k-1:  k] = 0
 * 
 * i.e. where the n bits starting at k counting from the LSB are set to 0,
 * and all other bits are taken from v.
 */

static uint64_t CV(uint64_t v, int n, int k){
	return v & ZV(n,k);
}

/**
 * @brief RDMSR wrapper.
 * 
 * Returns the unmodified value of the given MSR.
 * 
 * @returns The value read at the given MSR.
 * @note    Does *not* check that addr is valid!
 */

static uint64_t pfcRDMSR(uint64_t addr){
	return native_read_msr(addr);
}

/**
 * @brief WRMSR wrapper.
 * 
 * Writes to the given MSR. If it is a known MSR, mask out reserved bits into
 * a temporary, logic-OR the reserved bits into the temporary and write back
 * this temporary.
 */

static void pfcWRMSR(uint64_t addr, uint64_t newVal){
	uint64_t mask;
	
	/**
	 * For writing to MSRs, it's required to retrieve the old value of
	 * reserved bits and write them back. Things seem to blow up big-time
	 * otherwise.
	 * 
	 * Thus we retrieve a mask whose bits are set to 1 where the MSR's
	 * corresponding bits are reserved.
	 */
	
	if(     (addr >= MSR_IA32_A_PMC0               &&
	         addr <  MSR_IA32_A_PMC0+pmcGp)        ||
	        (addr >= MSR_IA32_PERFCTR0             &&
	         addr <  MSR_IA32_PERFCTR0+pmcGp)      ){
		mask =                                        ~pmcGpMask;
	}else if(addr == MSR_IA32_PERF_GLOBAL_CTRL     ){
		mask =                   ZV(pmcFf,  32) & ZV(pmcGp,   0);
	}else if(addr == MSR_IA32_PERF_GLOBAL_STATUS   ){
		return;/* RO MSR! */
	}else if(addr == MSR_IA32_PERF_GLOBAL_OVF_CTRL ){
		mask = ZV( 3,      61) & ZV(pmcFf,  32) & ZV(pmcGp,   0);
	}else if(addr == MSR_IA32_FIXED_CTR_CTRL       ){
		mask =                                    ZV(4*pmcFf, 0);
	}else if(addr >= MSR_IA32_FIXED_CTR0           &&
	         addr <  MSR_IA32_FIXED_CTR0 +pmcFf    ){
		mask =                                        ~pmcFfMask;
	}else if(addr >= MSR_IA32_PERFEVTSEL0          &&
	         addr <  MSR_IA32_PERFEVTSEL0+pmcGp    ){
		mask =                                0xFFFFFFFF00000000;
	}else if(addr == MSR_IA32_THERM_STATUS){
		mask =                                0xFFFFFFFF0780F000;
	}else if(addr == MSR_IA32_PACKAGE_THERM_STATUS){
		mask =                                0xFFFFFFFFFF80F000;
	}else if(addr == MSR_IA32_TEMPERATURE_TARGET){
		/**
		 * On i7-4700MQ,
		 * 
		 * - Bits 29-28 are undefined.
		 * - Bits 15- 8 are, in fact, defined.
		 */
		mask =                                0xFFFFFFFFF00000FF;
	}else if(addr == MSR_CORE_PERF_LIMIT_REASONS){
		mask =                                0xFFFFFFFF1A90FFFF;
	}else if(addr == MSR_IA32_ENERGY_PERF_BIAS){
		mask =                                0xFFFFFFFFFFFFFFF0;
	}else if(addr == MSR_IA32_PERF_CTL){
		mask =                                0xFFFFFFFFFFFF0000;
	}else if(addr == MSR_PEBS_FRONTEND){
		mask =                                0xFFFFFFFFFFC000E8;
	}else{
		return;/* Unknown MSR! Taking no chances! */
	}
	
	
	/**
	 * Blend new and old & Writeback
	 */
	
	newVal = (~mask&newVal) | (mask&pfcRDMSR(addr));
	native_write_msr(addr,
	                 (uint32_t)(newVal >>  0),
	                 (uint32_t)(newVal >> 32));
}

/**
 * Clamp offset+len into a given counter range.
 * 
 * Writes out the overlap's bounds through the output arguments.
 * 
 * Returns whether or not [offset+len) overlaps [rangeStart, rangeEnd)
 */

static int  pfcClampRange(int    off,
                          int    len,
                          int    rangeStart,
                          int    rangeEnd,
                          int*   pmcStart,
                          int*   pmcEnd){
	if(off+len <= rangeStart || /* If given range is fully before or     */
	   off     >= rangeEnd   || /* fully after the target range, or      */
	   len     <= 0){           /* its length is <=0, then               */
		return 0;
	}else{
		*pmcStart = off     < rangeStart ? rangeStart : off;
		*pmcEnd   = off+len > rangeEnd   ? rangeEnd   : off+len;
		return 1;
	}
}

/**
 * Check whether or not the given offset+length are sanely aligned.
 */

static int  pfcIsAligned(loff_t off, size_t len, size_t mask){
	return !((off|len) & mask);
}

/*************** END UTILITIES ***************/


/*************** COUNTER MANIPULATION ***************/
/* Ff */
void     pfcFfCntWrEnb(int i, int      v){
	uint64_t en = pfcRDMSR(MSR_IA32_PERF_GLOBAL_CTRL);
	en |= (uint64_t)!!v << (32+i);
	pfcWRMSR(MSR_IA32_PERF_GLOBAL_CTRL, en);
}
void     pfcFfCntWrCfg(int i, uint64_t c){
	/**
	 * We forbid the setting of the following bits in each 4-bit config group.
	 *     Bit 3: PMI Interrupt on counter overflow
	 * for all counters.
	 * 
	 * This corresponds to keeping only 0b0111.
	 */
	
	c  &= 0x7;
	c   = CV(pfcRDMSR(MSR_IA32_FIXED_CTR_CTRL),  4, 4*i) | BV(c, 4, 4*i);
	pfcWRMSR(MSR_IA32_FIXED_CTR_CTRL, c);
}
uint64_t pfcFfCntRdCfg(int i            ){
	uint64_t cfg = pfcRDMSR(MSR_IA32_FIXED_CTR_CTRL);
	cfg >>= 4*i;
	return cfg & 0xF;
}
void     pfcFfCntWrVal(int i, uint64_t v){
	pfcWRMSR(MSR_IA32_FIXED_CTR0+i, v);
}
uint64_t pfcFfCntRdVal(int i            ){
	return pfcRDMSR(MSR_IA32_FIXED_CTR0+i);
}

/* Gp */
void     pfcGpCntWrEnb(int i, int      v){
	uint64_t en = pfcRDMSR(MSR_IA32_PERF_GLOBAL_CTRL);
	en |= (uint64_t)!!v << ( 0+i);
	pfcWRMSR(MSR_IA32_PERF_GLOBAL_CTRL, en);
}
void     pfcGpCntWrCfg(int i, uint64_t c){
	uint64_t evtNum, umask;
	
	/**
	 * We forbid the setting of the following bits in each PERFEVTSELx MSR:
	 *     Bit 20: APIC Interrupt Enable on overflow bit
	 *     Bit 19: Pin Control bit
	 * for all counters.
	 */
	
	c &= ~0x0000000000180000ULL;
	
	/**
	 * For odd reasons, certain cache statistics can only be collected on
	 * certain counters.
	 */
	
	evtNum = (c >>  0) & 0xFF;
	umask  = (c >>  8) & 0xFF;
	
	if((evtNum == 0x48) ||                                   /* l1d_pend_miss */
	   (evtNum == 0xA3 && (umask == 0x08 || umask == 0x0C))){/* cycle_activity.l1d_pending */
		if(i != 2){
			c = 0;/* Disable. */
		}
	}
	
	if(evtNum == 0xC0 && umask == 0x01){
		if(i != 1){
			c = 0;/* Disable. */
		}
	}
	
	
	pfcWRMSR(MSR_IA32_PERFEVTSEL0+i, c);
}
uint64_t pfcGpCntRdCfg(int i            ){
	return pfcRDMSR(MSR_IA32_PERFEVTSEL0+i);
}
void     pfcGpCntWrVal(int i, uint64_t v){
	if (fullWidthWrites){
		pfcWRMSR(MSR_IA32_A_PMC0+i, v);
	}else{
		pfcWRMSR(MSR_IA32_PERFCTR0+i, v);
	}
}
uint64_t pfcGpCntRdVal(int i            ){
	return pfcRDMSR(MSR_IA32_PERFCTR0+i);
}
/*************** END COUNTER MANIPULATION ***************/


/**************** SYSFS ATTRIBUTES ****************/

/**
 * Read configuration.
 * 
 * Returns the configuration of the selected counters, one 64-bit word per
 * counter, with the Ff counters first and the Gp counters last.
 * 
 * For Ff counters, their 4-bit field from IA32_FIXED_CTR_CTRL is read.
 * For Gp counters, their respective IA32_PERFEVTSEL is read.
 * 
 * @return Bytes of configuration data read
 */

static ssize_t pfcCfgRd (struct file*          f,
                         struct kobject*       kobj,
                         struct bin_attribute* binattr,
                         char*                 buf,
                         loff_t                off,
                         size_t                len){
	int pmcStart, pmcEnd, i, j;
	uint64_t* buf64 = (uint64_t*)buf;
	
	/* Check access is reasonable. */
	if(!pfcIsAligned(off, len, 0x7) || off<0 || len<0){
		return -1;
	}
	
	/* Read relevant MSRs */
	j=0;
	if(pfcClampRange(off>>3, len>>3, pmcStartFf, pmcEndFf, &pmcStart, &pmcEnd)){
		pmcStart -= pmcStartFf;
		pmcEnd   -= pmcStartFf;
		
		for(i=pmcStart;i<pmcEnd;i++,j++){
			buf64[j] = pfcFfCntRdCfg(i);
		}
	}
	if(pfcClampRange(off>>3, len>>3, pmcStartGp, pmcEndGp, &pmcStart, &pmcEnd)){
		pmcStart -= pmcStartGp;
		pmcEnd   -= pmcStartGp;
		
		for(i=pmcStart;i<pmcEnd;i++,j++){
			buf64[j] = pfcGpCntRdCfg(i);
		}
	}
	
	/* Report read data */
	return j*sizeof(uint64_t);
}

/**
 * Write configuration.
 * 
 * Sets the configuration of the selected counters, given one 64-bit word per
 * counter, with the Ff counters first and the Gp counters last.
 * 
 * Disables and leaves disabled all selected counters.
 * 
 * @return Bytes of configuration data written
 */

static ssize_t pfcCfgWr(struct file*          f,
                        struct kobject*       kobj,
                        struct bin_attribute* binattr,
                        char*                 buf,
                        loff_t                off,
                        size_t                len){
	int pmcStart, pmcEnd, i, j;
	uint64_t* buf64 = (uint64_t*)buf;
	
	/* Check access is reasonable. */
	if(!pfcIsAligned(off, len, 0x7) || off<0 || len<0){
		return -1;
	}
	
	/* Write relevant MSRs */
	j=0;
	if(pfcClampRange(off>>3, len>>3, pmcStartFf, pmcEndFf, &pmcStart, &pmcEnd)){
		pmcStart -= pmcStartFf;
		pmcEnd   -= pmcStartFf;
		
		for(i=pmcStart;i<pmcEnd;i++,j++){
			pfcFfCntWrEnb(i, 0);
			pfcFfCntWrCfg(i, buf64[j]);
			
			if(pfcFfCntRdCfg(i) & 0x2){
				pfcFfCntWrEnb(i, 1);
			}
		}
	}
	if(pfcClampRange(off>>3, len>>3, pmcStartGp, pmcEndGp, &pmcStart, &pmcEnd)){
		pmcStart -= pmcStartGp;
		pmcEnd   -= pmcStartGp;
		
		for(i=pmcStart;i<pmcEnd;i++,j++){
			pfcGpCntWrEnb(i, 0);
			pfcGpCntWrCfg(i, buf64[j]);
			
			if(pfcGpCntRdCfg(i) & 0x00400000){
				pfcGpCntWrEnb(i, 1);
			}
		}
	}
	
	/* Report written data */
	return j*sizeof(uint64_t);
}

/**
 * Read masks.
 * 
 * Returns the mask of the selected counters, one 64-bit word per
 * counter, with the Ff counters first and the Gp counters last.
 * 
 * For Ff counters, pmcMaskFf is read.
 * For Gp counters, pmcMaskGp is read.
 * 
 * @return Bytes of mask data read
 */

static ssize_t pfcMskRd (struct file*          f,
                         struct kobject*       kobj,
                         struct bin_attribute* binattr,
                         char*                 buf,
                         loff_t                off,
                         size_t                len){
	int pmcStart, pmcEnd, i, j;
	uint64_t* buf64 = (uint64_t*)buf;
	
	/* Check access is reasonable. */
	if(!pfcIsAligned(off, len, 0x7) || off<0 || len<0){
		return -1;
	}
	
	/* Read relevant MSRs */
	j=0;
	if(pfcClampRange(off>>3, len>>3, pmcStartFf, pmcEndFf, &pmcStart, &pmcEnd)){
		pmcStart -= pmcStartFf;
		pmcEnd   -= pmcStartFf;
		
		for(i=pmcStart;i<pmcEnd;i++,j++){
			buf64[j] = pmcFfMask;
		}
	}
	if(pfcClampRange(off>>3, len>>3, pmcStartGp, pmcEndGp, &pmcStart, &pmcEnd)){
		pmcStart -= pmcStartGp;
		pmcEnd   -= pmcStartGp;
		
		for(i=pmcStart;i<pmcEnd;i++,j++){
			buf64[j] = pmcGpMask;
		}
	}
	
	/* Report read data */
	return j*sizeof(uint64_t);
}

/**
 * Reads counts.
 * 
 * Returns the counts of the selected counters, one 64-bit word per counter,
 * with the Ff counters first and the Gp counters last.
 * 
 * Enables and starts the selected counters.
 * 
 * @return Bytes of counter data read
 */

static ssize_t pfcCntRd (struct file*          f,
                         struct kobject*       kobj,
                         struct bin_attribute* binattr,
                         char*                 buf,
                         loff_t                off,
                         size_t                len){
	int pmcStart, pmcEnd, i, j;
	uint64_t* buf64 = (uint64_t*)buf;
	
	/* Check access is reasonable. */
	if(!pfcIsAligned(off, len, 0x7) || off<0 || len<0){
		return -1;
	}
	
	/* Read relevant MSRs */
	j=0;
	if(pfcClampRange(off>>3, len>>3, pmcStartFf, pmcEndFf, &pmcStart, &pmcEnd)){
		pmcStart -= pmcStartFf;
		pmcEnd   -= pmcStartFf;
		
		for(i=pmcStart;i<pmcEnd;i++,j++){
			buf64[j] = pfcFfCntRdVal(i);
		}
	}
	if(pfcClampRange(off>>3, len>>3, pmcStartGp, pmcEndGp, &pmcStart, &pmcEnd)){
		pmcStart -= pmcStartGp;
		pmcEnd   -= pmcStartGp;
		
		for(i=pmcStart;i<pmcEnd;i++,j++){
			buf64[j] = pfcGpCntRdVal(i);
		}
	}
	
	/* Report read data */
	return j*sizeof(uint64_t);
}

/**
 * Write counts.
 * 
 * Sets the value of the selected counters, given one 64-bit word per
 * counter, with the Ff counters first and the Gp counters last.
 * 
 * Disables and leaves disabled all selected counters.
 * 
 * @return Bytes of counter data written
 */

static ssize_t pfcCntWr(struct file*          f,
                        struct kobject*       kobj,
                        struct bin_attribute* binattr,
                        char*                 buf,
                        loff_t                off,
                        size_t                len){
	int pmcStart, pmcEnd, i, j;
	uint64_t* buf64 = (uint64_t*)buf;
	
	/* Check access is reasonable. */
	if(!pfcIsAligned(off, len, 0x7) || off<0 || len<0){
		return -1;
	}
	
	/* Write relevant MSRs */
	j=0;
	if(pfcClampRange(off>>3, len>>3, pmcStartFf, pmcEndFf, &pmcStart, &pmcEnd)){
		pmcStart -= pmcStartFf;
		pmcEnd   -= pmcStartFf;
		
		for(i=pmcStart;i<pmcEnd;i++,j++){
			pfcFfCntWrVal(i, buf64[j]);
		}
	}
	if(pfcClampRange(off>>3, len>>3, pmcStartGp, pmcEndGp, &pmcStart, &pmcEnd)){
		pmcStart -= pmcStartGp;
		pmcEnd   -= pmcStartGp;
		
		for(i=pmcStart;i<pmcEnd;i++,j++){
			pfcGpCntWrVal(i, buf64[j]);
		}
	}
	
	/* Report written data */
	return j*sizeof(uint64_t);
}

/**
 * Read MSRs from userland.
 * 
 * EXTREMELY NASTY AND VERY VERY DANGEROUS HACK. CAN ****EASILY**** TURN INTO
 * USERLAND-EXPLOITABLE #GP FAULT-GENERATOR DENIAL-OF-SERVICE.
 * 
 * @return MSR bytes read
 */

static ssize_t pfcMsrRd (struct file*          f,
                         struct kobject*       kobj,
                         struct bin_attribute* binattr,
                         char*                 buf,
                         loff_t                off,
                         size_t                len){
	uint64_t v;
	
	if(len != 8){
		return -1;
	}
	
	switch(off){
		case MSR_CORE_PERF_LIMIT_REASONS:
			v               = pfcRDMSR(off);
			*(uint64_t*)buf = v;
			pfcWRMSR(off, 0);/* Clear all writable log bits. */
		return len;
		case MSR_IA32_PERF_STATUS:
		case MSR_IA32_PERF_CTL:
		case MSR_IA32_MISC_ENABLE:
		case MSR_PLATFORM_INFO:
		case MSR_IA32_TEMPERATURE_TARGET:
			*(uint64_t*)buf = pfcRDMSR(off);
		return len;
		case MSR_IA32_PACKAGE_THERM_STATUS:
		case MSR_IA32_PACKAGE_THERM_INTERRUPT:
			if(leaf6.a & (1ULL<<6)){
				*(uint64_t*)buf = pfcRDMSR(off);
			}else{
				*(uint64_t*)buf = 0;
				return -1;
			}
		return len;
		case MSR_IA32_ENERGY_PERF_BIAS:
			if(leaf6.c & (1ULL<<3)){
				*(uint64_t*)buf = pfcRDMSR(off);
			}else{
				*(uint64_t*)buf = 0;
				return -1;
			}
		return len;
		case MSR_PEBS_FRONTEND:
			/**
			 * It's technically only available on some Skylake+ processors, but
			 * it's almost impossible to keep a pretty, up-to-date list of
			 * CPUID families that are compatible.
			 * 
			 * Trust that the user knows what he's doing. The Doxygen for this
			 * function does warn about the extreme lack of safety.
			 */
			
			*(uint64_t*)buf = pfcRDMSR(off);
		return len;
		default:
			*(uint64_t*)buf = 0;
		return -1;
	}
}

static ssize_t pfcVerboseRd(struct kobject* kobj,
                            struct kobj_attribute* attr,
                            char* buf) {
	strcpy(buf, verbose ? "1" : "0");
	return 1;
}

static ssize_t pfcCR4PceRd(struct kobject* kobj,
                           struct kobj_attribute* attr,
                           char* buf) {
	strcpy(buf, (native_read_cr4() & 0x00000100L) ? "1" : "0");
	return 1;
}

static ssize_t pfcVerboseWr(struct kobject* kobj,
                            struct kobj_attribute* attr,
                            const char* buf,
                            size_t count) {
	if (count == 1 || (count == 2 && buf[1] == '\n')) {
		char value = *buf;
		if (value == '0' || value == '1') {
			verbose = (value == '1');
			printk(KERN_INFO "pfc: verbose mode %s\n", verbose ? "enabled" : "disabled");
			return count;
		}
	}

	printk(KERN_NOTICE "pfc: bad value written to verbose, expected 0 or 1 but got '%s' (sz=%zu)\n", buf, count);
	return -1;
}





/**************** INIT CODE ****************/

/**
 * Read CPUID to identify # of fixed-function and general-purpose PMCs, as well
 * as quite a few other things.
 */

static int  pfcInitCPUID(void){
	/* Perform all CPUID reads we will need. */
	cpuid_count(0x00000000, 0, &leaf0.a,        &leaf0.b,        &leaf0.c,        &leaf0.d);
	cpuid_count(0x00000001, 0, &leaf1.a,        &leaf1.b,        &leaf1.c,        &leaf1.d);
	cpuid_count(0x00000006, 0, &leaf6.a,        &leaf6.b,        &leaf6.c,        &leaf6.d);
	cpuid_count(0x0000000A, 0, &leafA.a,        &leafA.b,        &leafA.c,        &leafA.d);
	cpuid_count(0x80000000, 0, &leaf80000000.a, &leaf80000000.b, &leaf80000000.c, &leaf80000000.d);
	cpuid_count(0x80000001, 0, &leaf80000001.a, &leaf80000001.b, &leaf80000001.c, &leaf80000001.d);
	cpuid_count(0x80000002, 0, &leaf80000002.a, &leaf80000002.b, &leaf80000002.c, &leaf80000002.d);
	cpuid_count(0x80000003, 0, &leaf80000003.a, &leaf80000003.b, &leaf80000003.c, &leaf80000003.d);
	cpuid_count(0x80000004, 0, &leaf80000004.a, &leaf80000004.b, &leaf80000004.c, &leaf80000004.d);
	
	family          = (leaf1.a >>  8) & 0x0F;
	model           = (leaf1.a >>  4) & 0x0F;
	stepping        = (leaf1.a >>  0) & 0x0F;
	exmodel         = (leaf1.a >> 16) & 0x0F;
	exfamily        = (leaf1.a >> 20) & 0xFF;
	dispFamily      = (        family != 0x0F        ) ? family : (family+exfamily);
	dispModel       = (family == 0x06 || family==0x0F) ? (exmodel<<4|model) : model;
	
	maxLeaf         = leaf0.a;
	maxExtendedLeaf = leaf80000000.a;
	memcpy(&procBrandString[ 0], (const char*)&leaf80000002, 16);
	memcpy(&procBrandString[16], (const char*)&leaf80000003, 16);
	memcpy(&procBrandString[32], (const char*)&leaf80000004, 16);
	if(maxExtendedLeaf < 4){
		strcpy(procBrandString, "(unknown)");
	}
	
	
	/* And dump the CPUID info to the ring buffer for debug purposes. */
	if(maxLeaf >= 1){
		printk(KERN_INFO "pfc: Kernel Module loading on processor %s (Family %u (%X), Model %u (%03X), Stepping %u (%X))\n",
		       procBrandString, dispFamily, dispFamily, dispModel, dispModel, stepping, stepping);
	}else{
		printk(KERN_INFO "pfc: Kernel Module loading on processor %s\n", procBrandString);
	}
	printk(KERN_INFO "pfc: cpuid.0x0.0x0:        EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
	       leaf0.a,        leaf0.b,        leaf0.c,        leaf0.d);
	printk(KERN_INFO "pfc: cpuid.0x1.0x0:        EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
	       leaf1.a,        leaf1.b,        leaf1.c,        leaf1.d);
	printk(KERN_INFO "pfc: cpuid.0x6.0x0:        EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
	       leaf6.a,        leaf6.b,        leaf6.c,        leaf6.d);
	printk(KERN_INFO "pfc: cpuid.0xA.0x0:        EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
	       leafA.a,        leafA.b,        leafA.c,        leafA.d);
	printk(KERN_INFO "pfc: cpuid.0x80000000.0x0: EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
	       leaf80000000.a, leaf80000000.b, leaf80000000.c, leaf80000000.d);
	printk(KERN_INFO "pfc: cpuid.0x80000001.0x0: EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
	       leaf80000001.a, leaf80000001.b, leaf80000001.c, leaf80000001.d);
	printk(KERN_INFO "pfc: cpuid.0x80000002.0x0: EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
	       leaf80000002.a, leaf80000002.b, leaf80000002.c, leaf80000002.d);
	printk(KERN_INFO "pfc: cpuid.0x80000003.0x0: EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
	       leaf80000003.a, leaf80000003.b, leaf80000003.c, leaf80000003.d);
	printk(KERN_INFO "pfc: cpuid.0x80000004.0x0: EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
	       leaf80000004.a, leaf80000004.b, leaf80000004.c, leaf80000004.d);
	
	
	/* Begin sanity checks. */
	if(maxLeaf < 0xA){
		printk(KERN_ERR  "pfc: ERROR: Processor too old!\n");
		return -1;
	}
	
	
	/* Check that CPU has performance monitoring. */
	if(((leaf1.c>>15) & 1) == 0){
		printk(KERN_ERR  "pfc: ERROR: Processor does not have Perfmon and Debug Capability!\n");
		return -1;
	}
	
	
	/**
	 * Inform ourselves about PMC support.
	 *
	 * PMC information is gotten by CPUID.EAX = 0xA.
	 * 
	 *     PerfMon architecture version is in EAX[ 7: 0].
	 *     #Gp PMCs                     is in EAX[15: 8]
	 *     Gp bitwidth                  is in EAX[23:16]
	 *     #Ff PMCs                     is in EDX[ 4: 0] if PMArchVer > 1.
	 *     Ff bitwidth                  is in EDX[12: 5] if PMArchVer > 1.
	 */
	
	pmcArchVer = (leafA.a >>  0) & 0xFF;
	if(pmcArchVer < 3 || pmcArchVer > 4){
		printk(KERN_INFO "pfc: ERROR: Unsupported performance monitoring architecture version %d, only 3 or 4 supported!\n", pmcArchVer);
		return -1;
	}
	
	fullWidthWrites = (pfcRDMSR(MSR_IA32_PERF_CAPABILITIES) >> 13) & 1;
	
	pmcGp         = (leafA.a >>  8) & 0xFF;
	pmcGpBitwidth = (leafA.a >> 16) & 0xFF;
	if (fullWidthWrites){
		pmcGpMask     = OV(pmcGpBitwidth,0);
	}else{
		pmcGpMask     = OV(32,0);
	}
	
	pmcFf         = (leafA.d >>  0) & 0x1F;
	pmcFfBitwidth = (leafA.d >>  5) & 0xFF;
	pmcFfMask     = OV(pmcFfBitwidth,0);
	
	
	/* Save bounds. */
	pmcStartFf = 0;
	pmcEndFf   = pmcFf;
	pmcStartGp = pmcFf;
	pmcEndGp   = pmcFf+pmcGp;
	
	
	/* Dump out this data */
	printk(KERN_INFO "pfc: PM Arch Version:      %d\n", pmcArchVer);
	if(pmcFf + pmcGp > MAXPMC){
		printk(KERN_INFO "pfc: More than %d PMCs found! Clamping to %d.\n",
		       MAXPMC, MAXPMC);
		pmcGp = pmcGp>MAXPMC       ?       MAXPMC : pmcGp;
		pmcFf = pmcGp+pmcFf>MAXPMC ? MAXPMC-pmcGp : pmcFf;
	}
	printk(KERN_INFO "pfc: Fixed-function  PMCs: %d\tMask %016llx (%d bits)\n", pmcFf, pmcFfMask, pmcFfBitwidth);
	printk(KERN_INFO "pfc: General-purpose PMCs: %d\tMask %016llx (%d bits)\n", pmcGp, pmcGpMask, pmcGpBitwidth);
	
	
	return 0;
}

/**
 * Initialize all counters.
 * 
 * For each counter, therefore,
 * 1. Globally disable it
 * 2. Deconfigure it
 * 3. Zero it
 * 4. Clear its interrupt flags
 */

static void pfcInitCounters(void* unused){
	int i;
	(void)unused;
	
	pfcWRMSR(MSR_IA32_PERF_GLOBAL_CTRL,      0);
	pfcWRMSR(MSR_IA32_FIXED_CTR_CTRL,        0);
	for(i=0;i<pmcFf;i++){
		pfcWRMSR(MSR_IA32_FIXED_CTR0  + i,       0);
	}
	for(i=0;i<pmcGp;i++){
		pfcWRMSR(MSR_IA32_PERFEVTSEL0 + i,       0);
		pfcWRMSR(MSR_IA32_PERFCTR0    + i,       0);
	}
	pfcWRMSR(MSR_IA32_PERF_GLOBAL_OVF_CTRL, ~0);
}

/**
 * Load module.
 * 
 * @return 0 if module load successful, non-0 otherwise.
 */

static int __init pfcInit(void){
	int ret;
	
	
	if(pfcInitCPUID() != 0){
		goto fail;
	}
	if(!(native_read_cr4() & 0x00000100L)){
		printk(KERN_INFO "pfc: ERROR: User-space RDPMC not enabled!\n");
		printk(KERN_INFO "pfc: ERROR: Make sure to execute\n");
		printk(KERN_INFO "pfc: ERROR:     echo 2 > /sys/bus/event_source/devices/cpu/rdpmc\n");
		printk(KERN_INFO "pfc: ERROR: as root. This enables ultra-low-overhead, userspace\n");
		printk(KERN_INFO "pfc: ERROR: access to the performance counters.\n");
		goto fail;
	}
	
	on_each_cpu(pfcInitCounters, NULL, 1);
	
	/**
	 * The chmods that follow are necessary because sysfs_create_group()
	 * doesn't appreciate world-writeable files, but we WANT that for the
	 * convenience of our users.
	 */
	
	ret  = sysfs_create_group((struct kobject*)&THIS_MODULE->mkobj,
	                          &PFC_ATTR_GRP);
	ret |= sysfs_chmod_file  ((struct kobject*)  &THIS_MODULE->mkobj,
	                          (struct attribute*)&PFC_ATTR_config,  0666);
	ret |= sysfs_chmod_file  ((struct kobject*)  &THIS_MODULE->mkobj,
	                          (struct attribute*)&PFC_ATTR_masks,   0444);
	ret |= sysfs_chmod_file  ((struct kobject*)  &THIS_MODULE->mkobj,
	                          (struct attribute*)&PFC_ATTR_counts,  0666);
	ret |= sysfs_chmod_file  ((struct kobject*)  &THIS_MODULE->mkobj,
	                          (struct attribute*)&PFC_ATTR_msr,     0444);
	ret |= sysfs_chmod_file  ((struct kobject*)  &THIS_MODULE->mkobj,
		                      (struct attribute*)&PFC_ATTR_verbose, 0666);
	ret |= sysfs_chmod_file  ((struct kobject*)  &THIS_MODULE->mkobj,
	                          (struct attribute*)&PFC_ATTR_cr4pce,  0444);
	if(ret != 0){
		printk(KERN_INFO "pfc: ERROR: Failed to create sysfs attributes.\n");
		goto lateFail;
	}
	
	
	printk(KERN_INFO "pfc: Module pfc loaded successfully. Make sure to execute\n");
	printk(KERN_INFO "pfc:     modprobe -ar iTCO_wdt iTCO_vendor_support\n");
	printk(KERN_INFO "pfc:     echo 0 > /proc/sys/kernel/nmi_watchdog\n");
	printk(KERN_INFO "pfc: and blacklist iTCO_vendor_support and iTCO_wdt, since the CR4.PCE register\n");
	printk(KERN_INFO "pfc: initialization is periodically undone by an unknown agent.\n");
	return 0;
	
	
	lateFail:
	pfcExit();
	fail:
	printk(KERN_INFO "pfc: ERROR: Failed to load module pfc.\n");
	return -1;
}

/**
 * Exit module.
 */

static void        pfcExit(void){
	printk(KERN_INFO "pfc: Module exiting...\n");
	
	on_each_cpu(pfcInitCounters, NULL, 1);
	sysfs_remove_group((struct kobject*)&THIS_MODULE->mkobj,
	                   &PFC_ATTR_GRP);
	
	printk(KERN_INFO "pfc: Module exited.\n");
}



/* Module magic */
module_init(pfcInit);
module_exit(pfcExit);
MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("");
MODULE_DESCRIPTION("Grants ***EXTREMELY UNSAFE*** access to the Intel Performance Monitor Counters (PMC).");



/* Notes */

/**
 * Possibly required boot args:
 *     nmi_watchdog=0 modprobe.blacklist=iTCO_wdt,iTCO_vendor_support
 * 
 * Otherwise, IA32_FIXED_CTR1 is monopolised by !@#$ NMI watchdog crapware. Even
 * so, it seems that something gratuitously, though rarely, sets IA32_PMC0 to
 * 0xFFFF every so often.
 */

