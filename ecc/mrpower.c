﻿/*
 *   MIRACL methods for modular exponentiation
 *   mrpower.c 
 *
 *   Copyright (c) 1988-1999 Shamus Software Ltd.
 */
//#include "stdafx.h"
#include <stdio.h>
#include "miracl.h"

void nres_powltr(_MIPD_ int x,big y,big w)
{ /* calculates w=x^y mod z using Left to Right Method   */
  /* uses only n^2 modular squarings, because x is small */
  /* Note: x is NOT an nresidue */
    int i,nb;
#ifndef MR_GENERIC_MT
    miracl *mr_mip=get_mip();
#endif

    if (mr_mip->ERNUM) return;
    copy(y,mr_mip->w1);

    MR_IN(86)

    zero(w);
    if (x==0) 
    {
        if (size(mr_mip->w1)==0) 
        { /* 0^0 = 1 */
            convert(_MIPP_ 1,w);
            nres(_MIPP_ w,w);
        }
        MR_OUT
        return;
    }

    convert(_MIPP_ 1,w);
    nres(_MIPP_ w,w);
    if (size(mr_mip->w1)==0) 
    {
        MR_OUT
        return;
    }
    if (size(mr_mip->w1)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);
    if (mr_mip->ERNUM)
    {
        MR_OUT
        return;
    }

    if (mr_mip->base==mr_mip->base2)
    { 
        nb=logb2(_MIPP_ mr_mip->w1);
        convert(_MIPP_ x,w);
        nres(_MIPP_ w,w);
        if (nb>1) for (i=nb-2;i>=0;i--)
        { /* Left to Right binary method */

            if (mr_mip->user!=NULL) (*mr_mip->user)();

            nres_modmult(_MIPP_ w,w,w);
            if (mr_testbit(_MIPP_ mr_mip->w1,i))
            { /* this is quick */
                premult(_MIPP_ w,x,w);
                divide(_MIPP_ w,mr_mip->modulus,mr_mip->modulus);
            }
        }
    }    
    else
    {
        expint(_MIPP_ 2,logb2(_MIPP_ mr_mip->w1)-1,mr_mip->w2);
        while (size(mr_mip->w2)!=0)
        { /* Left to Right binary method */

            if (mr_mip->user!=NULL) (*mr_mip->user)();

            nres_modmult(_MIPP_ w,w,w);
            if (compare(mr_mip->w1,mr_mip->w2)>=0)
            {
                premult(_MIPP_ w,x,w);
                divide(_MIPP_ w,mr_mip->modulus,mr_mip->modulus);
                subtract(_MIPP_ mr_mip->w1,mr_mip->w2,mr_mip->w1);
            }
            subdiv(_MIPP_ mr_mip->w2,2,mr_mip->w2);
        }
    }
    MR_OUT
    return;
}

