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

#include <Windows.h>
#include "Utf8IO.hpp"

Utf8Buffer::Utf8Buffer(size_t BufferSize)
{
	this->Buffer = reinterpret_cast<CHAR *>(::VirtualAlloc(nullptr, BufferSize, MEM_COMMIT, PAGE_READWRITE));
	if(this->Buffer == nullptr)
	{
		throw "VIRTUALALLOC FAILURE AT Utf8Buffer";
	}
	this->BufferSize = BufferSize;
	this->BufferCursor = 0;
}

Utf8Buffer::~Utf8Buffer()
{
	::VirtualFree(this->Buffer, 0, MEM_RELEASE);
}

bool Utf8Buffer::Utf8ToUcs4(unsigned long * const ucs4, const unsigned char * const utf8, unsigned char * const bytes_read)
{
	const unsigned char * utf = utf8;
	unsigned char read = 0;
	unsigned long RC = 0;

	if((utf[0]) && ((utf[0] & 0xC0) != 0x80))
	{
		if(utf[0] < 0xC0)
		{
			RC = utf[0];
			read = 1;
		}
		else if((utf[1] & 0xC0) == 0x80)
		{
			if(utf[0] < 0xE0)
			{
				RC = (((utf[0] - 0xC0) << 6) + (utf[1] - 0x80));
				read = 2;
			}
			else if((utf[2] & 0xC0) == 0x80)
			{
				if(utf[0] < 0xF0)
				{
					RC = (((utf[0] - 0xE0) << 12) + ((utf[1] - 0x80) << 6) + (utf[2] - 0x80));
					read = 3;
				}
				else if((utf[3] & 0xC0) == 0x80)
				{
					if(utf[0] < 0xF8)
					{
						RC = (((utf[0] - 0xF0) << 18) + ((utf[1] - 0x80) << 12) + ((utf[2] - 0x80) << 6) + (utf[3] - 0x80));
						read = 4;
					}
					else if((utf[4] & 0xC0) == 0x80)
					{
						if(utf[0] < 0xFC)
						{
							RC = (((utf[0] - 0xF8) << 24) + ((utf[1] - 0x80) << 18) + ((utf[2] - 0x80) << 12) + ((utf[3] - 0x80) << 6) + (utf[4] - 0x80));
							read = 4;
						}
						else if((utf[5] & 0xC0) == 0x80)
						{
							if(utf[0] < 0xFE)
							{
								RC = (((utf[0] - 0xFC) << 30) + ((utf[1] - 0x80) << 24) + ((utf[2] - 0x80) << 18) + ((utf[3] - 0x80) << 12) + ((utf[4] - 0x80) << 6) + (utf[5] - 0x80));
								read = 5;
							}
						}
					}
				}
			}
		}
	}

	if(read)
	{
		if(bytes_read)
		{
			*bytes_read = read;
		}
		if(ucs4)
		{
			*ucs4 = RC;
		}
		return true;
	}

	return false;
}
