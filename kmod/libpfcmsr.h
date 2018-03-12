/* Include Guards */
#ifndef LIBPFCMSR_H
#define LIBPFCMSR_H



/**
 * MSRs we know of, sorted by offset.
 */

#ifndef MSR_PLATFORM_INFO
#define MSR_PLATFORM_INFO                  0x0CE
#endif
#ifndef MSR_IA32_PERFEVTSEL0
#define MSR_IA32_PERFEVTSEL0               0x186
#endif
#ifndef MSR_IA32_PERF_STATUS
#define MSR_IA32_PERF_STATUS               0x198
#endif
#ifndef MSR_IA32_PERF_CTL
#define MSR_IA32_PERF_CTL                  0x199
#endif
#ifndef MSR_IA32_CLOCK_MODULATION
#define MSR_IA32_CLOCK_MODULATION          0x19A
#endif
#ifndef MSR_IA32_THERM_INTERRUPT
#define MSR_IA32_THERM_INTERRUPT           0x19B
#endif
#ifndef MSR_IA32_THERM_STATUS
#define MSR_IA32_THERM_STATUS              0x19C
#endif
#ifndef MSR_IA32_MISC_ENABLE
#define MSR_IA32_MISC_ENABLE               0x1A0
#endif
#ifndef MSR_IA32_TEMPERATURE_TARGET
#define MSR_IA32_TEMPERATURE_TARGET        0x1A2
#endif
#ifndef MSR_IA32_ENERGY_PERF_BIAS
#define MSR_IA32_ENERGY_PERF_BIAS          0x1B0
#endif
#ifndef MSR_IA32_PACKAGE_THERM_STATUS
#define MSR_IA32_PACKAGE_THERM_STATUS      0x1B1
#endif
#ifndef MSR_IA32_PACKAGE_THERM_INTERRUPT
#define MSR_IA32_PACKAGE_THERM_INTERRUPT   0x1B2
#endif
#ifndef MSR_IA32_FIXED_CTR0
#define MSR_IA32_FIXED_CTR0                0x309
#endif
#ifndef MSR_IA32_PERF_CAPABILITIES
#define MSR_IA32_PERF_CAPABILITIES         0x345
#endif
#ifndef MSR_IA32_FIXED_CTR_CTRL
#define MSR_IA32_FIXED_CTR_CTRL            0x38D
#endif
#ifndef MSR_IA32_PERF_GLOBAL_STATUS
#define MSR_IA32_PERF_GLOBAL_STATUS        0x38E
#endif
#ifndef MSR_IA32_PERF_GLOBAL_CTRL
#define MSR_IA32_PERF_GLOBAL_CTRL          0x38F
#endif
#ifndef MSR_IA32_PERF_GLOBAL_OVF_CTRL
#define MSR_IA32_PERF_GLOBAL_OVF_CTRL      0x390
#endif
#ifndef MSR_PEBS_FRONTEND
#define MSR_PEBS_FRONTEND                  0x3F7
#endif
#ifndef MSR_IA32_A_PMC0
#define MSR_IA32_A_PMC0                    0x4C1
#endif
#ifndef MSR_CORE_PERF_LIMIT_REASONS
#define MSR_CORE_PERF_LIMIT_REASONS        0x690
#endif
#ifndef MSR_IA32_PKG_HDC_CTL
#define MSR_IA32_PKG_HDC_CTL               0xDB0
#endif
#ifndef MSR_IA32_PM_CTL1
#define MSR_IA32_PM_CTL1                   0xDB1
#endif
#ifndef MSR_IA32_THREAD_STALL
#define MSR_IA32_THREAD_STALL              0xDB2
#endif


/* Notes */

/** 186+x IA32_PERFEVTSELx           -  Performance Event Selection, ArchPerfMon v3
 * 
 *                     /63/60 /56     /48     /40     /32     /24     /16     /08     /00
 *                    {................................################################}
 *                                                     |      ||||||||||      ||      |
 *     Counter Mask -----------------------------------^^^^^^^^|||||||||      ||      |
 *     Invert Counter Mask ------------------------------------^||||||||      ||      |
 *     Enable Counter ------------------------------------------^|||||||      ||      |
 *     AnyThread ------------------------------------------------^||||||      ||      |
 *     APIC Interrupt Enable -------------------------------------^|||||      ||      |
 *     Pin Control ------------------------------------------------^||||      ||      |
 *     Edge Detect -------------------------------------------------^|||      ||      |
 *     Operating System Mode ----------------------------------------^||      ||      |
 *     User Mode -----------------------------------------------------^|      ||      |
 *     Unit Mask (UMASK) ----------------------------------------------^^^^^^^^|      |
 *     Event Select -----------------------------------------------------------^^^^^^^^
 */
/** 309+x IA32_FIXED_CTRx            -  Fixed-Function Counter,      ArchPerfMon v3
 * 
 *                     /63/60 /56     /48     /40     /32     /24     /16     /08     /00
 *                    {????????????????????????????????????????????????????????????????}
 *                     |                                                              |
 *     Counter Value --^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *     
 *     NB: Number of FF counters determined by    CPUID.0x0A.EDX[ 4: 0]
 *     NB: ???? FF counter bitwidth determined by CPUID.0x0A.EDX[12: 5]
 */
 /** 345   IA32_PERF_CAPABILITIES     -  Performance Capabilities Enumeration
 * 
 *                     /63/60 /56     /48     /40     /32     /24     /16     /08     /00
 *                    {..................................................##############}
 *                                                                       |||  ||||    |
 *     Full-Width Write -------------------------------------------------^||  ||||    |
 *     SMM Freeze --------------------------------------------------------^|  ||||    |
 *     PEBS Record Format -------------------------------------------------^^^^|||    |
 *     PEBS Arch Regs ---------------------------------------------------------^||    |
 *     PEBS Trap ---------------------------------------------------------------^|    |
 *     LBR Format ---------------------------------------------------------------^^^^^^
 */
