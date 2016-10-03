/* Include Guards */
#ifndef LIBPFC_H
#define LIBPFC_H


/* Includes */
#include <stdint.h>



/* Data types */
typedef uint64_t  PFC_CFG;
typedef int64_t   PFC_CNT;



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
 * Pins calling thread to given core
 */

void      pfcPinThread     (int core);

/**
 * Read and write the configurations and values of the n counters starting at
 * counter k.
 */

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

/**
 * The PFCSTART macro takes a single pointer to a 7-element array of 64-bit
 * integers and *subtracts* out of them the start counter values.
 */

#define PFCSTART(b)                             \
asm volatile(                                   \
"\n\t.intel_syntax noprefix                  "  \
"\n\tlfence                                  "  \
"\n\tmov      rcx, 0x40000000                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tsub      qword ptr [%0 +   0], rdx      "  \
"\n\tmov      rcx, 0x40000001                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tsub      qword ptr [%0 +   8], rdx      "  \
"\n\tmov      rcx, 0x40000002                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tsub      qword ptr [%0 +  16], rdx      "  \
"\n\tmov      rcx, 0x00000000                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tsub      qword ptr [%0 +  24], rdx      "  \
"\n\tmov      rcx, 0x00000001                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tsub      qword ptr [%0 +  32], rdx      "  \
"\n\tmov      rcx, 0x00000002                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tsub      qword ptr [%0 +  40], rdx      "  \
"\n\tmov      rcx, 0x00000003                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tsub      qword ptr [%0 +  48], rdx      "  \
"\n\tlfence                                  "  \
"\n\t.att_syntax noprefix                    "  \
:        /* Outputs */                          \
: "r"((b)) /* Inputs */                         \
: "memory", "rax", "rcx", "rdx"                 \
)

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

#define PFCEND(b)                               \
asm volatile(                                   \
"\n\t.intel_syntax noprefix                  "  \
"\n\tlfence                                  "  \
"\n\tmov      rcx, 0x40000000                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tadd      qword ptr [%0 +   0], rdx      "  \
"\n\tmov      rcx, 0x40000001                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tadd      qword ptr [%0 +   8], rdx      "  \
"\n\tmov      rcx, 0x40000002                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tadd      qword ptr [%0 +  16], rdx      "  \
"\n\tmov      rcx, 0x00000000                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tadd      qword ptr [%0 +  24], rdx      "  \
"\n\tmov      rcx, 0x00000001                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tadd      qword ptr [%0 +  32], rdx      "  \
"\n\tmov      rcx, 0x00000002                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tadd      qword ptr [%0 +  40], rdx      "  \
"\n\tmov      rcx, 0x00000003                "  \
"\n\trdpmc                                   "  \
"\n\tshl      rdx, 32                        "  \
"\n\tor       rdx, rax                       "  \
"\n\tadd      qword ptr [%0 +  48], rdx      "  \
"\n\tlfence                                  "  \
"\n\t.att_syntax noprefix                    "  \
:        /* Outputs */                          \
: "r"((b)) /* Inputs */                         \
: "memory", "rax", "rcx", "rdx"                 \
)

/**
 * Remove mul times from b the counter bias due to PFCSTART/PFCEND.
 */

void      pfcRemoveBias     (PFC_CNT* b, int64_t mul);



/* End Extern "C" Guard */
#ifdef __cplusplus
}
#endif

#endif // LIBPFC_H

