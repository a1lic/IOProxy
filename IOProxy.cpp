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
#include <process.h>
#include <Windows.h>
#include "Utf8IO.hpp"

extern "C" const _TCHAR * GetNextToken(const _TCHAR * const Str)
{
	const _TCHAR * _Str;
	_TCHAR PrevChar;
	bool IsInToken;
	bool Spacing;

	for(PrevChar = _T('\0'), IsInToken = false, Spacing = false, _Str = Str; *_Str; _Str++)
	{
		switch(*_Str)
		{
		case _T('"'):
			IsInToken = !IsInToken;
			break;
		case _T(' '):
			if(!IsInToken)
			{
				Spacing = true;
			}
			break;
		default:
			if(Spacing)
			{
				if(PrevChar == _T('"'))
				{
					_Str--;
				}
				return _Str;
			}
			break;
		}
		PrevChar = *_Str;
	}

	return nullptr;
}

extern "C" int _tmain(int const argc, const _TCHAR * const argv[])
{
	SECURITY_ATTRIBUTES Attribute;
	STARTUPINFO Startup;
	PROCESS_INFORMATION Process;
	Utf8Outputter * Utf8OutC, * Utf8ErrC;
	Utf8Inputter * Utf8InC;
	HANDLE Utf8OutReader, Utf8ErrReader, Utf8InWriter;
	HANDLE Utf8Out, Utf8Err, Utf8In;
	HANDLE Threads[3];
	const _TCHAR * NewArg;
	_TCHAR * ArgDup;
	BOOL BoolResult;
	unsigned int ThreadId;
	unsigned char _i;

	if(argc <= 1)
	{
		return 1;
	}

	NewArg = ::GetNextToken(::GetCommandLine());
	::_tprintf_s(_T("Next token : %s\n"), NewArg);

	ArgDup = ::_tcsdup(NewArg);

	// パイプのハンドルを継承できるように属性を設定する
	::memset(&Attribute, 0, sizeof(SECURITY_ATTRIBUTES));
	Attribute.nLength = sizeof(SECURITY_ATTRIBUTES);
	Attribute.bInheritHandle = TRUE;

	// 子プロセスの標準入出力に使用するパイプを作成する
	::CreatePipe(&Utf8OutReader, &Utf8Out, &Attribute, 0);
	::CreatePipe(&Utf8ErrReader, &Utf8Err, &Attribute, 0);
	::CreatePipe(&Utf8In, &Utf8InWriter, &Attribute, 0);

	// 子プロセスの標準入出力を設定する
	::memset(&Startup, 0, sizeof(STARTUPINFO));
	Startup.cb = sizeof(STARTUPINFO);
	Startup.dwFlags = STARTF_USESTDHANDLES;
	Startup.hStdOutput = Utf8Out;
	Startup.hStdError = Utf8Err;
	Startup.hStdInput = Utf8In;

	// 子プロセスを起動する
	BoolResult = ::CreateProcess(nullptr, ArgDup, nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &Startup, &Process);
	// この時点で子プロセスに渡したハンドルは必要ないので破棄する
	::CloseHandle(Utf8In);
	::CloseHandle(Utf8Out);
	::CloseHandle(Utf8Err);
	if(BoolResult)
	{
		// スレッドのハンドルは不要なので閉じる
		::CloseHandle(Process.hThread);

		// このクラスはスレッドの引数として渡され、スレッドの終了時に自動的に解放される
		Utf8OutC = new Utf8Outputter(Utf8OutReader, 65536, false);
		Utf8ErrC = new Utf8Outputter(Utf8ErrReader, 65536, true);
		Utf8InC = new Utf8Inputter(Utf8InWriter, 65536);

		// スレッドをハンドル毎に生成する
		Threads[0] = reinterpret_cast<HANDLE>(::_beginthreadex(nullptr, 0, Utf8Outputter::_OutputterThread, Utf8OutC, 0, &ThreadId));
		Threads[1] = reinterpret_cast<HANDLE>(::_beginthreadex(nullptr, 0, Utf8Outputter::_OutputterThread, Utf8ErrC, 0, &ThreadId));
		Threads[2] = reinterpret_cast<HANDLE>(::_beginthreadex(nullptr, 0, Utf8Inputter::_InputterThread, Utf8InC, 0, &ThreadId));

		// プロセスが終わるまで待機
		::WaitForSingleObject(Process.hProcess, INFINITE);
		// プロセスのハンドルを破棄する
		::CloseHandle(Process.hProcess);

		// スレッドに終了を通知する
		Utf8OutC->AbortThread();
		Utf8ErrC->AbortThread();
		Utf8InC->AbortThread();

		::WaitForMultipleObjects(3, Threads, TRUE, INFINITE);
		for(_i = 0; _i < 3; _i++)
		{
			::CloseHandle(Threads[_i]);
		}
	}

	return 0;
}