void nres_powmod(_MIPD_ big x,big y,big w)
{  /*  calculates w=x^y mod z, using m-residues       *
    *  See "Analysis of Sliding Window Techniques for *
    *  Exponentiation, C.K. Koc, Computers. Math. &   *
    *  Applic. Vol. 30 pp17-24 1995. Uses work-space  *
    *  variables for pre-computed table. */
    int i,j,k,t,nb,nbw,nzs,n;
#ifdef MR_PENTIUM
    double *dtable[16],*dw,*dw2,*p1,*p2;
#endif
    big table[16];
#ifndef MR_GENERIC_MT
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    copy(y,mr_mip->w1);
    copy(x,mr_mip->w3);

    MR_IN(84)
    zero(w);
    if (size(x)==0)
    {
       if (size(mr_mip->w1)==0)
       { /* 0^0 = 1 */
           convert(_MIPP_ 1,w);
           nres(_MIPP_ w,w);
       } 
       MR_OUT
       return;
    }

    convert(_MIPP_ 1,w);
    nres(_MIPP_ w,w);
    if (size(mr_mip->w1)==0) 
    {
        MR_OUT
        return;
    }

    if (size(mr_mip->w1)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);

    if (mr_mip->ERNUM)
    {
        MR_OUT
        return;
    }
    if (mr_mip->base==mr_mip->base2)
    { /* build a table. Up to 5-bit sliding windows. Windows with
       * two adjacent 0 bits simply won't happen */
#ifdef MR_PENTIUM
      if (mr_mip->ACTIVE)
      {
        mr_set_align(_MIPP_ sizeof(double));  /* set for correct alignment */

        dw=mr_alloc(_MIPP_ mr_mip->nib+1,sizeof(double));
        
        dw2=mr_alloc(_MIPP_ mr_mip->nib+1,sizeof(double));

        for (i=0;i<4;i++) dtable[i]=mr_alloc(_MIPP_ mr_mip->nib+1,sizeof(double));
        dtable[4]=NULL;
        for (i=5;i<8;i++) dtable[i]=mr_alloc(_MIPP_ mr_mip->nib+1,sizeof(double));
        dtable[8]=NULL;
        dtable[9]=NULL;
        for (i=10;i<12;i++) dtable[i]=mr_alloc(_MIPP_ mr_mip->nib+1,sizeof(double));
        dtable[12]=NULL;
        for (i=13;i<16;i++) dtable[i]=mr_alloc(_MIPP_ mr_mip->nib+1,sizeof(double));

        mr_mip->dw0=mr_alloc(_MIPP_ 2*mr_mip->nib+3,sizeof(double));
        mr_mip->dmodulus=mr_alloc(_MIPP_ mr_mip->nib+1,sizeof(double));

        for (i=0;i<=mr_mip->modulus[0];i++) mr_mip->dmodulus[i]=(double)mr_mip->modulus[i];
        p1=dtable[0];
        for (i=0;i<=mr_mip->w3[0];i++) 
        {
            dw[i]=(double)mr_mip->w3[i];
            p1[i]=(double)mr_mip->w3[i];
        }
        fastmodsquare(_MIPP_ dw,dw2);

        n=15;
        j=0;
        do
        { /* pre-computations */
            t=1; k=j+1;
            while (dtable[k]==NULL) {k++; t++;}

            p1=dtable[j]; p2=dtable[k];
            for (i=0;i<=p1[0];i++) p2[i]=p1[i];

            for (i=0;i<t;i++) fastmodmult(_MIPP_ dtable[k],dw2,dtable[k]);
            j=k;
        } while (j<n);

        nb=logb2(mr_mip->w1);
        if (nb>1) for (i=nb-2;i>=0;)
        { /* Left to Right method */
            if (mr_mip->user!=NULL) (*mr_mip->user)();

            n=mr_window(_MIPP_ mr_mip->w1,i,&nbw,&nzs); 
            for (j=0;j<nbw;j++)
                    fastmodsquare(_MIPP_ dw,dw);
            if (n>0) fastmodmult(_MIPP_ dw,dtable[n/2],dw); 
            i-=nbw;
            if (nzs)
            {
                for (j=0;j<nzs;j++) fastmodsquare(_MIPP_ dw,dw);
                i-=nzs;
            }
        }
        for (i=0;i<=(int)dw[0];i++) w[i]=(int)dw[i];

        mr_free(_MIPP_ dw);
        mr_free(_MIPP_ dw2);
        mr_free(_MIPP_ mr_mip->dmodulus);
        mr_free(_MIPP_ mr_mip->dw0);
        for (i=0;i<16;i++) if (dtable[i]!=NULL) mr_free(_MIPP_ dtable[i]);

        mr_set_align(_MIPP_ 0);
      }
      else
#endif
      {
        zero(mr_mip->w5);   /* break double-length ......*/
        zero(mr_mip->w6);   /* work-space variables into 2 halves      */ 
        table[0]=mr_mip->w3; table[1]=mr_mip->w4; table[2]=mr_mip->w5; table[3]=mr_mip->w5d;
        table[4]=NULL;  table[5]=mr_mip->w6; table[6]=mr_mip->w6d; table[7]=mr_mip->w8;
        table[8]=NULL;  table[9]=NULL;  table[10]=mr_mip->w9; table[11]=mr_mip->w10;
        table[12]=NULL; table[13]=mr_mip->w11; table[14]=mr_mip->w12; table[15]=mr_mip->w13;

        nres_modmult(_MIPP_ mr_mip->w3,mr_mip->w3,mr_mip->w2);  /* x^2 */
        n=15;
        j=0;
        do
        { /* pre-computations */
            t=1; k=j+1;
            while (table[k]==NULL) {k++; t++;}
            copy(table[j],table[k]);
            for (i=0;i<t;i++) nres_modmult(_MIPP_ table[k],mr_mip->w2,table[k]);
            j=k;
        } while (j<n);

        nb=logb2(_MIPP_ mr_mip->w1);
        copy(mr_mip->w3,w);
        if (nb>1) for (i=nb-2;i>=0;)
        { /* Left to Right method */

            if (mr_mip->user!=NULL) (*mr_mip->user)();

            n=mr_window(_MIPP_ mr_mip->w1,i,&nbw,&nzs); 
            for (j=0;j<nbw;j++)
                    nres_modmult(_MIPP_ w,w,w);
            if (n>0) nres_modmult(_MIPP_ w,table[n/2],w); 
            i-=nbw;
            if (nzs)
            {
                for (j=0;j<nzs;j++) nres_modmult(_MIPP_ w,w,w);
                i-=nzs;
            }
        }
        zero(mr_mip->w5d);
        zero(mr_mip->w6d);
      }
    }
    else
    {
        copy(mr_mip->w3,mr_mip->w2);
        forever
        { /* "Russian peasant" Right-to-Left exponentiation */

            if (mr_mip->user!=NULL) (*mr_mip->user)();

            if (subdiv(_MIPP_ mr_mip->w1,2,mr_mip->w1)!=0)
                nres_modmult(_MIPP_ w,mr_mip->w2,w);
            if (mr_mip->ERNUM || size(mr_mip->w1)==0) break;
            nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w2,mr_mip->w2);
        }
    }
    MR_OUT
}

