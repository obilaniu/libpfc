/* Include Guards */
#ifndef LIBPFC_H
#define LIBPFC_H


/* Includes */
#include <stdint.h>



/* Data types */
typedef uint64_t  PFC_CFG;
typedef int64_t   PFC_CNT;

// the next three definitions are the positions of the three fixed counters within
// the PFC_CNT array
// See also "Table 18-2. Association of Fixed-Function Performance Counters with Architectural Performance Events"
// in the Intel SDM Vol 3.
#define PFC_FIXEDCNT_INSTRUCTIONS_RETIRED 0      /* INST_RETIRED.ANY */
#define PFC_FIXEDCNT_CPU_CLK_UNHALTED     1      /* CPU_CLK_UNHALTED.THREAD */
#define PFC_FIXEDCNT_CPU_CLK_REF_TSC      2      /* CPU_CLK_UNHALTED.REF_TSC */


/* Extern "C" Guard */
#ifdef __cplusplus
extern "C"{
#endif

/* Functions */

/**
 * Initialize and finalize library.
 * 
 * Returns 0 on success and non-zero on failure.
 */

int       pfcInit          (void);
void      pfcFini          (void);

/**
 * Pins calling thread to given core, returns zero on success, non-zero otherwise.
 */
int      pfcPinThread     (int core);

/**
 * Read and write the configurations and values of the n counters starting at
 * counter k.
 */

/* Writes n configuration values from cfg, starting at counter k. Returns 0 on success or an error code otherwise. */
int       pfcWrCfgs        (int k, int n, const PFC_CFG* cfg);
int       pfcRdCfgs        (int k, int n,       PFC_CFG* cfg);
int       pfcWrCnts        (int k, int n, const PFC_CNT* cnt);
int       pfcRdCnts        (int k, int n,       PFC_CNT* cnt);

/**
 * Translate argument to configuration.
 */

PFC_CFG   pfcParseCfg      (const char* s);

/**
 * Dump out available events
 */

void      pfcDumpEvts      (void);


/*********************
 *****  MACROS   *****
 *********************/

#define _pfc_asm_code_cnt_read_(op, rcx, off)   \
"\n\tmov      rcx, "#rcx"                    "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\t"#op"    qword ptr [%0 +   "#off"], rdx "

#define _pfc_asm_code_(op)                      \
"\n\tlfence                                  "  \
_pfc_asm_code_cnt_read_(op, 0x40000000,  0)     \
_pfc_asm_code_cnt_read_(op, 0x40000001,  8)     \
_pfc_asm_code_cnt_read_(op, 0x40000002, 16)     \
_pfc_asm_code_cnt_read_(op, 0x00000000, 24)     \
_pfc_asm_code_cnt_read_(op, 0x00000001, 32)     \
_pfc_asm_code_cnt_read_(op, 0x00000002, 40)     \
_pfc_asm_code_cnt_read_(op, 0x00000003, 48)     \
"\n\tlfence                                  "  \

#define _pfc_macro_(b, op)                      \
asm volatile(                                   \
"\n\t.intel_syntax noprefix                  "  \
_pfc_asm_code_(op)                              \
"\n\t.att_syntax noprefix                    "  \
:        /* Outputs */                          \
: "r"((b)) /* Inputs */                         \
: "memory", "rax", "rcx", "rdx"                 \
)

/**
 * The PFCSTART macro takes a single pointer to a 7-element array of 64-bit
 * integers and *subtracts* out of them the start counter values.
 */

#define PFCSTART(b) _pfc_macro_((b), sub)

/**
 * The PFCEND macro is *exactly* the same as the PFCSTART macro, down to the
 * size and scheduling of every instruction, *except* that it *adds* the end
 * counter-values into the array.
 * 
 * The end result is buffer[i] = (buffer[i] - start[i]) + end[i], which amounts
 * to the same thing as buffer[i] += (end[i] - start[i])!
 * 
 * So, if the buffer is initialized beforehand to 0, then at the end it will
 * contain a biased count of the events currently selected on the PMCs. The
 * bias comes from the 37 instructions of PFCSTART and PFCEND; This bias is
 * roughly constant and can be estimated and removed by calling
 * pfcRemoveBias(b, 1) with no intervening change to the counters.
 */

#define PFCEND(b)   _pfc_macro_((b), add)

/**
 * Remove mul times from b the counter bias due to PFCSTART/PFCEND.
 */

void      pfcRemoveBias     (PFC_CNT* b, int64_t mul);

/**
 * Return a string representation of an error code such as those returned by pfcInit
 */
const char *pfcErrorString(int err);


/* End Extern "C" Guard */
#ifdef __cplusplus
}
#endif

#endif // LIBPFC_H