/** 38D   IA32_FIXED_CTR_CTRL        -  Fixed Counter Controls,      ArchPerfMon v3
 * 
 *                     /63/60 /56     /48     /40     /32     /24     /16     /08     /00
 *                    {....................................................############}
 *                                                                         |  ||  |||||
 *     IA32_FIXED_CTR2 controls  ------------------------------------------^^^^|  |||||
 *     IA32_FIXED_CTR1 controls  ----------------------------------------------^^^^||||
 *                                                                                 ||||
 *     IA32_FIXED_CTR0 controls:                                                   ||||
 *     IA32_FIXED_CTR0 PMI --------------------------------------------------------^|||
 *     IA32_FIXED_CTR0 AnyThread ---------------------------------------------------^||
 *     IA32_FIXED_CTR0 enable (0:Disable 1:OS 2:User 3:All) -------------------------^^
 */
/** 38E   IA32_PERF_GLOBAL_STATUS    -  Global Overflow Status,      ArchPerfMon v3
 * 
 *                  /63/60 /56     /48     /40     /32     /24     /16     /08     /00
 *                 {###..........................###............................####}
 *                  |||                          |||                            ||||
 *     CondChgd ----^||                          |||                            ||||
 *     OvfDSBuffer --^|                          |||                            ||||
 *     OvfUncore -----^                          |||                            ||||
 *     IA32_FIXED_CTR2 Overflow -----------------^||                            ||||
 *     IA32_FIXED_CTR1 Overflow ------------------^|                            ||||
 *     IA32_FIXED_CTR0 Overflow -------------------^                            ||||
 *     IA32_PMC(N-1)   Overflow ------------------------------------------------^|||
 *     ....                     -------------------------------------------------^||
 *     IA32_PMC1       Overflow --------------------------------------------------^|
 *     IA32_PMC0       Overflow ---------------------------------------------------^
 */
/** 38F   IA32_PERF_GLOBAL_CTRL      -  Global Enable Controls,      ArchPerfMon v3
 * 
 *                     /63/60 /56     /48     /40     /32     /24     /16     /08     /00
 *                    {.............................###............................####}
 *                                                  |||                            ||||
 *     IA32_FIXED_CTR2 enable ----------------------^||                            ||||
 *     IA32_FIXED_CTR1 enable -----------------------^|                            ||||
 *     IA32_FIXED_CTR0 enable ------------------------^                            ||||
 *     IA32_PMC(N-1)   enable -----------------------------------------------------^|||
 *     ....                   ------------------------------------------------------^||
 *     IA32_PMC1       enable -------------------------------------------------------^|
 *     IA32_PMC0       enable --------------------------------------------------------^
 */
/** 390   IA32_PERF_GLOBAL_OVF_CTRL  -  Global Overflow Control,     ArchPerfMon v3
 * 
 *                     /63/60 /56     /48     /40     /32     /24     /16     /08     /00
 *                    {###..........................###............................####}
 *                     |||                          |||                            ||||
 *     ClrCondChgd ----^||                          |||                            ||||
 *     ClrOvfDSBuffer --^|                          |||                            ||||
 *     ClrOvfUncore -----^                          |||                            ||||
 *     IA32_FIXED_CTR2 ClrOverflow -----------------^||                            ||||
 *     IA32_FIXED_CTR1 ClrOverflow ------------------^|                            ||||
 *     IA32_FIXED_CTR0 ClrOverflow -------------------^                            ||||
 *     IA32_PMC(N-1)   ClrOverflow ------------------------------------------------^|||
 *     ....                        -------------------------------------------------^||
 *     IA32_PMC1       ClrOverflow --------------------------------------------------^|
 *     IA32_PMC0       ClrOverflow ---------------------------------------------------^
 */
/** 3F7   MSR_PEBS_FRONTEND          -  Front-End Precise Event Condition Select,    ArchPerfMon v4
 * 
 *                     /63/60 /56     /48     /40     /32     /24     /16     /08     /00
 *                    {.........................................###############...#.###}
 *                                                              | ||          |   | | |
 *     IDQ Bubble Width  Specifier -----------------------------^^^|          |   | | |
 *     IDQ Bubble Length Specifier --------------------------------^^^^^^^^^^^^   | | |
 *     Event Code Select High      -----------------------------------------------^ | |
 *     Event Code Select           -------------------------------------------------^^^
 */
/** 4C1+x IA32_A_PMCx                -  General-Purpose Counter,     ArchPerfMon v3
 * 
 *                     /63/60 /56     /48     /40     /32     /24     /16     /08     /00
 *                    {????????????????????????????????????????????????????????????????}
 *                     |                                                              |
 *     Counter Value --^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *     
 *     NB: Number of GP counters determined by    CPUID.0x0A.EAX[15: 8]
 *     NB: ???? GP counter bitwidth determined by CPUID.0x0A.EAX[23:16]
 */
/** DB2   IA32_THREAD_STALL          -  HDC Forced-Idle Cycle Counter
 * 
 * Available if CPUID.06H:EAX[bit 13] = 1
 * 
 *                     /63/60 /56     /48     /40     /32     /24     /16     /08     /00
 *                    {????????????????????????????????????????????????????????????????}
 *                     |                                                              |
 *     Counter Value --^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * 
 * Accumulate stalled cycles on this logical processor due to HDC forced idling.
 */


#endif /* End Include Guards */

