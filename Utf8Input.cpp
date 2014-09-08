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

#include <malloc.h>
#include <Windows.h>
#include "Utf8IO.hpp"

Utf8Inputter::Utf8Inputter(HANDLE Output, size_t BufferSize) : Utf8Buffer(BufferSize)
{
	this->UnicodeBuffer = reinterpret_cast<WCHAR *>(::VirtualAlloc(nullptr, BufferSize, MEM_COMMIT, PAGE_READWRITE));
	if(this->UnicodeBuffer == nullptr)
	{
		throw "VIRTUALALLOC FAILURE AT Utf8Inputter";
	}
	this->UnicodeBufferSize = BufferSize / sizeof(WCHAR);
	this->Output = Output;
	this->Input = ::GetStdHandle(STD_INPUT_HANDLE);
}

Utf8Inputter::~Utf8Inputter()
{
	::VirtualFree(this->UnicodeBuffer, 0, MEM_RELEASE);
	::CloseHandle(this->Output);
}

/*/
extern "C" unsigned int __stdcall ConsoleReaderThread(void * const Argument)
{
	return 0;
}
*/

unsigned int Utf8Inputter::InputterThread()
{
	DWORD Read;
	DWORD RecordsInBuffer;
	DWORD _i;
	int Length;

	for(this->AbortingThread = false; !this->AbortingThread;)
	{
		if(::WaitForSingleObject(this->Input, 100) != WAIT_OBJECT_0)
		{
			continue;
		}

		::memset(this->UnicodeBuffer, 0, sizeof(WCHAR) * this->UnicodeBufferSize);
		if(!::ReadConsoleW(this->Input, this->UnicodeBuffer, this->UnicodeBufferSize, &Read, nullptr))
		{
			continue;
		}

		Length = ::WideCharToMultiByte(CP_UTF8, 0, this->UnicodeBuffer, Read, this->Buffer, this->BufferSize, nullptr, nullptr);

		if(Length > 0)
		{
			::WriteFile(this->Output, this->Buffer, Length, &Read, nullptr);
		}
	}

	return 0;
}

unsigned int __stdcall Utf8Inputter::_InputterThread(void * const Argument)
{
	auto ReturnValue = ((Utf8Inputter *)Argument)->InputterThread();
	delete (Utf8Inputter*)Argument;
	return ReturnValue;
}
