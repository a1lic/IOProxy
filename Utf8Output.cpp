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

#include <tchar.h>
#include <malloc.h>
#include <Windows.h>
#include "Utf8IO.hpp"

Utf8Outputter::Utf8Outputter(HANDLE Input, size_t BufferSize, bool IsStdError) : Utf8Buffer(BufferSize)
{
	// バッファーから有効なUTF-8とみなされたバイト列を格納するバッファー
	// このバッファーの内容は画面に出力が行われるごとに無効化される
	this->ValidUtf8Buffer = reinterpret_cast<CHAR *>(::VirtualAlloc(nullptr, BufferSize, MEM_COMMIT, PAGE_READWRITE));
	if(this->ValidUtf8Buffer == nullptr)
	{
		throw "VIRTUALALLOC FAILURE AT Utf8Outputter";
	}

	this->Input = Input;
	this->OutToStdErr = IsStdError;
}

Utf8Outputter::~Utf8Outputter()
{
	::VirtualFree(this->ValidUtf8Buffer, 0, MEM_RELEASE);
	::CloseHandle(this->Input);
}

void Utf8Outputter::WriteToConsole()
{
	WCHAR * UnicodeString;
	DWORD _w;
	int Length;
	int Utf8Length;

	// 定期的にバッファを確認し、有効なUTF-8のバイトシーケンスを取り出す
	if(!this->CopyValidUtf8())
	{
		return;
	}

	// 有効なバイトシーケンスが取り出せた場合は出力
	Utf8Length = static_cast<int>(::strlen(this->ValidUtf8Buffer));
	Length = ::MultiByteToWideChar(CP_UTF8, 0, this->ValidUtf8Buffer, Utf8Length, nullptr, 0);
	if(Length > 0)
	{
		UnicodeString = new WCHAR[Length];
		::MultiByteToWideChar(CP_UTF8, 0, this->ValidUtf8Buffer, Utf8Length, UnicodeString, Length);
		::WriteConsoleW(::GetStdHandle(STD_OUTPUT_HANDLE), UnicodeString, Length, &_w, nullptr);
		delete[] UnicodeString;
	}
}

bool Utf8Outputter::GetValidRange()
{
	ptrdiff_t ValidStart;
	ptrdiff_t Offset;
	ptrdiff_t EndOffset;
	unsigned char SequenceLength;
	unsigned char CharBuffer[8];

	EndOffset = this->BufferSize - this->BufferCursor;
	for(ValidStart = this->BufferCursor; ValidStart <= EndOffset; ValidStart++)
	{
		::memset(CharBuffer, 0, 8);
		::memcpy(CharBuffer, &Buffer[ValidStart], min(6, EndOffset - ValidStart));
		if(Utf8Buffer::Utf8ToUcs4(nullptr, CharBuffer, &SequenceLength))
		{
			break;
		}
	}
	if(ValidStart > EndOffset)
	{
		this->Range = 0;
		return false;
	}

	this->RangeStart = ValidStart;

	this->Range = 0;
	for(Offset = ValidStart; Offset <= EndOffset; Offset += SequenceLength)
	{
		::memset(CharBuffer, 0, 8);
		::memcpy(CharBuffer, &Buffer[Offset], min(6, EndOffset - Offset));
		if(!Utf8Buffer::Utf8ToUcs4(nullptr, CharBuffer, &SequenceLength))
		{
			break;
		}
		this->Range += SequenceLength;
	}

	return true;
}

bool Utf8Outputter::CopyValidUtf8()
{
	CHAR * PtrBuffer;

	::memset(this->ValidUtf8Buffer, 0, this->BufferSize);

	if(this->GetValidRange())
	{
		PtrBuffer = &this->Buffer[this->RangeStart];
		::memcpy(this->ValidUtf8Buffer, PtrBuffer, this->Range);

		this->BufferCursor += this->Range;
	}

	return (this->Range > 0);
}

unsigned int Utf8Outputter::OutputterThread()
{
	OVERLAPPED Overlapped;
	CHAR * BufferStart;
	DWORD BufferCurrentSize;
	DWORD BytesInBuffer;
	DWORD PrevBytesInBuffer;

	// 剰余バッファを初期化する
	this->Remaining = 0;
	::memset(this->Remains, 0, REMAINS_BUFFER_BYTES);

	for(this->AbortingThread = false; !this->AbortingThread;)
	{
		// 入力バッファーの初期化
		::memset(Buffer, 0, this->BufferSize);
		this->BufferCursor = 0;

		if(this->Remaining > 0)
		{
			if(this->Remaining >= REMAINS_BUFFER_BYTES)
			{
				throw "REMAINING BUFFER TOO LARGE";
			}

			// 前回の読み込みで余ったバイトシーケンスをバッファーに書き込む
			::memcpy(Buffer, this->Remains, this->Remaining);
			this->BufferCursor = this->Remaining;

			// 剰余バッファのクリア
			::memset(this->Remains, 0, REMAINS_BUFFER_BYTES);
			this->Remaining = 0;
		}

		::memset(&Overlapped, 0, sizeof(OVERLAPPED));
		// 完了ルーチンの引数に自身を渡す
		// イベントハンドルとして宣言されているが、内部では無視されているとドキュメントに記載されている
		Overlapped.hEvent = this;

		// 非同期読み込み開始
		BufferStart = &this->Buffer[this->BufferCursor];
		BufferCurrentSize = static_cast<DWORD>(this->BufferSize - this->BufferCursor);
		if(!::ReadFileEx(this->Input, BufferStart, BufferCurrentSize, &Overlapped, Utf8Outputter::OutputComplete))
		{
			// 非同期処理に失敗した場合はやり直し
			::OutputDebugString(TEXT("ReadFileEx failed.\n"));
			::Sleep(100);
			continue;
		}

		PrevBytesInBuffer = 0;
		for(this->Cancelled = false; !this->Cancelled; ::Sleep(50))
		{
			if(this->AbortingThread)
			{
				if(!this->Cancelled)
				{
					// スレッドの終了要求が来たらI/Oを終了してループを脱出する
					::CancelIo(this->Input);
				}
				break;
			}

			if(::GetOverlappedResult(this->Input, &Overlapped, &BytesInBuffer, FALSE))
			{
				if((BytesInBuffer > 0) && (BytesInBuffer == PrevBytesInBuffer))
				{
					if(!this->Cancelled)
					{
						// これ以上ストリームにデータが無い場合はI/Oを終わらせる
						::CancelIo(this->Input);
					}
					break;
				}

				PrevBytesInBuffer = BytesInBuffer;
				this->WriteToConsole();
			}
		}
	}

	return 0;
}

VOID CALLBACK Utf8Outputter::OutputComplete(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	auto * _this = reinterpret_cast<Utf8Outputter *>(lpOverlapped->hEvent);

	// I/O完了ルーチンが呼ばれていた場合は現在のReadFileExを終了して新たにReadFileExを開始する
	_this->Remaining = 0;
	::memcpy(_this->Remains, &_this->Buffer[_this->BufferCursor], dwNumberOfBytesTransfered - _this->BufferCursor);

	_this->Cancelled = true;
}

unsigned int __stdcall Utf8Outputter::_OutputterThread(void * const Argument)
{
	auto ReturnValue = ((Utf8Outputter *)Argument)->OutputterThread();
	delete reinterpret_cast<Utf8Outputter *>(Argument);
	return ReturnValue;
}
