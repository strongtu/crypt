#pragma once

#include "ecckey.h"
#include "eccparam.h"

//#include "Common/Include/Crypt.h"

//#ifdef WIN32
//#ifndef TXCOMMON_API
//#define TXCOMMON_API __declspec(dllimport)
//#endif
//#else
//#ifndef TXCOMMON_API
//#define TXCOMMON_API 
//#endif
//#endif
#undef TXCOMMON_API
#define TXCOMMON_API

class Miracl;
/******************************************************************************************************************************************/
//
//           !!!注意 CEcdsa 变量的定义一定要比 CPublicKey，CPrivateKey，CSignature定义先，因为 CEcdsa的构造函数中存在全局初始化函数。!!! -lorischen
//
/********************************************************************************************************************************************/
class CEcdsa
{
public:
	//ECC_P192相当于RSA的1024位
	TXCOMMON_API CEcdsa(eccparam::ECCBIT bit = eccparam::ECC_P112);
	TXCOMMON_API ~CEcdsa();
	//生成KEY, pub是公钥, pri是私钥
	TXCOMMON_API void GenKey(CPublicKey& pub, CPrivateKey& pri);
	//签名, pri是私钥,val是签名的内容,sig是生成的签名
	TXCOMMON_API void Sign(const CPrivateKey& pri, const Big& val, CSignature& sig);
	//验证,pub是公钥,val是签名的内容,sig是签名,返回true:成功
	TXCOMMON_API bool Verify(const CPublicKey& pub, const Big& val, const CSignature& sig);
	//交换KEY, pub是对方的公钥, pri是自己的私钥,返回交换出来的KEY
	TXCOMMON_API Big GetExchangeKey(const CPublicKey& pub, const CPrivateKey& pri);
	//lorischen add
    //先对buff计算md5，然后再对md5计算签名。
	TXCOMMON_API void QQSign(const CPrivateKey& pri,const unsigned char *pBuf,int nLen,CSignature& sig);
    //对应于上面的QQSign，添加QQVerify
	TXCOMMON_API bool QQVerify(const CPublicKey& pub,const unsigned char *pBuf,int nLen, const CSignature& sig);
    //lorischen end

private:
	ECn *m_oG;
	Big *m_oQ;
	Big *m_oV;
	Miracl *m_pMiracl;
};

