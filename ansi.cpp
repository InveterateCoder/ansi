//----------------------------------------ansi------------------------------------------------
#undef UNICODE
#undef _UNICODE
#include<Windows.h>

HANDLE hHeap;

bool isUnicode;

class file
{
protected:
	char fName[MAX_PATH];
	HANDLE hFile;
	DWORD fileSize;
	~file();
	LARGE_INTEGER li;
	DWORD dwTmp;
	void writeFile(char*, DWORD);
public:
	char errbuf[MAX_PATH];
	bool bRead = false;
	void assignFileName(char *buf)
	{
		strcpy_s(fName, buf);
	}
	BOOL openFile();
	void cleanFile();
};
file::~file()
{
	CloseHandle(hFile);
}
BOOL file::openFile()
{
	hFile = CreateFile(fName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, bRead == true ? OPEN_EXISTING : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		dwTmp = GetLastError();
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwTmp, NULL, errbuf, MAX_PATH, NULL);
		return TRUE;
	}
	fileSize = GetFileSize(hFile, NULL);
	return 0;
}
void file::cleanFile()
{
	li.QuadPart = 0;
	SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
	SetEndOfFile(hFile);
}
void file::writeFile(char* buf,DWORD size)
{
	WriteFile(hFile, buf, size, &dwTmp, NULL);
}


//---------------------------------------------TEXT-------------------------------------------------------
typedef enum { FIRST, SECOND }readyString;

class text : public file
{
protected:
	char *buf1, *buf2;
	readyString ready;
	DWORD *ciPt, dwReadyLength;
	char *chBuf;
	DWORD size, dwTmp;
	void alloc(DWORD s)
	{
		size = s;
		buf1 = (char*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, size);
		buf2 = (char*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, size);
	}
	void initialize(DWORD *dwP, char *chB) { ciPt = dwP; chBuf = chB; }
	void processString(int);
public:
	text(): ready(FIRST){}
	~text(){}
	BOOL readFile();
	void writeFile();
};
void text::processString(int count)
{
	if (count < 0)
	{
		*ciPt += count;
		int c = 0;
		switch (ready)
		{
		case FIRST:
			for (int i(*ciPt); count < 0; i++, c++, count++)
			{
				buf2[c] = chBuf[i];
				chBuf[i] = 0;
			}
			for (int i(0); buf1[i]; i++, c++)
			{
				buf2[c] = buf1[i];
				buf1[i] = 0;
			}
			buf2[c] = 0;
			dwReadyLength = c;
			ready = SECOND;
			break;
		case SECOND:
			for (int i(*ciPt); count < 0; i++, c++, count++)
			{
				buf1[c] = chBuf[i];
				chBuf[i] = 0;
			}
			for (int i(0); buf2[i]; i++, c++)
			{
				buf1[c] = buf2[i];
				buf2[i] = 0;
			}
			buf1[c] = 0;
			dwReadyLength = c;
			ready = FIRST;
			break;
		}
		return;
	}
	if (count > 0)
	{
		int i = 0;
		int c = 0;
		int ci = *ciPt;
		*ciPt += count;
		switch (ready)
		{
		case FIRST:
			for (; count > 0; count--, c++, ci++)
			{
				chBuf[ci] = buf1[c];
				buf1[c] = 0;
			}
			chBuf[ci] = 0;
			for (; c<dwReadyLength; c++, i++)
			{
				buf2[i] = buf1[c];
				buf1[c] = 0;
			}
			buf2[i] = 0;
			dwReadyLength = i;
			ready = SECOND;
			break;
		case SECOND:
			for (; count > 0; count--, c++, ci++)
			{
				chBuf[ci] = buf2[c];
				buf2[c] = 0;
			}
			chBuf[ci] = 0;
			for (; c<dwReadyLength; c++, i++)
			{
				buf1[i] = buf2[c];
				buf2[c] = 0;
			}
			buf1[i] = 0;
			dwReadyLength = i;
			ready = FIRST;
			break;
		}
		return;
	}
}
BOOL text::readFile()
{
	if (fileSize > size - 1)
		return FALSE;
	ReadFile(hFile, buf1, size > fileSize ? fileSize : size - 1, &dwTmp, NULL);
	dwReadyLength = dwTmp;
	ready = FIRST;
	if (IsTextUnicode(buf1, size > fileSize ? fileSize : size - 1, NULL))
		isUnicode = true;
	else
		isUnicode = false;
	return TRUE;
}
void text::writeFile()
{
	file::writeFile(chBuf, *ciPt);
	file::writeFile(ready == FIRST ? buf1 : buf2, dwReadyLength);
}

//-------------------------------------------CONSOLE------------------------------------------------
INPUT_RECORD ir;
const USHORT maxY = 500;
USHORT maxX;

