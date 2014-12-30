/* 
 *   MIRACL compiler/hardware definitions - mirdef.h
 *   This version suitable for use with most 32-bit computers
 *   e.g. 80386+ PC, VAX, ARM etc. Assembly language versions of muldiv,
 *   muldvm, muldvd and muldvd2 will be necessary. See mrmuldv.any 
 *
 *   Also suitable for DJGPP GNU C Compiler
 *   Copyright (c) 1988-2001 Shamus Software Ltd.
 */
#if !defined(NO_ASM)
#define MR_WINDOWS_MT
#define MIRACL 32
#define MR_LITTLE_ENDIAN    /* This may need to be changed        */
#define mr_utype int
                            /* the underlying type is usually int *
                             * but see mrmuldv.any                */
#define mr_unsign32 unsigned int
                            /* 32 bit unsigned type               */
#define MR_IBITS      32    /* bits in int  */
#define MR_LBITS      32    /* bits in long */
#define MR_FLASH      52      
                            /* Delete this definition if integer  *
                             * only version of MIRACL required    */
                            /* Number of bits per double mantissa */
#else

#define MIRACL 32
#define MR_LITTLE_ENDIAN    /* This may need to be changed        */
#define mr_utype int
#define mr_unsign32 unsigned int
#if defined(_MSC_VER)
#define mr_dltype _int64
#define mr_unsign64 unsigned _int64
#else
#define mr_dltype long long
#define mr_unsign64 unsigned long long
#endif
#define MR_IBITS 32
#define MR_LBITS 32
#define MR_NOASM
#define MR_FLASH 52
#endif