void nres_powmodn(_MIPD_ int n,big *x,big *y,big w)
{
    int i,j,k,m,nb,ea;
    big *G;
#ifndef MR_GENERIC_MT
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(112)

    m=1<<n;
    G=(big *)mr_alloc(_MIPP_ m,sizeof(big));

/* 2^n - n - 1 modmults */
/* 4(n=3) 11(n=4) etc   */

    for (i=0,k=1;i<n;i++)
    {
        for (j=0; j < (1<<i) ;j++)
        {
            G[k]=mirvar(_MIPP_ 0);
            if (j==0) copy(x[i],G[k]);
            else      nres_modmult(_MIPP_ G[j],x[i],G[k]);
            k++;
        }
    }

    nb=0;
    for (j=0;j<n;j++) 
        if ((k=logb2(_MIPP_ y[j]))>nb) nb=k;

    convert(_MIPP_ 1,w);
    nres(_MIPP_ w,w);

    if (mr_mip->base==mr_mip->base2)
    {

        for (i=nb-1;i>=0;i--) 
        {
            if (mr_mip->user!=NULL) (*mr_mip->user)();
            ea=0;
            k=1;
            for (j=0;j<n;j++)
            {
                if (mr_testbit(_MIPP_ y[j],i)) ea+=k;
                k<<=1;
            }
            nres_modmult(_MIPP_ w,w,w);
            if (ea!=0) nres_modmult(_MIPP_ w,G[ea],w);
        }
    }
    else mr_berror(_MIPP_ MR_ERR_NOT_SUPPORTED);
    for (i=1;i<m;i++) mirkill(_MIPP_ G[i]);
    mr_free(_MIPP_ G);

    MR_OUT
}

void powmodn(_MIPD_ int n,big *x,big *y,big p,big w)
{/* w=x[0]^y[0].x[1]^y[1] .... x[n-1]^y[n-1] mod n */
    int j;
#ifndef MR_GENERIC_MT
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(113)

    prepare_monty(_MIPP_ p);
    for (j=0;j<n;j++) nres(_MIPP_ x[j],x[j]);
    nres_powmodn(_MIPP_ n,x,y,w);   
    for (j=0;j<n;j++) redc(_MIPP_ x[j],x[j]);

    redc(_MIPP_ w,w);

    MR_OUT
}

