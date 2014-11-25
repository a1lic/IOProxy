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
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <process.h>
#include <stdio.h>
#include <locale.h>
#include <Windows.h>
#include <Psapi.h>
#include "Utf8IO.hpp"

const _TCHAR * GetNextToken(const _TCHAR * const Str)
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

extern "C" void ColorCodeToAttribute(CONSOLE_SCREEN_BUFFER_INFOEX * const Info, bool IsBackground, int Color)
{
	WORD Attr;

	if(Color & 0x8)
	{
		Attr = IsBackground ? BACKGROUND_INTENSITY : FOREGROUND_INTENSITY;
	}
	else
	{
		Attr = 0;
	}

	switch(Color & 0x7)
	{
	case 1:
		Attr |= IsBackground ? BACKGROUND_BLUE : FOREGROUND_BLUE;
		break;
	case 2:
		Attr |= IsBackground ? BACKGROUND_GREEN : FOREGROUND_GREEN;
		break;
	case 3:
		Attr |= IsBackground ? (BACKGROUND_BLUE | BACKGROUND_GREEN) : (FOREGROUND_BLUE | FOREGROUND_GREEN);
		break;
	case 4:
		Attr |= IsBackground ? BACKGROUND_RED : FOREGROUND_RED;
		break;
	case 5:
		Attr |= IsBackground ? (BACKGROUND_BLUE | BACKGROUND_RED) : (FOREGROUND_BLUE | FOREGROUND_RED);
		break;
	case 6:
		Attr |= IsBackground ? (BACKGROUND_GREEN | BACKGROUND_RED) : (FOREGROUND_GREEN | FOREGROUND_RED);
		break;
	case 7:
		Attr |= IsBackground ? (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED) : (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
		break;
	}

	if(IsBackground)
	{
		Info->wAttributes &= ~(BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY);
	}
	else
	{
		Info->wAttributes &= ~(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	}

	Info->wAttributes |= Attr;
}

extern "C" void SetConsoleStyle()
{
	HANDLE _h;
	TCHAR * ConsoleAttributes;
	const TCHAR * ConsoleAttribute;
	TCHAR * _s;
	long Attr;
	CONSOLE_SCREEN_BUFFER_INFOEX _c;

	ConsoleAttributes = reinterpret_cast<TCHAR*>(::calloc(256, sizeof(TCHAR)));
#if defined(_DEBUG)
	::SetEnvironmentVariable(TEXT("CONSOLE_ATTRIBUTES"), TEXT("F0 B15 W160 w160"));
#endif
	if(::GetEnvironmentVariable(TEXT("CONSOLE_ATTRIBUTES"), ConsoleAttributes, 256) > 0)
	{
		::SetEnvironmentVariable(TEXT("CONSOLE_ATTRIBUTES"), nullptr);
		_h = ::GetStdHandle(STD_OUTPUT_HANDLE);
		::memset(&_c, 0, sizeof(CONSOLE_SCREEN_BUFFER_INFOEX));
		_c.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
		::GetConsoleScreenBufferInfoEx(_h, &_c);

		for(ConsoleAttribute = ConsoleAttributes; ConsoleAttribute; ConsoleAttribute = ::GetNextToken(ConsoleAttribute))
		{
			switch(ConsoleAttribute[0] /* & 0x5F*/)
			{
			case 'B':
			_set_bg:
				Attr = ::_tcstol(&ConsoleAttribute[1], &_s, 10);
				::ColorCodeToAttribute(&_c, true, Attr);
				break;
			case 'F':
			_set_fg:
				Attr = ::_tcstol(&ConsoleAttribute[1], &_s, 10);
				::ColorCodeToAttribute(&_c, false, Attr);
				break;
			case 'H':
				Attr = ::_tcstol(&ConsoleAttribute[1], &_s, 10);
				_c.dwSize.Y = (SHORT)Attr;
				break;
			case 'W':
				Attr = ::_tcstol(&ConsoleAttribute[1], &_s, 10);
				_c.dwSize.X = (SHORT)Attr;
				break;
			case 'b':
				goto _set_bg;
			case 'f':
				goto _set_fg;
			case 'h':
				Attr = ::_tcstol(&ConsoleAttribute[1], &_s, 10);
				_c.dwMaximumWindowSize.Y = (SHORT)Attr;
				_c.srWindow.Bottom = (SHORT)Attr;
				break;
			case 'w':
				Attr = ::_tcstol(&ConsoleAttribute[1], &_s, 10);
				_c.dwMaximumWindowSize.X = (SHORT)Attr;
				_c.srWindow.Right = (SHORT)Attr;
				break;
			}
		}

		::SetConsoleScreenBufferInfoEx(_h, &_c);
	}

	::free(ConsoleAttributes);
}

extern "C" void InitializeTerminal()
{
	int _fd;
	HANDLE _h;
	DWORD _m;
	int _stdin_tty;

#if !defined(_UNICODE)
	/* Unicode以外の場合はロケールを設定 */
	::setlocale(LC_ALL, "");
#endif

#if defined(_UNICODE)
	/* Unicodeの場合はストリームをUnicodeにする(ファイルの場合はUTF-8に) */
	_fd = ::_fileno(stderr);
	::_setmode(_fd, ::_isatty(_fd) ? _O_U16TEXT : _O_U8TEXT);

	_fd = ::_fileno(stdout);
	::_setmode(_fd, ::_isatty(_fd) ? _O_U16TEXT : _O_U8TEXT);
#endif

	_fd = ::_fileno(stdin);
	_stdin_tty = ::_isatty(_fd);
#if defined(_UNICODE)
	/* 入力もUnicodeに */
	::_setmode(_fd, _stdin_tty ? _O_U16TEXT : _O_U8TEXT);
#endif

	/* 入力がコンソール入力の場合はマウスの入力を無効化、これにより右クリックメニューが有効になる */
	if(_stdin_tty)
	{
		_h = (HANDLE)::_get_osfhandle(_fd);
		if(::GetConsoleMode(_h, &_m))
		{
			if(_m & ENABLE_MOUSE_INPUT)
			{
				_m &= ~ENABLE_MOUSE_INPUT;
				::SetConsoleMode(_h, _m);
			}
		}
	}

	::SetConsoleStyle();
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
	_TCHAR * ProcessName;
	_TCHAR * ProcessFileName;
	BOOL BoolResult;
	unsigned int ThreadId;
	unsigned char _i;

	if(argc <= 1)
	{
		return 1;
	}

	::InitializeTerminal();

	NewArg = ::GetNextToken(::GetCommandLine());
	//::_tprintf_s(_T("Next token : %s\n"), NewArg);

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
	ArgDup = ::_tcsdup(NewArg);
	BoolResult = ::CreateProcess(nullptr, ArgDup, nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &Startup, &Process);
	// この時点で子プロセスに渡したハンドルは必要ないので破棄する
	::CloseHandle(Utf8In);
	::CloseHandle(Utf8Out);
	::CloseHandle(Utf8Err);
	::free(ArgDup);
	if(BoolResult)
	{
		// スレッドのハンドルは不要なので閉じる
		::CloseHandle(Process.hThread);

		ProcessName = new TCHAR[1024];
		::GetProcessImageFileName(Process.hProcess, ProcessName, 1024);
		ProcessFileName = ::_tcsrchr(ProcessName, _T('\\'));
		if(ProcessFileName != nullptr)
		{
			ProcessFileName++;
		}
		::SetConsoleTitle(ProcessFileName);
		delete[] ProcessName;

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
