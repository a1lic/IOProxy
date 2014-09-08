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
* Neither the name of the Alice Software Studio nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ALICE BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <Windows.h>

class Utf8Buffer
{
protected:
	CHAR * Buffer;
	size_t BufferSize;
	ptrdiff_t BufferCursor;
	bool AbortingThread;
	bool InvalidInstance;
public:
	Utf8Buffer(size_t BufferSize);
	virtual ~Utf8Buffer();
	inline void AbortThread() { this->AbortingThread = true; };
	static bool Utf8ToUcs4(unsigned long * const ucs4, const unsigned char * const utf8, unsigned char * const bytes_read);
};

#define REMAINS_BUFFER_BYTES 8

class Utf8Outputter : public Utf8Buffer
{
protected:
	HANDLE Input;
	CHAR * ValidUtf8Buffer;
	ptrdiff_t RangeStart;
	size_t Range;
	CHAR Remains[REMAINS_BUFFER_BYTES];
	unsigned char Remaining;
	bool OutToStdErr;
	bool Cancelled;
public:
	Utf8Outputter(HANDLE Input, size_t BufferSize, bool IsStdError);
	~Utf8Outputter();
private:
	void WriteToConsole();
	bool GetValidRange();
	bool CopyValidUtf8();
	unsigned int OutputterThread();
	// CALLBACK
public:
	static unsigned int __stdcall _OutputterThread(void * const Argument);
private:
	static VOID CALLBACK OutputComplete(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
};

class Utf8Inputter : public Utf8Buffer
{
protected:
	HANDLE Output;
	HANDLE Input;
	WCHAR * UnicodeBuffer;
	size_t UnicodeBufferSize;
public:
	Utf8Inputter(HANDLE Output, size_t BufferSize);
	~Utf8Inputter();
private:
	unsigned int InputterThread();
public:
	static unsigned int __stdcall _InputterThread(void * const Argument);
};