class console : public text
{
protected:
	char *buffer1, *buffer2;
	char* buf;
	char* chBuf;
	DWORD ciPt;
	CONSOLE_CURSOR_INFO cci;
	HANDLE hStdIn, hStdOut, hNewCon, hClean;
	DWORD oldConsoleInputMode;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	CHAR_INFO *ci;
	COORD coord, crd, coordci;
	DWORD dwTmp;
	UINT CP, nCP;
	bool isSplitted, bEndConsole;
	char cursor[11];
	char space[2];
	void write(char*, USHORT);
	void readConsoleLine(int);
	void spacesBack(int length);
	int integ;
	size_t fS;
	size_t size;
public:
	bool bWrite;
	BOOL critError;
	console();
	~console();
	void writeCursor();
	void writeC();
	void read();
	void writeSpace();
	void backSpace();
	void del();
	void oneBack();
	void oneForward();
	void oneUp();
	void oneDown();
	void pgDown();
	void pgUp();
	void ret();
	void tab();
	void end();
	void home();
	void ifSplitted();
	BOOL readFileToCon();
	bool getfName();
	void adjust();
	bool getAnsw();
	void help();
	void rmDown();
	void rmUp();
	void rmDownPg();
	void rmUpPg();
};
console::console() : ciPt(0), isSplitted(false), bWrite(false), bEndConsole(true), critError(false)
{
	hHeap = HeapCreate(0, 0, 0);
	if (!hHeap)
	{
		critError = true;
		return;
	}
	hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hStdOut, &csbi);
	HWND hWnd = GetConsoleWindow();
	RECT rect;
	GetClientRect(hWnd, &rect);
	float res = (float)GetSystemMetrics(SM_CXSCREEN);
	maxX = ((USHORT)(res / rect.right +1)) * csbi.dwMaximumWindowSize.X;
	coordci.X = maxX + 5;
	res = (float)GetSystemMetrics(SM_CYSCREEN);
	coordci.Y = ((USHORT)(res / rect.bottom + 1)) * csbi.dwMaximumWindowSize.Y;
	ci = (CHAR_INFO*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, coordci.X*coordci.Y * sizeof(CHAR_INFO));
	size = maxY*maxX;
	chBuf = (char*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, size);
	alloc(size);
	initialize(&ciPt, chBuf);
	buf = (char*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, maxX);
	hNewCon = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	hClean = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	GetConsoleMode(hStdIn, &oldConsoleInputMode);
	SetConsoleMode(hStdIn, ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);
	SetConsoleMode(hNewCon, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
	CP = GetConsoleCP();
	//-----------------------------------------------
	buffer1 = ready == FIRST ? buf2 : buf1;
	wsprintf(buffer1, "C:\\ProgramData\\ansi_text", buf);
	if (!CreateDirectory(buffer1, NULL))
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			critError = true;
			return;
		}
	}
	wsprintf(buf, "%s\\set.txt", buffer1);
	HANDLE hLFile = CreateFile(buf, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hLFile == INVALID_HANDLE_VALUE)
	{
		critError = true;
		return;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		char num[6];
		ReadFile(hLFile, buf, maxX, &dwTmp, NULL);
		buf[dwTmp] = 0;
		int i, c;
		for (i = 0; i < 6 && buf[i] != ';'; i++)
		{
			if (buf[i] >= '0'&&buf[i] <= '9')
				num[i] = buf[i];
			else
			{
				wsprintf(buf, "only numbers allowed, restoring default parameters...\n");
				WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
				Sleep(4000);
				goto def;
			}
		}
		if (i == 6)
		{
			wsprintf(buf, "code page is not so big, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		num[i] = 0;
		char* chp, lchbuf[100];
		unsigned int ch;
		nCP = strtol(num, &chp, 10);
		CPINFO cpi;
		if (!GetCPInfo(nCP, &cpi))
		{
			wsprintf(buf, "invalid code page, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		if (cpi.MaxCharSize > 1)
		{
			wsprintf(buf, "one byte code pages only, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		wsprintf(buffer1, ";code page/default-system%c%c", 13, 10);
		for (c = 0; buf[i] != 0 && buf[i] != 10; lchbuf[c++] = buf[i++]);
		if (buf[i] == 0)
		{
			wsprintf(buf, "you messed it all up, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		lchbuf[c++] = buf[i++];
		lchbuf[c] = 0;
		if (strcmp(lchbuf, buffer1))
		{
			wsprintf(buf, "you messed it all up, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		for (c = 0; c < 4 && buf[i] != ';'&&buf[i]!=0; i++, c++)
		{
			if (buf[i] >= '0'&&buf[i] <= '9')
			{
				num[c] = buf[i];
			}
			else
			{
				wsprintf(buf, "cursor has to be a number 0 - 255, restoring default parameters...\n");
				WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
				Sleep(4000);
				goto def;
			}
		}
		if (buf[i] == 0)
		{
			wsprintf(buf, "you messed it all up, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		if (c == 4)
		{
			wsprintf(buf, "cursor has to be a number 0 - 255, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		num[c] = 0;
		chp = NULL;
		ch = strtol(num, &chp, 10);
		if (ch > 255)
		{
			wsprintf(buf, "cursor has to be a number 0 - 255, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		wsprintf(cursor, "%c0        ", ch);
		wsprintf(buffer1, ";cursor|0-255/default-177%c%c", 13, 10);
		for (c = 0; buf[i] != 0 && buf[i] != 10; lchbuf[c++] = buf[i++]);
		if (buf[i] == 0)
		{
			wsprintf(buf, "you messed it all up, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		lchbuf[c++] = buf[i++];
		lchbuf[c] = 0;
		if (strcmp(lchbuf, buffer1))
		{
			wsprintf(buf, "you messed it all up, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		for (c = 0; c < 4 && buf[i] != ';'&&buf[i]!=0; i++, c++)
		{
			if (buf[i] >= '0'&&buf[i] <= '9')
			{
				num[c] = buf[i];
			}
			else
			{
				wsprintf(buf, "space has to be a number 0 - 255(excluding 32), restoring default parameters...\n");
				WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
				Sleep(4000);
				goto def;
			}
		}
		if (c == 4)
		{
			wsprintf(buf, "space has to be a number 0 - 255(excluding 32), restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		num[c] = 0;
		ch = strtol(num, &chp, 10);
		if (ch > 255 || ch == 32)
		{
			wsprintf(buf, "space has to be a number 0 - 255(excluding 32), restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		wsprintf(space, "%c", (char)ch);
		wsprintf(buffer1, ";space(can't be 32 used by the program)0-255/default-255");
		for (c = 0; buf[i] != 0 && buf[i] != 10; lchbuf[c++] = buf[i++]);
		if (buf[i] != 0)
		{
			wsprintf(buf, "you messed it all up, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
		lchbuf[c] = buf[i];
		if (strcmp(lchbuf, buffer1))
		{
			wsprintf(buf, "you messed it all up, restoring default parameters...\n");
			WriteConsole(hStdOut, buf, strlen(buf), &dwTmp, NULL);
			Sleep(4000);
			goto def;
		}
	}
	else
	{
		def:
		wsprintf(buf, "%u;code page/default-system%c%c177;cursor|0-255/default-177%c%c255;space(can't be 32 used by the program)0-255/default-255", CP, 13, 10, 13, 10);
		SetFilePointer(hLFile, 0, NULL, FILE_BEGIN);
		SetEndOfFile(hLFile);
		WriteFile(hLFile, buf, lstrlen(buf), &dwTmp, NULL);
		nCP = 437;
		wsprintf(cursor, "%c0        ", 177);
		wsprintf(space, "%c", 255);
	}
	CloseHandle(hLFile);
	//-----------------------------------------------
	csbi.dwSize.Y = 9001;
	//csbi.dwSize.X = csbi.dwMaximumWindowSize.X;
	SetConsoleScreenBufferSize(hNewCon, csbi.dwSize);
	SetConsoleScreenBufferSize(hClean, csbi.dwSize);
	SetConsoleWindowInfo(hNewCon, TRUE, &csbi.srWindow);
	SetConsoleWindowInfo(hClean, TRUE, &csbi.srWindow);
	SetConsoleActiveScreenBuffer(hNewCon);
	SetConsoleCP(nCP);
	SetConsoleOutputCP(nCP);
	writeCursor();
	GetConsoleCursorInfo(hNewCon, &cci);
	cci.bVisible = false;
	SetConsoleCursorInfo(hNewCon, &cci);
}
console::~console()
{
	SetConsoleMode(hStdIn, oldConsoleInputMode);
	SetConsoleActiveScreenBuffer(hStdOut);
	SetConsoleCP(CP);
	SetConsoleOutputCP(CP);
	CloseHandle(hNewCon);
	CloseHandle(hClean);
	HeapDestroy(hHeap);
}
void console::write(char* buf, USHORT c)
{
	WriteConsole(hNewCon, buf, c, &dwTmp, NULL);
}
void console::writeCursor()
{
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	write(cursor, 10);
	SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
}
void console::writeC()
{
	if (ciPt + dwReadyLength >= size)
		return;
	chBuf[ciPt] = ir.Event.KeyEvent.uChar.AsciiChar;
	ciPt++;
	wsprintf(buf, "%c", ir.Event.KeyEvent.uChar.AsciiChar);
	write(buf, 1);
	bWrite = true;
	ifSplitted();
}
void console::writeSpace()
{
	if (ciPt + dwReadyLength >= size)
		return;
	chBuf[ciPt] = ir.Event.KeyEvent.uChar.AsciiChar;
	ciPt++;
	write(space, 1);
	bWrite = true;
	ifSplitted();
}
void console::backSpace()
{
	if (!ciPt)
		return;
	bWrite = true;
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	ciPt--;
	switch (chBuf[ciPt])
	{
	case 10:
		if (!isSplitted)
			WriteConsoleOutputCharacter(hNewCon, "  ", 2, csbi.dwCursorPosition, &dwTmp);
		spacesBack(0);
		chBuf[ciPt] = 0;
		chBuf[--ciPt] = 0;
		if (csbi.dwCursorPosition.X > 0)
		{
			csbi.dwCursorPosition.X--;
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			if (!isSplitted)
				WriteConsoleOutputCharacter(hNewCon, "  ", 2, csbi.dwCursorPosition, &dwTmp);
		}
		else
		{
			WriteConsoleOutputCharacter(hNewCon, " ", 1, csbi.dwCursorPosition, &dwTmp);
			csbi.dwCursorPosition.Y--;
			csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			WriteConsoleOutputCharacter(hNewCon, " ", 1, csbi.dwCursorPosition, &dwTmp);
		}
		ifSplitted();
		break;
	case '\t':
		if (!isSplitted)
			WriteConsoleOutputCharacter(hNewCon, "  ", 2, csbi.dwCursorPosition, &dwTmp);
		spacesBack(csbi.dwCursorPosition.X - 1);
		chBuf[ciPt] = 0;
		if (csbi.dwCursorPosition.X > 0)
		{
			csbi.dwCursorPosition.X--;
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			if (!isSplitted)
				WriteConsoleOutputCharacter(hNewCon, "  ", 2, csbi.dwCursorPosition, &dwTmp);
		}
		else
		{
			WriteConsoleOutputCharacter(hNewCon, " ", 1, csbi.dwCursorPosition, &dwTmp);
			csbi.dwCursorPosition.Y--;
			csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			if (!isSplitted)
				WriteConsoleOutputCharacter(hNewCon, " ", 1, csbi.dwCursorPosition, &dwTmp);
		}
		ifSplitted();
		break;
	default:
		chBuf[ciPt] = 0;
		if (csbi.dwCursorPosition.X > 0)
		{
			csbi.dwCursorPosition.X--;
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			if (!isSplitted)
				WriteConsoleOutputCharacter(hNewCon, " ", 1, csbi.dwCursorPosition, &dwTmp);
		}
		else
		{
			csbi.dwCursorPosition.Y--;
			csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			if (!isSplitted)
				WriteConsoleOutputCharacter(hNewCon, " ", 1, csbi.dwCursorPosition, &dwTmp);
		}
		ifSplitted();
		break;
	}
}
void console::del()
{
	if (!isSplitted)
		return;
	else
	{
		cci.bVisible = false;
		SetConsoleCursorInfo(hNewCon, &cci);
		switch (ready == FIRST ? buf1[0] : buf2[0])
		{
		case '\t':
			processString(1);
			chBuf[--ciPt] = 0;
			break;
		case 13:
			processString(2);
			chBuf[--ciPt] = 0;
			chBuf[--ciPt] = 0;
			break;
		default:
			processString(1);
			chBuf[--ciPt] = 0;
		}
	}
	if (dwReadyLength == 0)
	{
		isSplitted = false;
		GetConsoleScreenBufferInfo(hNewCon, &csbi);
		coord.Y = csbi.dwCursorPosition.Y + 1;
		coord.X = 0;
		FillConsoleOutputCharacter(hNewCon, ' ', csbi.dwSize.X, coord, &dwTmp);
		cci.bVisible = true;
	}
	else
		bWrite = true;
	cci.bVisible = !cci.bVisible;
	SetConsoleCursorInfo(hNewCon, &cci);
	ifSplitted();
}
void console::read()
{
	ReadConsoleInput(hStdIn, &ir, 1, &dwTmp);
}
void console::readConsoleLine(int count)
{
	coord.Y = csbi.dwCursorPosition.Y;
	coord.X = 0;
	ReadConsoleOutputCharacter(hNewCon, buf, count + 1, coord, &dwTmp);
}
void console::spacesBack(int posStart)
{
	bool rep = false;
	if (!posStart)
	{
		rep = true;
	}
	else
		readConsoleLine(posStart);
	if (rep)
	{
		csbi.dwCursorPosition.Y--;
		posStart = csbi.dwSize.X - 1;
		readConsoleLine(csbi.dwSize.X);
	}
	for (int i(posStart); i >= 0; i--)
	{
		if(buf[i]!=32)
		{
			csbi.dwCursorPosition.X = i;
			return;
		}
	}
	spacesBack(0);
}
void console::ret()
{
	if (ciPt + dwReadyLength >= size)
		return;
	bWrite = true;
	chBuf[ciPt++] = 13;
	chBuf[ciPt++] = 10;
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	FillConsoleOutputCharacter(hNewCon, ' ', csbi.dwSize.X - csbi.dwCursorPosition.X + 1, csbi.dwCursorPosition, &dwTmp);
	write("\\n\n", 3);
	Sleep(9);
	ifSplitted();
}
void console::tab()
{
	if (ciPt + dwReadyLength >= size)
		return;
	bWrite = true;
	chBuf[ciPt] = ir.Event.KeyEvent.uChar.AsciiChar;
	ciPt++;
	wsprintf(buf, "%s", "\\t\t");
	write(buf, 3);
	ifSplitted();
}
void console::oneBack()
{
	if (!ciPt)
		return;
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	switch (chBuf[ciPt - 1])
	{
	case '\t':
		spacesBack(csbi.dwCursorPosition.X - 1);
		if (csbi.dwCursorPosition.X == 0)
		{
			csbi.dwCursorPosition.Y--;
			csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
		}
		else
		{
			csbi.dwCursorPosition.X--;
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
		}
		processString(-1);
		break;
	case 10:
		spacesBack(0);
		if (csbi.dwCursorPosition.X > 0)
		{
			csbi.dwCursorPosition.X--;
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
		}
		else
		{
			csbi.dwCursorPosition.Y--;
			csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
		}
		processString(-2);
		break;
	default:
		if (csbi.dwCursorPosition.X > 0)
			csbi.dwCursorPosition.X--;
		else
		{
			csbi.dwCursorPosition.Y--;
			csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
		}
		SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
		processString(-1);
	}
	if (!isSplitted)
	{
		isSplitted = true;
		cci.bVisible = true;
		SetConsoleCursorInfo(hNewCon, &cci);
	}
}
void console::oneForward()
{
	if (!isSplitted)
		return;
	switch (ready == FIRST ? buf1[0] : buf2[0])
	{
	case '\t':
		write("\\t\t", 3);
		processString(1);
		break;
	case 13:
		write("\\n\n", 3);
		processString(2);
		break;
	default:
		write(ready == FIRST ? buf1 : buf2, 1);
		processString(1);
	}
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
	if (!dwReadyLength)
	{
		isSplitted = false;
		cci.bVisible = false;
		SetConsoleCursorInfo(hNewCon, &cci);
		ifSplitted();
	}
}
void console::oneDown()
{
	if (!isSplitted)
		return;
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	crd = csbi.dwCursorPosition;
	//cci.bVisible = false;
	//SetConsoleCursorInfo(hNewCon, &cci);
	integ = 0;
	buffer1 = ready == FIRST ? buf1 : buf2;
	for (int i(0); i < dwReadyLength && (csbi.dwCursorPosition.Y == crd.Y || csbi.dwCursorPosition.X < crd.X); i++)
	{
		switch (buffer1[i])
		{
		case '\t':
			write("\\t\t", 3);
			integ++;
			break;
		case 13:
			write("\\n\n", 3);
			integ += 2;
			i++;
			break;
		default:
			write(buffer1 + i, 1);
			integ++;
		}
		GetConsoleScreenBufferInfo(hNewCon, &csbi);
		if (csbi.dwCursorPosition.Y > crd.Y && buffer1[i+1] == 13)
			break;
	}
	//cci.bVisible = true;
	//SetConsoleCursorInfo(hNewCon, &cci);
	SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
	processString(integ);
	if (!dwReadyLength)
	{
		isSplitted = false;
		cci.bVisible = false;
		SetConsoleCursorInfo(hNewCon, &cci);
		ifSplitted();
	}
}
void console::oneUp()
{
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	integ = 0;
	if (csbi.dwCursorPosition.Y == 0)
	{
		if (csbi.dwCursorPosition.X > 0)
		{
			//cci.bVisible = false;
			//SetConsoleCursorInfo(hNewCon, &cci);
			while (csbi.dwCursorPosition.X != 0)
			{
				switch (chBuf[ciPt + integ - 1])
				{
				case '\t':
					spacesBack(csbi.dwCursorPosition.X - 1);
					if (csbi.dwCursorPosition.X == 0)
					{
						csbi.dwCursorPosition.Y--;
						csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
						SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
					}
					else
					{
						csbi.dwCursorPosition.X--;
						SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
					}
					integ--;
					break;
				case 10:
					spacesBack(0);
					if (csbi.dwCursorPosition.X > 0)
					{
						csbi.dwCursorPosition.X--;
						SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
					}
					else
					{
						csbi.dwCursorPosition.Y--;
						csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
						SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
					}
					integ -= 2;
					break;
				default:
					if (csbi.dwCursorPosition.X > 0)
						csbi.dwCursorPosition.X--;
					else
					{
						csbi.dwCursorPosition.Y--;
						csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
					}
					SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
					integ--;
				}
			}
			processString(integ);
			cci.bVisible = true;
			SetConsoleCursorInfo(hNewCon, &cci);
			if (!isSplitted)
				isSplitted = true;
		}
		return;
	}
	crd = csbi.dwCursorPosition;
	//cci.bVisible = false;
	//SetConsoleCursorInfo(hNewCon, &cci);
	while (csbi.dwCursorPosition.Y == crd.Y || csbi.dwCursorPosition.X > crd.X)
	{
		switch (chBuf[ciPt + integ - 1])
		{
		case '\t':
			spacesBack(csbi.dwCursorPosition.X - 1);
			if (csbi.dwCursorPosition.X == 0)
			{
				csbi.dwCursorPosition.Y--;
				csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			else
			{
				csbi.dwCursorPosition.X--;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			integ--;
			break;
		case 10:
			spacesBack(0);
			if (csbi.dwCursorPosition.X > 0)
			{
				csbi.dwCursorPosition.X--;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			else
			{
				csbi.dwCursorPosition.Y--;
				csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			integ -= 2;
			break;
		default:
			if (csbi.dwCursorPosition.X > 0)
				csbi.dwCursorPosition.X--;
			else
			{
				csbi.dwCursorPosition.Y--;
				csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
			}
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			integ--;
		}
	}
	processString(integ);
	if (!isSplitted)
	{
		isSplitted = true;
		cci.bVisible = true;
		SetConsoleCursorInfo(hNewCon, &cci);
	}
}
void console::pgDown()
{
	if (!isSplitted)
		return;
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	crd = csbi.dwCursorPosition;
	cci.bVisible = false;
	SetConsoleCursorInfo(hNewCon, &cci);
	integ = 0;
	buffer1 = ready == FIRST ? buf1 : buf2;
	for (int i(0), check = csbi.srWindow.Bottom - csbi.srWindow.Top + crd.Y; i < dwReadyLength && (csbi.dwCursorPosition.Y < check || csbi.dwCursorPosition.X < crd.X); i++)
	{
		switch (buffer1[i])
		{
		case '\t':
			write("\\t\t", 3);
			integ++;
			break;
		case 13:
			write("\\n\n", 3);
			integ += 2;
			i++;
			break;
		default:
			write(buffer1 + i, 1);
			integ++;
		}
		GetConsoleScreenBufferInfo(hNewCon, &csbi);
		if (csbi.dwCursorPosition.Y >= check&&csbi.dwCursorPosition.Y > crd.Y && buffer1[i + 1] == 13)
			break;
	}
	cci.bVisible = true;
	SetConsoleCursorInfo(hNewCon, &cci);
	SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
	processString(integ);
	if (!dwReadyLength)
	{
		isSplitted = false;
		cci.bVisible = false;
		SetConsoleCursorInfo(hNewCon, &cci);
		ifSplitted();
	}
}
void console::pgUp()
{
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	integ = 0;
	if (csbi.dwCursorPosition.Y == 0)
	{
		if (csbi.dwCursorPosition.X > 0)
		{
			cci.bVisible = false;
			SetConsoleCursorInfo(hNewCon, &cci);
			while (csbi.dwCursorPosition.X != 0)
			{
				switch (chBuf[ciPt + integ - 1])
				{
				case '\t':
					spacesBack(csbi.dwCursorPosition.X - 1);
					if (csbi.dwCursorPosition.X == 0)
					{
						csbi.dwCursorPosition.Y--;
						csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
						SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
					}
					else
					{
						csbi.dwCursorPosition.X--;
						SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
					}
					integ--;
					break;
				case 10:
					spacesBack(0);
					if (csbi.dwCursorPosition.X > 0)
					{
						csbi.dwCursorPosition.X--;
						SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
					}
					else
					{
						csbi.dwCursorPosition.Y--;
						csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
						SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
					}
					integ -= 2;
					break;
				default:
					if (csbi.dwCursorPosition.X > 0)
						csbi.dwCursorPosition.X--;
					else
					{
						csbi.dwCursorPosition.Y--;
						csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
					}
					SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
					integ--;
				}
			}
			processString(integ);
			cci.bVisible = true;
			SetConsoleCursorInfo(hNewCon, &cci);
			if (!isSplitted)
				isSplitted = true;
		}
		return;
	}
	crd = csbi.dwCursorPosition;
	cci.bVisible = false;
	SetConsoleCursorInfo(hNewCon, &cci);
	int check = crd.Y - (csbi.srWindow.Bottom - csbi.srWindow.Top);
	if (check < 0)
		check = 0;
	while (csbi.dwCursorPosition.Y > check || csbi.dwCursorPosition.X > crd.X)
	{
		switch (chBuf[ciPt + integ - 1])
		{
		case '\t':
			spacesBack(csbi.dwCursorPosition.X - 1);
			if (csbi.dwCursorPosition.X == 0)
			{
				csbi.dwCursorPosition.Y--;
				csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			else
			{
				csbi.dwCursorPosition.X--;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			integ--;
			break;
		case 10:
			spacesBack(0);
			if (csbi.dwCursorPosition.X > 0)
			{
				csbi.dwCursorPosition.X--;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			else
			{
				csbi.dwCursorPosition.Y--;
				csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			integ -= 2;
			break;
		default:
			if (csbi.dwCursorPosition.X > 0)
				csbi.dwCursorPosition.X--;
			else
			{
				csbi.dwCursorPosition.Y--;
				csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
			}
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			integ--;
		}
	}
	processString(integ);
	cci.bVisible = true;
	SetConsoleCursorInfo(hNewCon, &cci);
	if (!isSplitted)
		isSplitted = true;
}
void console::end()
{
	if (!isSplitted )
		return;
	buffer1 = ready == FIRST ? buf1 : buf2;
	integ = 0;
	if (buffer1[0] == 13)
		return;
	cci.bVisible = false;
	SetConsoleCursorInfo(hNewCon, &cci);
	for (int i(0); i < dwReadyLength && buffer1[i] != 13; i++)
	{
		switch (buffer1[i])
		{
		case '\t':
			write("\\t\t", 3);
			integ++;
			break;
		case 13:
			write("\\n\n", 3);
			integ += 2;
			break;
		default:
			write(buffer1 + i, 1);
			integ++;
		}
	}
	processString(integ);
	if (!dwReadyLength)
	{
		isSplitted = false;
		cci.bVisible = false;
		SetConsoleCursorInfo(hNewCon, &cci);
		ifSplitted();
	}
	else
	{
		cci.bVisible = true;
		SetConsoleCursorInfo(hNewCon, &cci);
	}
}
void console::home()
{
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	if (csbi.dwCursorPosition.X == 0 && csbi.dwCursorPosition.Y == 0)
		return;
	if (chBuf[ciPt - 1] == 10)
		return;
	cci.bVisible = false;
	SetConsoleCursorInfo(hNewCon, &cci);
	integ = 0;
	while (chBuf[ciPt + integ - 1] != 10 && !(csbi.dwCursorPosition.X == 0 && csbi.dwCursorPosition.Y == 0))
	{
		switch (chBuf[ciPt + integ - 1])
		{
		case '\t':
			spacesBack(csbi.dwCursorPosition.X - 1);
			if (csbi.dwCursorPosition.X == 0)
			{
				csbi.dwCursorPosition.Y--;
				csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			else
			{
				csbi.dwCursorPosition.X--;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			integ--;
			break;
		case 10:
			spacesBack(0);
			if (csbi.dwCursorPosition.X > 0)
			{
				csbi.dwCursorPosition.X--;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			else
			{
				csbi.dwCursorPosition.Y--;
				csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
				SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			}
			integ -= 2;
			break;
		default:
			if (csbi.dwCursorPosition.X > 0)
				csbi.dwCursorPosition.X--;
			else
			{
				csbi.dwCursorPosition.Y--;
				csbi.dwCursorPosition.X = csbi.dwSize.X - 1;
			}
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			integ--;
		}
	}
	cci.bVisible = true;
	SetConsoleCursorInfo(hNewCon, &cci);
	processString(integ);
	if (!isSplitted)
		isSplitted = true;
}
void console::ifSplitted()
{
	if (isSplitted)
	{
		if (bWrite)
		{
			GetConsoleScreenBufferInfo(hNewCon, &csbi);
			ReadConsoleOutput(hNewCon, ci, coordci, { 0,0 }, &csbi.srWindow);
			WriteConsoleOutput(hClean, ci, coordci, { 0,0 }, &csbi.srWindow);
			SetConsoleWindowInfo(hClean, TRUE, &csbi.srWindow);
			SetConsoleActiveScreenBuffer(hClean);
			SetConsoleScreenBufferSize(hClean, csbi.dwSize);
			cci.bVisible = false;
			SetConsoleCursorInfo(hNewCon, &cci);
			buffer2 = ready == FIRST ? buf1 : buf2;
			buffer1 = ready == FIRST ? buf2 : buf1;
			int c = 0, n = 0;
			for (int i(0); i < dwReadyLength; i++, c++)
			{
				switch (buffer2[i])
				{
				case 13:
					n++;
					buffer1[c++] = '\\';
					buffer1[c++] = 'n';
					buffer1[c++] = buffer2[i++];
					buffer1[c] = buffer2[i];
					break;
				case '\t':
					buffer1[c++] = '\\';
					buffer1[c++] = 't';
					buffer1[c] = buffer2[i];
					break;
				case ' ':
					buffer1[c] = space[0];
					break;
				default:
					buffer1[c] = buffer2[i];
				}
			}
			buffer1[c++] = '\\';
			buffer1[c++] = '0';
			buffer1[c] = 32;
			FillConsoleOutputCharacter(hNewCon, ' ', c + ((n + 1)*csbi.dwSize.X), csbi.dwCursorPosition, &dwTmp);
			write(buffer1, c);
			SetConsoleCursorPosition(hNewCon, csbi.dwCursorPosition);
			SetConsoleWindowInfo(hNewCon, TRUE, &csbi.srWindow);
			SetConsoleActiveScreenBuffer(hNewCon);
			if (!bRead)
			{
				cci.bVisible = true;
				SetConsoleCursorInfo(hNewCon, &cci);
			}
		}
	}
	else
	{
		writeCursor();
	}
	bWrite = false;
}
BOOL console::readFileToCon()
{
	if (!readFile())
	{
		WriteConsole(hStdOut, "File is too big.", 16, &dwTmp, NULL);
		return FALSE;
	}
	if (isUnicode)
	{
		WriteConsole(hStdOut, "Sorry but i have chosen to be one byte ansi", 43, &dwTmp, NULL);
		return FALSE;
	}
	if (dwReadyLength > 0)
	{
		isSplitted = true;
		bWrite = true;
	}
	ifSplitted();
	return TRUE;
}
bool console::getfName()
{
	SetConsoleActiveScreenBuffer(hStdOut);
	SetConsoleMode(hStdIn, oldConsoleInputMode);
	som:
	WriteConsole(hStdOut, "write file? y/n: ", 17, &dwTmp, NULL);
	ReadConsole(hStdIn, buf, MAX_PATH, &dwTmp, NULL);
	switch (buf[0])
	{
	case 'y':
		WriteConsole(hStdOut, "[full path] or [name](to write in current directory): ", 54, &dwTmp, NULL);
		ReadConsole(hStdIn, buf, MAX_PATH, &dwTmp, NULL);
		buf[dwTmp - 2] = buf[dwTmp - 1] = 0;
		if (buf[1] == ':'&&buf[2] == '\\')
			assignFileName(buf);
		else
		{
			buffer1 = ready == FIRST ? buf2 : buf1;
			GetCurrentDirectory(MAX_PATH, buffer1);
			wsprintf(buffer1, "%s\\", buffer1);
			strcat_s(buffer1, MAX_PATH - strlen(buffer1), buf);
			assignFileName(buffer1);
		}
		return true;
	case 'n':
		return false;
	default:
		goto som;
	}
}
bool console::getAnsw()
{
	SetConsoleActiveScreenBuffer(hStdOut);
	SetConsoleMode(hStdIn, oldConsoleInputMode);
	ag:
	WriteConsole(hStdOut, "write file? y/n: ", 17, &dwTmp, NULL);
	ReadConsole(hStdIn, buf, MAX_PATH, &dwTmp, NULL);
	switch (buf[0])
	{
	case 'y':
		return true;
	case 'n':
		return false;
	default:
		goto ag;
	}
}
void console::adjust()
{
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	if (csbi.dwSize.Y < 9001)
	{
		csbi.dwSize.Y = 9001;
		SetConsoleScreenBufferSize(hNewCon, csbi.dwSize);
		SetConsoleScreenBufferSize(hClean, csbi.dwSize);
		cci.bVisible = false;
		SetConsoleCursorInfo(hNewCon, &cci);
		coord.X = 0;
		coord.Y = 0;
		SetConsoleCursorPosition(hNewCon, coord);
		buffer1 = ready == FIRST ? buf2 : buf1;
		int c = 0, n = 0;
		for (int i(0); i < ciPt; i++, c++)
		{
			switch (chBuf[i])
			{
			case 13:
				n++;
				buffer1[c++] = '\\';
				buffer1[c++] = 'n';
				buffer1[c++] = chBuf[i++];
				buffer1[c] = chBuf[i];
				break;
			case '\t':
				buffer1[c++] = '\\';
				buffer1[c++] = 't';
				buffer1[c] = chBuf[i];
				break;
			case ' ':
				buffer1[c] = space[0];
				break;
			default:
				buffer1[c] = chBuf[i];
			}
		}
		FillConsoleOutputCharacter(hNewCon, ' ', c + ((n + 1)*csbi.dwSize.X), coord, &dwTmp);
		write(buffer1, c);
		bWrite = true;
		ifSplitted();
	}
	else
	{
		csbi.dwSize.Y = 9001;
		SetConsoleScreenBufferSize(hNewCon, csbi.dwSize);
		SetConsoleScreenBufferSize(hClean, csbi.dwSize);
		bWrite = true;
		ifSplitted();
	}
}
void console::help()
{
	SetConsoleActiveScreenBuffer(hStdOut);
	SetConsoleMode(hStdIn, oldConsoleInputMode);
	buffer1 = ready == FIRST ? buf2 : buf1;
	wsprintf(buffer1, "\nUsage:\tansi [option] [target_name]\n\n\
Options:\n\t/set settings\tSettings Access\n\t-r read\t\tRead Only.\n\t-c create\tCreate New File\n\n\
Examples:\n\tansi /set\n\tansi text.txt\n\tansi -r text.txt\n\tansi -c C:\\Users\\User\\Desktop\\text.txt\n\n\
Hot Keys:\n\tctrl+x\texit with prompt to save\n\tctrl+s\tsave while editing file\n\n");
	WriteConsole(hStdOut, buffer1, strlen(buffer1), &dwTmp, NULL);
}
void console::rmDown()
{
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	csbi.srWindow.Top += 3;
	csbi.srWindow.Bottom += 3;
	SetConsoleWindowInfo(hNewCon, TRUE, &csbi.srWindow);
}
void console::rmUp()
{
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	csbi.srWindow.Top -= 3;
	csbi.srWindow.Bottom -= 3;
	SetConsoleWindowInfo(hNewCon, TRUE, &csbi.srWindow);
}
void console::rmDownPg()
{
	USHORT sh;
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	sh = (USHORT)(short)((float)(csbi.srWindow.Bottom - csbi.srWindow.Top) / 1.1);
	csbi.srWindow.Top += sh;
	csbi.srWindow.Bottom += sh;
	SetConsoleWindowInfo(hNewCon, TRUE, &csbi.srWindow);
}
void console::rmUpPg()
{
	USHORT sh;
	GetConsoleScreenBufferInfo(hNewCon, &csbi);
	sh = (USHORT)(short)((float)(csbi.srWindow.Bottom - csbi.srWindow.Top) / 1.1);
	csbi.srWindow.Top -= sh;
	csbi.srWindow.Bottom -= sh;
	SetConsoleWindowInfo(hNewCon, TRUE, &csbi.srWindow);
}


//------------------------------------------------MAIN-----------------------------------------------------
int main(int argc, char** argv)
{
	DWORD dwTmp;
	char originName[MAX_PATH];
	char buf[MAX_PATH];
	console con;
	if (con.critError)
		return 0;
	GetConsoleTitle(originName, 150);
	wsprintf(buf, "cmd :ansi %s", argv[1]);
	SetConsoleTitle(buf);
	bool bCtrl = false;
	switch (argc)
	{
	case 1:
		while (true)
		{
			con.read();
			if (ir.EventType == KEY_EVENT)
			{
				if (ir.Event.KeyEvent.bKeyDown == TRUE)
				{
					if ((ir.Event.KeyEvent.wVirtualKeyCode >= 0x30 && ir.Event.KeyEvent.wVirtualKeyCode <= 0x39) ||
						(ir.Event.KeyEvent.wVirtualKeyCode >= 0x41 && ir.Event.KeyEvent.wVirtualKeyCode <= 0x5A) ||
						(ir.Event.KeyEvent.wVirtualKeyCode >= 0x60 && ir.Event.KeyEvent.wVirtualKeyCode <= 0x6F) ||
						(ir.Event.KeyEvent.wVirtualKeyCode >= 0xBA && ir.Event.KeyEvent.wVirtualKeyCode <= 0xC0) ||
						(ir.Event.KeyEvent.wVirtualKeyCode >= 0xDB && ir.Event.KeyEvent.wVirtualKeyCode <= 0xDF))
					{
						if (bCtrl)
						{
							if (ir.Event.KeyEvent.wVirtualKeyCode == 0x58)
							{
							sasa:
								if (!con.getfName())
									return 0;
								else
								{
									if (con.openFile())
									{
										WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), con.errbuf, lstrlen(con.errbuf), &dwTmp, NULL);
										goto sasa;
									}
									else
									{
										con.cleanFile();
										con.writeFile();
										return 0;
									}
								}
							}
							else
								continue;
						}
						con.writeC();
					}
					else
					{
						switch (ir.Event.KeyEvent.wVirtualKeyCode)
						{
						case VK_CONTROL:
							bCtrl = true;
							break;
						case VK_SPACE:
							con.writeSpace();
							break;
						case VK_BACK:
							con.backSpace();
							break;
						case VK_DELETE:
							con.del();
							break;
						case VK_RETURN:
							con.ret();
							break;
						case VK_TAB:
							con.tab();
							break;
						case VK_LEFT:
							con.oneBack();
							break;
						case VK_RIGHT:
							con.oneForward();
							break;
						case VK_UP:
							con.oneUp();
							break;
						case VK_DOWN:
							con.oneDown();
							break;
						case VK_HOME:
							con.home();
							break;
						case VK_END:
							con.end();
							break;
						case VK_PRIOR://page Up
							con.pgUp();
							break;
						case VK_NEXT://page Dn
							con.pgDown();
							break;
						default:
							break;
						}
					}
				}
				else
				{
					if (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL)
						bCtrl = false;
				}
			}
			else
				if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT)
				{
					con.adjust();
				}
		}
		break;
	case 2:
		switch (argv[1][0])
		{
		case '/':
			if (!strcmp(argv[1] + 1, "set"))
			{
				con.assignFileName("C:\\ProgramData\\ansi_text\\set.txt");
				con.bRead = true;
				goto pr;
			}
			con.help();
			break;
		default:
			con.bRead = true;
			if (argv[1][1] == ':'&&argv[1][2] == '\\')
				con.assignFileName(argv[1]);
			else
			{
				GetCurrentDirectory(MAX_PATH, buf);
				strcat_s(buf, "\\");
				if (strcat_s(buf, MAX_PATH - lstrlen(buf), argv[1]))
				{	
					return 0;
				}
				con.assignFileName(buf);
			}
			pr:
			if (con.openFile())
			{
				WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), con.errbuf, lstrlen(con.errbuf), &dwTmp, NULL);
				return 0;
			}
			con.bRead = false;
			if (!con.readFileToCon())
				return 0;
			while (true)
			{
				con.read();
				if (ir.EventType == KEY_EVENT)
				{
					if (ir.Event.KeyEvent.bKeyDown == TRUE)
					{
						if ((ir.Event.KeyEvent.wVirtualKeyCode >= 0x30 && ir.Event.KeyEvent.wVirtualKeyCode <= 0x39) ||
							(ir.Event.KeyEvent.wVirtualKeyCode >= 0x41 && ir.Event.KeyEvent.wVirtualKeyCode <= 0x5A) ||
							(ir.Event.KeyEvent.wVirtualKeyCode >= 0x60 && ir.Event.KeyEvent.wVirtualKeyCode <= 0x6F) ||
							(ir.Event.KeyEvent.wVirtualKeyCode >= 0xBA && ir.Event.KeyEvent.wVirtualKeyCode <= 0xC0) ||
							(ir.Event.KeyEvent.wVirtualKeyCode >= 0xDB && ir.Event.KeyEvent.wVirtualKeyCode <= 0xDF))
						{
							if (bCtrl)
							{
								if (ir.Event.KeyEvent.wVirtualKeyCode == 0x58)
								{
									if (con.getAnsw())
									{
										con.cleanFile();
										con.writeFile();
									}
									return 0;
								}
								else if (ir.Event.KeyEvent.wVirtualKeyCode == 0x53)
								{
									con.cleanFile();
									con.writeFile();
									continue;
								}
								else
									continue;
							}
							con.writeC();
						}
						else
						{
							switch (ir.Event.KeyEvent.wVirtualKeyCode)
							{
							case VK_CONTROL:
								bCtrl = true;
								break;
							case VK_SPACE:
								con.writeSpace();
								break;
							case VK_BACK:
								con.backSpace();
								break;
							case VK_DELETE:
								con.del();
								break;
							case VK_RETURN:
								con.ret();
								break;
							case VK_TAB:
								con.tab();
								break;
							case VK_LEFT:
								con.oneBack();
								break;
							case VK_RIGHT:
								con.oneForward();
								break;
							case VK_UP:
								con.oneUp();
								break;
							case VK_DOWN:
								con.oneDown();
								break;
							case VK_HOME:
								con.home();
								break;
							case VK_END:
								con.end();
								break;
							case VK_PRIOR://page Up
								con.pgUp();
								break;
							case VK_NEXT://page Dn
								con.pgDown();
								break;
							default:
								break;
							}
						}
					}
					else
					{
						if (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL)
							bCtrl = false;
					}
				}
				else
					if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT)
					{
						con.adjust();
					}
			}
		}
		break;
	case 3:
		switch (argv[1][0])
		{
		case '-':
			if (argv[1][1] == 'r')
			{
				if (argv[1][2] != 0)
				{
					con.help();
					break;
				}
				con.bRead = true;
				switch (argv[2][0])
				{
				case '/':
					con.help();
					break;
				default:
					if (argv[2][1] == ':'&&argv[2][2] == '\\')
						con.assignFileName(argv[2]);
					else
					{
						GetCurrentDirectory(MAX_PATH, buf);
						strcat_s(buf, "\\");
						if (strcat_s(buf, MAX_PATH - lstrlen(buf), argv[2]))
						{
							return 0;
						}
						con.assignFileName(buf);
					}
					if (con.openFile())
					{
						WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), con.errbuf, lstrlen(con.errbuf), &dwTmp, NULL);
						return 0;
					}
					if (!con.readFileToCon())
						return 0;

					while (true)
					{
						con.read();
						if (ir.EventType == KEY_EVENT)
						{
							if (ir.Event.KeyEvent.bKeyDown == TRUE)
							{
								if (bCtrl)
								{
									if (ir.Event.KeyEvent.wVirtualKeyCode == 0x58)
									{
										return 0;
									}
								}
								switch (ir.Event.KeyEvent.wVirtualKeyCode)
								{
								case VK_CONTROL:
									bCtrl = true;
									break;
								case VK_UP:
									con.rmUp();
									break;
								case VK_DOWN:
									con.rmDown();
									break;
								case VK_PRIOR:
									con.rmUpPg();
									break;
								case VK_NEXT:
									con.rmDownPg();
									break;
								}
							}
							else
							{
								if (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL)
									bCtrl = false;
							}
						}
						else
							if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT)
							{
								con.adjust();
							}
					}
				}
				break;
			}
			else if (argv[1][1] == 'c')
			{
				if (argv[1][2] != 0)
				{
					con.help();
					break;
				}
				switch (argv[2][0])
				{
				case '/':
					con.help();
					break;
				default:
					if (argv[2][1] == ':'&&argv[2][2] == '\\')
						con.assignFileName(argv[2]);
					else
					{
						GetCurrentDirectory(MAX_PATH, buf);
						strcat_s(buf, "\\");
						if (strcat_s(buf, MAX_PATH - lstrlen(buf), argv[2]))
						{
							return 0;
						}
						con.assignFileName(buf);
					}
					if (con.openFile())
					{
						WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), con.errbuf, lstrlen(con.errbuf), &dwTmp, NULL);
						return 0;
					}
					if (!con.readFileToCon())
						return 0;
					while (true)
					{
						con.read();
						if (ir.EventType == KEY_EVENT)
						{
							if (ir.Event.KeyEvent.bKeyDown == TRUE)
							{
								if ((ir.Event.KeyEvent.wVirtualKeyCode >= 0x30 && ir.Event.KeyEvent.wVirtualKeyCode <= 0x39) ||
									(ir.Event.KeyEvent.wVirtualKeyCode >= 0x41 && ir.Event.KeyEvent.wVirtualKeyCode <= 0x5A) ||
									(ir.Event.KeyEvent.wVirtualKeyCode >= 0x60 && ir.Event.KeyEvent.wVirtualKeyCode <= 0x6F) ||
									(ir.Event.KeyEvent.wVirtualKeyCode >= 0xBA && ir.Event.KeyEvent.wVirtualKeyCode <= 0xC0) ||
									(ir.Event.KeyEvent.wVirtualKeyCode >= 0xDB && ir.Event.KeyEvent.wVirtualKeyCode <= 0xDF))
								{
									if (bCtrl)
									{
										if (ir.Event.KeyEvent.wVirtualKeyCode == 0x58)
										{
											if (con.getAnsw())
											{
												con.cleanFile();
												con.writeFile();
											}
											return 0;
										}
										else if (ir.Event.KeyEvent.wVirtualKeyCode == 0x53)
										{
											con.cleanFile();
											con.writeFile();
											continue;
										}
										else
											continue;
									}
									con.writeC();
								}
								else
								{
									switch (ir.Event.KeyEvent.wVirtualKeyCode)
									{
									case VK_CONTROL:
										bCtrl = true;
										break;
									case VK_SPACE:
										con.writeSpace();
										break;
									case VK_BACK:
										con.backSpace();
										break;
									case VK_DELETE:
										con.del();
										break;
									case VK_RETURN:
										con.ret();
										break;
									case VK_TAB:
										con.tab();
										break;
									case VK_LEFT:
										con.oneBack();
										break;
									case VK_RIGHT:
										con.oneForward();
										break;
									case VK_UP:
										con.oneUp();
										break;
									case VK_DOWN:
										con.oneDown();
										break;
									case VK_HOME:
										con.home();
										break;
									case VK_END:
										con.end();
										break;
									case VK_PRIOR://page Up
										con.pgUp();
										break;
									case VK_NEXT://page Dn
										con.pgDown();
										break;
									default:
										break;
									}
								}
							}
							else
							{
								if (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL)
									bCtrl = false;
							}
						}
						else
							if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT)
							{
								con.adjust();
							}
					}
				}
				break;
			}
			con.help();
			break;
		default:
			con.help();
		}
		break;
	default:
		con.help();
	}
	SetConsoleTitle(originName);
	return 0;
}