void nres_powmod2(_MIPD_ big x,big y,big a,big b,big w)
{ /* finds w = x^y.a^b mod n. Fast for some cryptosystems */ 
    int i,j,nb,nb2,nbw,nzs,n;
    big table[16];
#ifndef MR_GENERIC_MT
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    copy(y,mr_mip->w1);
    copy(x,mr_mip->w2);
    copy(b,mr_mip->w3);
    copy(a,mr_mip->w4);
    zero(w);
    if (size(mr_mip->w2)==0 || size(mr_mip->w4)==0) return;

    MR_IN(99)

    convert(_MIPP_ 1,w);
    nres(_MIPP_ w,w);
    if (size(mr_mip->w1)==0 && size(mr_mip->w3)==0) 
    {
        MR_OUT
        return;
    }
    if (size(mr_mip->w1)<0 || size(mr_mip->w3)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);
    if (mr_mip->ERNUM)
    {
        MR_OUT
        return;
    }
     
    if (mr_mip->base==mr_mip->base2)
    { /* uses 2-bit sliding window. This is 25% faster! */

        zero(mr_mip->w5);   /* break double-length ......*/
        zero(mr_mip->w6);   /* work-space variables into 2 halves      */ 

        nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w4,mr_mip->w5);
        nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w2,mr_mip->w12);
        nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w4,mr_mip->w13);
        nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w13,mr_mip->w5d);
        nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w13,mr_mip->w6);
        nres_modmult(_MIPP_ mr_mip->w6,mr_mip->w4,mr_mip->w6d);
        nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w12,mr_mip->w8);
        nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w12,mr_mip->w9);
        nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w9,mr_mip->w10);
        nres_modmult(_MIPP_ mr_mip->w5d,mr_mip->w12,mr_mip->w11);
        nres_modmult(_MIPP_ mr_mip->w9,mr_mip->w13,mr_mip->w12);
        nres_modmult(_MIPP_ mr_mip->w10,mr_mip->w13,mr_mip->w13);

        table[0]=NULL; table[1]=mr_mip->w4;  table[2]=mr_mip->w2;   table[3]=mr_mip->w5;
        table[4]=NULL; table[5]=mr_mip->w5d; table[6]=mr_mip->w6;   table[7]=mr_mip->w6d;
        table[8]=NULL; table[9]=mr_mip->w8;  table[10]=mr_mip->w9;  table[11]=mr_mip->w10;
        table[12]=NULL;table[13]=mr_mip->w11;table[14]=mr_mip->w12; table[15]=mr_mip->w13;
        nb=logb2(_MIPP_ mr_mip->w1);
        if ((nb2=logb2(_MIPP_ mr_mip->w3))>nb) nb=nb2;

        for (i=nb-1;i>=0;)
        { 
            if (mr_mip->user!=NULL) (*mr_mip->user)();

            n=mr_window2(_MIPP_ mr_mip->w1,mr_mip->w3,i,&nbw,&nzs);
            for (j=0;j<nbw;j++)
                nres_modmult(_MIPP_ w,w,w);
            if (n>0) nres_modmult(_MIPP_ w,table[n],w);
            i-=nbw;
            if (nzs) 
            {
                nres_modmult(_MIPP_ w,w,w);
                i--;
            } 
        }
        zero(mr_mip->w5d);
        zero(mr_mip->w6d);

    }
    else
    {
        nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w4,mr_mip->w5);

        if (compare(mr_mip->w1,mr_mip->w3)>=0)
             expint(_MIPP_ 2,logb2(_MIPP_ mr_mip->w1)-1,mr_mip->w6);
        else expint(_MIPP_ 2,logb2(_MIPP_ mr_mip->w3)-1,mr_mip->w6);
        while (size(mr_mip->w6)!=0)
        {
            if (mr_mip->user!=NULL) (*mr_mip->user)();

            nres_modmult(_MIPP_ w,w,w);
            if (compare(mr_mip->w1,mr_mip->w6)>=0)
            {
                if (compare(mr_mip->w3,mr_mip->w6)>=0)
                {
                     nres_modmult(_MIPP_ w,mr_mip->w5,w);
                     subtract(_MIPP_ mr_mip->w3,mr_mip->w6,mr_mip->w3);
                }

                else nres_modmult(_MIPP_ w,mr_mip->w2,w);
                subtract(_MIPP_ mr_mip->w1,mr_mip->w6,mr_mip->w1);
            }
            else
            {
                if (compare(mr_mip->w3,mr_mip->w6)>=0)
                {
                     nres_modmult(_MIPP_ w,mr_mip->w4,w);
                     subtract(_MIPP_ mr_mip->w3,mr_mip->w6,mr_mip->w3);
                }
            }
            subdiv(_MIPP_ mr_mip->w6,2,mr_mip->w6);
        }
    }
    MR_OUT
}

