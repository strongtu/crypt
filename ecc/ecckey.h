#pragma once

#ifdef WIN32

#include <windows.h>

#else

#include <netinet/in.h>
#include "big.h"
#endif

#include <iostream>
#include "elliptic.h"
#include <string.h>

struct CPrivateKey
{
	Big v;

	inline int Len()  //用于savetobuffer的长度，实际可能小于这个长度，以SaveToBuf为准
	{
		return v.len() * sizeof(mr_small);
	}

	inline int SaveToBuf(void * buf, int len)
	{
		return to_binary(v, len, (char *)buf);
	}

	inline void LoadFromBuf(void * buf, int len)
	{
		v = from_binary(len, (char *)buf);
	}
};

struct CPublicKey
{
	Big v;
	int ep;

	inline int Len() //用于savetobuffer的长度，实际可能小于这个长度，以SaveToBuf为准
	{
		return v.len() * sizeof(mr_small) + sizeof(ep);
	}

	inline int SaveToBuf(void * buf, int len)
	{
		if (len >= sizeof(ep))
		{
			int nep = htonl(ep);
			memcpy(buf, &nep, sizeof(ep));			
			return sizeof(ep) + to_binary(v, len - sizeof(ep), (char *)buf + sizeof(ep));
		}
		return 0;
	}

	inline void LoadFromBuf(void * buf, int len)
	{
		if (len >= sizeof(ep))
		{
			memcpy(&ep, buf, sizeof(ep));
			ep = ntohl(ep);
			v = from_binary(len - sizeof(ep), (char *)buf + sizeof(ep));
		}
	}
};

struct CSignature
{
	Big r;
	Big s;

	inline int Len() //用于savetobuffer的长度，实际可能小于这个长度，以SaveToBuf为准
	{
		return (r.len() + s.len()) * sizeof(mr_small) + sizeof(unsigned short);
	}

	inline int SaveToBuf(void * buf, int len)
	{
		int rlen = 0;
		if (len >= Len())
		{
			rlen = to_binary(r, r.len() * sizeof(mr_small), (char *)buf + sizeof(unsigned short));
			unsigned short usLen = rlen;
			usLen = ntohs(usLen);
			memcpy((char *)buf, &usLen, sizeof(usLen));
			rlen += to_binary(s, s.len() * sizeof(mr_small), (char *)buf + rlen + sizeof(unsigned short));
			rlen += sizeof(unsigned short);
		}
		return rlen;
	}

	inline void LoadFromBuf(void * buf, int len)
	{
		unsigned short rlen;
		if (len > sizeof(rlen))
		{
			memcpy(&rlen, buf, sizeof(rlen));
			rlen = htons(rlen);
			r = from_binary(rlen, (char *)buf + sizeof(rlen));
			if (len > rlen)
			{
				s = from_binary(len - rlen - sizeof(rlen), (char *)buf + rlen + sizeof(rlen));
			}
		}
	}
};


