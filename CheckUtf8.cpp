/*
Copyright (c) 2014, alice (@a1lic)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
* Neither the name of the <organization> nor theÅ@names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define OUTPUT_BUFFER_BYTES 65536U

#include <tchar.h>
#include <Windows.h>

const CHAR Utf8Str[] = {
	/*0xe8, */0xbe, 0x9b, 0xe3, 0x81, 0x95, 0xe3, 0x81, 0x97, 0xe3, 0x81, 0x8b, 0xe3, 0x81, 0xaa, 0xe3,
	0x81, 0x84, 0xe4, 0xba, 0xba, 0xe7, 0x94, 0x9f, 0xe3, 0x81, 0xa0, 0xe3, 0x81, 0xa3, 0xe3/*, 0x81,
	0x9f*/};

extern "C" bool GetValidRangeUtf8String(const CHAR * const Buffer, size_t BufferSize, ptrdiff_t StartAt, ptrdiff_t * const ValidAt, size_t * const ValidLength);

extern "C" int c_tmain(int const argc, const _TCHAR * const argv[])
{
	ptrdiff_t ValidAt;
	size_t ValidLength;
	WCHAR * WideStr;

	::GetValidRangeUtf8String(Utf8Str, sizeof(Utf8Str), 0, &ValidAt, &ValidLength);
	::_tprintf_s(_T("Valid at %d, Valid len: %ud\n"), ValidAt, ValidLength);
	WideStr = new WCHAR[256];
	::memset(WideStr, 0, sizeof(WCHAR) * 256);
	::MultiByteToWideChar(CP_UTF8, 0, &Utf8Str[ValidAt], (int)ValidLength, WideStr, 256);
	::_tprintf_s(_T("%ls\n"), WideStr);
	delete[] WideStr;

	return 0;
}