void powmod2(_MIPD_ big x,big y,big a,big b,big n,big w)
{/* w=x^y.a^b mod n */
#ifndef MR_GENERIC_MT
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(79)

    prepare_monty(_MIPP_ n);
    nres(_MIPP_ x,mr_mip->w2);
    nres(_MIPP_ a,mr_mip->w4);
    nres_powmod2(_MIPP_ mr_mip->w2,y,mr_mip->w4,b,w);
    redc(_MIPP_ w,w);

    MR_OUT   
}

static mr_small sgcd(mr_small x,mr_small y)
{ /* integer GCD, returns GCD of x and y */
    mr_small r;
    if (y==(mr_small)0) return x;
    while ((r=x%y)!=(mr_small)0)
        x=y,y=r;
    return y;
}


void powmod(_MIPD_ big x,big y,big n,big w)
{ /* fast powmod, using Montgomery's method internally */

    mr_small norm;
    BOOL mty;
#ifndef MR_GENERIC_MT
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(18)

    mty=TRUE;
    if (mr_mip->base!=mr_mip->base2)
    {
        if (size(n)<2 || sgcd(n[1],mr_mip->base)!=1) mty=FALSE;
    }
    else
        if (subdivisible(_MIPP_ n,2)) mty=FALSE;


    if (!mty)
    { /* can't use Montgomery */
        copy(y,mr_mip->w1);
        copy(x,mr_mip->w3);
        zero(w);
        if (size(mr_mip->w3)==0) 
        {
            MR_OUT
            return;
        }
        convert(_MIPP_ 1,w);
        if (size(mr_mip->w1)==0) 
        {
            MR_OUT
            return;
        }
        if (size(mr_mip->w1)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);
        if (w==n)           mr_berror(_MIPP_ MR_ERR_BAD_PARAMETERS) ;
        if (mr_mip->ERNUM)
        {
            MR_OUT
            return;
        }

        norm=normalise(_MIPP_ n,n);
        divide(_MIPP_ mr_mip->w3,n,n);
        forever
        { 
            if (mr_mip->user!=NULL) (*mr_mip->user)();

            if (subdiv(_MIPP_ mr_mip->w1,2,mr_mip->w1)!=0)
                mad(_MIPP_ w,mr_mip->w3,mr_mip->w3,n,n,w);
            if (mr_mip->ERNUM || size(mr_mip->w1)==0) break;
            mad(_MIPP_ mr_mip->w3,mr_mip->w3,mr_mip->w3,n,n,mr_mip->w3);
        }
        if (norm!=1)
        {
            mr_sdiv(_MIPP_ n,norm,n);
            divide(_MIPP_ w,n,n);
        }
    }
    else
    { /* optimized code for odd moduli */
        prepare_monty(_MIPP_ n); 
        nres(_MIPP_ x,mr_mip->w3);
        nres_powmod(_MIPP_ mr_mip->w3,y,w);
        redc(_MIPP_ w,w);
    }
    
    MR_OUT
}

