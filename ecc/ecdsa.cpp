#include "stdafx.h"
#include "ecdsa.h"
#include "eccparam.h"
#include <time.h>
#include "big.h"

#define MD5_DIGEST_LENGTH 16

Miracl *g_oPrecision = NULL;
static void UninitEcdsa()
{
	if(g_oPrecision && g_oPrecision->threadid == GetCurrentThreadId())  //lorischen add
	{
		delete g_oPrecision;
		g_oPrecision = NULL;
	}
	mr_end_threading();
}
static void InitEcdsa()
{
	atexit(UninitEcdsa);
	mr_init_threading();
	g_oPrecision = new Miracl(200);
}

extern "C" void * e_txweakobj_24 = InitEcdsa;

CEcdsa::CEcdsa(eccparam::ECCBIT bit)
{
	m_pMiracl = NULL;
	m_oG = NULL;
	m_oQ = NULL;
	m_oV = NULL;
	miracl * pMip = NULL;
	if(g_oPrecision->threadid == GetCurrentThreadId() )
	{
		pMip = g_oPrecision->mr;
	}
	else
	{
		m_pMiracl = new Miracl(200);
		pMip = m_pMiracl->mr;
	}
	irand((unsigned int)time(0));
	pMip->IOBASE=16;
	m_oG = new ECn;
	m_oQ = new Big;
	m_oV = new Big;
	Big a(eccparam::a[bit]);
	Big b(eccparam::b[bit]);
	Big p(eccparam::p[bit]);
	Big x(eccparam::x[bit]);
	Big y(eccparam::y[bit]);
	*m_oQ = Big(eccparam::q[bit]);
	ecurve(a, b, p, MR_PROJECTIVE);
	*m_oG = ECn(x, y);
}

CEcdsa::~CEcdsa()
{
	if(m_oG)
		delete m_oG;
	if(m_oQ)
		delete m_oQ;
	if(m_oV)
		delete m_oV;
	if(m_pMiracl)
	{
		delete m_pMiracl;
		m_pMiracl = NULL;
	}
}

void CEcdsa::GenKey(CPublicKey& pub, CPrivateKey& pri)
{
	pri.v = rand(*m_oQ);
	ECn tmp = pri.v * (*m_oG);
	pub.ep = tmp.get(pub.v);
}

void CEcdsa::Sign(const CPrivateKey& pri, const Big& val, CSignature& sig)
{
    Big k = rand(*m_oQ);
    ECn tmp = k * (*m_oG);            /* see brick.cpp for technique to speed this up */
    tmp.get(sig.r);
    sig.r %= (*m_oQ);
    k = inverse(k, (*m_oQ));
    sig.s = ((val + pri.v * sig.r) * k) % (*m_oQ);
}

bool CEcdsa::Verify(const CPublicKey& pub, const Big& val, const CSignature& sig)
{
	if(sig.r == 0 || sig.s == 0 ) //加上0保护。
		return false;
	ECn Pub(pub.v, pub.ep);
    Big s = inverse(sig.s, (*m_oQ));
    Big u1 = (val * s) % (*m_oQ);
    Big u2 = (sig.r * s) % (*m_oQ);

    mul(u2, Pub, u1, (*m_oG)).get((*m_oV));
    (*m_oV) %= (*m_oQ);
    return ((*m_oV) == sig.r) != 0;
}

Big CEcdsa::GetExchangeKey(const CPublicKey& pub, const CPrivateKey& pri)
{
	Big key;
	ECn Pub(pub.v, pub.ep);
	Pub *= pri.v;
	Pub.get(key);
	return key;
}

//void CEcdsa::QQSign(const CPrivateKey& pri,const unsigned char *pBuf,int nLen,CSignature& sig)
//{
//	Big val;
//	BYTE md5[MD5_DIGEST_LENGTH];
//	Md5HashBuffer(md5,pBuf,nLen);
//	val = from_binary(MD5_DIGEST_LENGTH,(char *)md5);
//	Sign(pri,val,sig);
//}
//
//bool CEcdsa::QQVerify(const CPublicKey& pub,const unsigned char *pBuf,int nLen, const CSignature& sig)
//{
//	Big val;
//	BYTE md5[MD5_DIGEST_LENGTH];
//	Md5HashBuffer(md5,pBuf,nLen);
//	val = from_binary(MD5_DIGEST_LENGTH,(char *)md5);
//	return Verify(pub,val,sig);
//}
