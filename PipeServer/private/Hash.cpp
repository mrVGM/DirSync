#include "Hash.h"

#include <string>
#include <stdio.h>

#include <Windows.h>

namespace
{
	typedef union uwb {
		unsigned w;
		unsigned char b[4];
	} MD5union;

	typedef unsigned DigestArray[4];

	static unsigned func0(unsigned abcd[]) {
		return (abcd[1] & abcd[2]) | (~abcd[1] & abcd[3]);
	}

	static unsigned func1(unsigned abcd[]) {
		return (abcd[3] & abcd[1]) | (~abcd[3] & abcd[2]);
	}

	static unsigned func2(unsigned abcd[]) {
		return  abcd[1] ^ abcd[2] ^ abcd[3];
	}

	static unsigned func3(unsigned abcd[]) {
		return abcd[2] ^ (abcd[1] | ~abcd[3]);
	}

	typedef unsigned(*DgstFctn)(unsigned a[]);

	static unsigned* calctable(unsigned* k)
	{
		double s, pwr;
		int i;

		pwr = pow(2.0, 32);
		for (i = 0; i < 64; i++) {
			s = fabs(sin(1.0 + i));
			k[i] = (unsigned)(s * pwr);
		}
		return k;
	}

	static unsigned rol(unsigned r, short N)
	{
		unsigned  mask1 = (1 << N) - 1;
		return ((r >> (32 - N)) & mask1) | ((r << N) & ~mask1);
	}

	void MD5Hash(DigestArray h, const unsigned char* msg, int mlen)
	{
		static DgstFctn ff[] = { &func0, &func1, &func2, &func3 };
		static short M[] = { 1, 5, 3, 7 };
		static short O[] = { 0, 1, 5, 0 };
		static short rot0[] = { 7, 12, 17, 22 };
		static short rot1[] = { 5, 9, 14, 20 };
		static short rot2[] = { 4, 11, 16, 23 };
		static short rot3[] = { 6, 10, 15, 21 };
		static short* rots[] = { rot0, rot1, rot2, rot3 };
		static unsigned kspace[64];
		static unsigned* k;

		DigestArray abcd;
		DgstFctn fctn;
		short m, o, g;
		unsigned f;
		short* rotn;
		union {
			unsigned w[16];
			char     b[64];
		}mm;
		int os = 0;
		int grp, grps, q, p;
		unsigned char* msg2 = nullptr;

		if (k == NULL) k = calctable(kspace);

		{
			grps = 1 + (mlen + 8) / 64;
			msg2 = new unsigned char[64 * grps];
			memcpy(msg2, msg, mlen);
			msg2[mlen] = (unsigned char)0x80;
			q = mlen + 1;
			while (q < 64 * grps) { msg2[q] = 0; q++; }
			{
				MD5union u;
				u.w = 8 * mlen;
				q -= 8;
				memcpy(msg2 + q, &u.w, 4);
			}
		}

		for (grp = 0; grp < grps; grp++)
		{
			memcpy(mm.b, msg2 + os, 64);
			for (q = 0; q < 4; q++) abcd[q] = h[q];
			for (p = 0; p < 4; p++) {
				fctn = ff[p];
				rotn = rots[p];
				m = M[p]; o = O[p];
				for (q = 0; q < 16; q++) {
					g = (m * q + o) % 16;
					f = abcd[1] + rol(abcd[0] + fctn(abcd) + k[q + 16 * p] + mm.w[g], rotn[q % 4]);

					abcd[0] = abcd[3];
					abcd[3] = abcd[2];
					abcd[2] = abcd[1];
					abcd[1] = f;
				}
			}
			for (p = 0; p < 4; p++)
				h[p] += abcd[p];
			os += 64;
		}

		delete[] msg2;
	}

	static std::string GetMD5String(unsigned* d) {
		std::string str;
		int j, k;
		MD5union u;
		for (j = 0; j < 4; j++) {
			u.w = d[j];
			char s[9];
			sprintf(s, "%02x%02x%02x%02x", u.b[0], u.b[1], u.b[2], u.b[3]);
			str += s;
		}

		return str;
	}
}

void crypto::HashBinFile(const std::string& fileName, std::string& hash, size_t& fileSize)
{
	hash = "";
	fileSize = 0;

	const size_t buffSize = 8 * 1024 * 1024;

	HANDLE fHandle = CreateFile(
		fileName.c_str(),
		GENERIC_READ,
		NULL,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	
	if (!fHandle)
	{
		return;
	}

	static DigestArray h0 = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };
	DigestArray h;
	for (int q = 0; q < 4; q++)
	{
		h[q] = h0[q];
	}

	unsigned char* buff = new unsigned char[buffSize];

	DWORD read;
	ReadFile(
		fHandle,
		buff,
		buffSize,
		&read,
		NULL);

	fileSize = read;
	while (read > 0)
	{
		MD5Hash(h, buff, buffSize);

		ReadFile(
			fHandle,
			buff,
			buffSize,
			&read,
			NULL);
		fileSize += read;
	}

	CloseHandle(fHandle);

	hash = GetMD5String(h);
	delete[] buff;
}