int powltr(_MIPD_ int x, big y, big n, big w)
{
    mr_small norm;
    BOOL mty;
#ifndef MR_GENERIC_MT
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return 0;

    MR_IN(71)

    mty=TRUE;
    if (mr_mip->base!=mr_mip->base2)
    {
        if (size(n)<2 || sgcd(n[1],mr_mip->base)!=1) mty=FALSE;
    }
    else
    {
        if (subdivisible(_MIPP_ n,2)) mty=FALSE;
    }

/* Montgomery modulus is in use... */
    if (mr_mip->modulus!=NULL)
        if (compare(n,mr_mip->modulus)!=0) mty=FALSE;

    if (!mty)
    { /* can't use Montgomery! */
        copy(y,mr_mip->w1);
        zero(w);
        if (x==0) 
        {
            MR_OUT
            return 0;
        }
        convert(_MIPP_ 1,w);
        if (size(mr_mip->w1)==0)
        {
            MR_OUT
            return 1;
        }
        
        if (size(mr_mip->w1)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);
        if (w==n)               mr_berror(_MIPP_ MR_ERR_BAD_PARAMETERS) ;
        if (mr_mip->ERNUM)
        {
            MR_OUT
            return 0;
        }
        
        norm=normalise(_MIPP_ n,n);

        expint(_MIPP_ 2,logb2(_MIPP_ mr_mip->w1)-1,mr_mip->w2);
        while (size(mr_mip->w2)!=0)
        { /* Left to Right binary method */

            if (mr_mip->user!=NULL) (*mr_mip->user)();
            mad(_MIPP_ w,w,w,n,n,w);
            if (compare(mr_mip->w1,mr_mip->w2)>=0)
            {
                premult(_MIPP_ w,x,w);
                divide(_MIPP_ w,n,n);
                subtract(_MIPP_ mr_mip->w1,mr_mip->w2,mr_mip->w1);
            }
            subdiv(_MIPP_ mr_mip->w2,2,mr_mip->w2);
        }
        if (norm!=1) 
        {
            mr_sdiv(_MIPP_ n,norm,n);
            divide(_MIPP_ w,n,n);
        }
    }
    else
    { /* optimized code for odd moduli */
        prepare_monty(_MIPP_ n);
        nres_powltr(_MIPP_ x,y,w);
        redc(_MIPP_ w,w);
    }
    MR_OUT 
    return (size(w));
}

BOOL nres_sqroot(_MIPD_ big x,big w)
{ /* w=sqrt(x) mod p. This depends on p being prime! */
    int i,n,e,r,cat;
    BOOL pp;
#ifndef MR_GENERIC_MT
    miracl *mr_mip=get_mip();
#endif
    copy(x,mr_mip->w15);
    zero(w);
    if (size(mr_mip->w15)==0) zero(w);
    if (mr_mip->ERNUM) return FALSE;

    MR_IN(100)

    convert(_MIPP_ 1,w);
    nres(_MIPP_ w,w);
    if (compare(w,mr_mip->w15)==0)
    {
        MR_OUT
        return TRUE;
    }

    cat=remain(_MIPP_ mr_mip->modulus,8);
    switch(cat)
    {
    case 0: case 2: case 4: case 6: 
        zero(w);
        MR_OUT
        return FALSE;

    case 3: case 7:                          /* easy case */
        incr(_MIPP_ mr_mip->modulus,1,mr_mip->w14);
        subdiv(_MIPP_ mr_mip->w14,4,mr_mip->w14);
        nres_powmod(_MIPP_ x,mr_mip->w14,w);
        nres_modmult(_MIPP_ w,w,mr_mip->w14);
        MR_OUT
        if (compare(mr_mip->w14,mr_mip->w15)==0) 
            return TRUE;
        zero(w);
        return FALSE;

    case 5:
        nres_modadd(_MIPP_ mr_mip->w15,mr_mip->w15,mr_mip->w15); /* 2x */
        decr(_MIPP_ mr_mip->modulus,5,mr_mip->w14);
        subdiv(_MIPP_ mr_mip->w14,8,mr_mip->w14);
        nres_powmod(_MIPP_ mr_mip->w15,mr_mip->w14,w);
        nres_modmult(_MIPP_ w,w,mr_mip->w14);
        nres_modmult(_MIPP_ mr_mip->w15,mr_mip->w14,mr_mip->w14);
        convert(_MIPP_ 1,mr_mip->w1);
        nres(_MIPP_ mr_mip->w1,mr_mip->w1);
        nres_modsub(_MIPP_ mr_mip->w14,mr_mip->w1,mr_mip->w14);
        nres_modmult(_MIPP_ mr_mip->w14,w,w);
        if (!subdivisible(_MIPP_ mr_mip->w15,2))
            add(_MIPP_ mr_mip->w15,mr_mip->modulus,mr_mip->w15);
        subdiv(_MIPP_ mr_mip->w15,2,mr_mip->w15);                /* x */
        nres_modmult(_MIPP_ w,mr_mip->w15,w);
        nres_modmult(_MIPP_ w,w,mr_mip->w14);
        MR_OUT
        if (compare(mr_mip->w14,mr_mip->w15)==0) 
            return TRUE;
        zero(w);
        return FALSE;
        
    case 1:                      /* difficult case. Shank's method */

        decr(_MIPP_ mr_mip->modulus,1,mr_mip->w14);
        e=0;
        while (subdivisible(_MIPP_ mr_mip->w14,2))
        {
            subdiv(_MIPP_ mr_mip->w14,2,mr_mip->w14);
            e++;
        }
        for (r=2;;r++)
        {
            convert(_MIPP_ 1,mr_mip->w3);
            nres(_MIPP_ mr_mip->w3,mr_mip->w3);             /* w3=1 */
            nres_powltr(_MIPP_ r,mr_mip->w14,w);
            if (compare(w,mr_mip->w3)==0) continue;
            copy(w,mr_mip->w4);
            nres_negate(_MIPP_ mr_mip->w3,mr_mip->w1);      /* w1 = -1 */
            pp=FALSE;
            for (i=1;i<e;i++)         
            { /* check for composite modulus */
                if (mr_mip->user!=NULL) (*mr_mip->user)();

                if (compare(mr_mip->w4,mr_mip->w1)==0) pp=TRUE;
                nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w4,mr_mip->w4);
                if (!pp && compare(mr_mip->w4,mr_mip->w3)==0)
                {              
                    zero(w);
                    MR_OUT
                    return FALSE;
                }
            }   
            if (compare(mr_mip->w4,mr_mip->w1)==0) break;
            if (!pp)
            {
                zero(w);
                MR_OUT
                return FALSE;
            }
        }                                                /* w= y    */
        copy(mr_mip->w15,mr_mip->w3);                    /* w3 = x  */
        nres_powmod(_MIPP_ mr_mip->w3,mr_mip->w14,mr_mip->w15);  /* w15 = w */
        incr(_MIPP_ mr_mip->w14,1,mr_mip->w14);
        subdiv(_MIPP_ mr_mip->w14,2,mr_mip->w14);
        nres_powmod(_MIPP_ mr_mip->w3,mr_mip->w14,mr_mip->w14);  /* w14 = v */
        forever
        {
            if (mr_mip->user!=NULL) (*mr_mip->user)();

            convert(_MIPP_ 1,mr_mip->w1);
            nres(_MIPP_ mr_mip->w1,mr_mip->w1);
            if (compare(w,mr_mip->w1)==0)  break;
            copy(mr_mip->w15,mr_mip->w2);
            for (n=0;compare(mr_mip->w2,mr_mip->w1)!=0;n++)
                nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w2,mr_mip->w2);
            if (n>=e)
            {
                zero(w);
                MR_OUT
                return FALSE;
            }
            r=e-n-1;
            for (i=0;i<r;i++) nres_modmult(_MIPP_ w,w,w);
            nres_modmult(_MIPP_ mr_mip->w14,w,mr_mip->w14);
            nres_modmult(_MIPP_ w,w,w);
            nres_modmult(_MIPP_ mr_mip->w15,w,mr_mip->w15);
            e=n;
        }
        copy(mr_mip->w14,w);
        nres_modmult(_MIPP_ w,w,mr_mip->w14);
        MR_OUT
        if (compare(mr_mip->w14,mr_mip->w3)==0) 
            return TRUE;
        zero(w);
        return FALSE;
        
    }
    return FALSE;
}

BOOL sqroot(_MIPD_ big x,big p,big w)
{ /* w = sqrt(x) mod p */
#ifndef MR_GENERIC_MT
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return FALSE;

    MR_IN(101)

    if (subdivisible(_MIPP_ p,2))
    { /* p must be prime */
        zero(w);
        MR_OUT
        return FALSE;
    }
    prepare_monty(_MIPP_ p);
    nres(_MIPP_ x,mr_mip->w15);
    if (nres_sqroot(_MIPP_ mr_mip->w15,w))
    {
        redc(_MIPP_ w,w);
        MR_OUT
        return TRUE;
    }
    zero(w);
    MR_OUT
    return FALSE;
}

