#include "GDebugger.h"

int GDebugger::WriteToLog(char *fmt,...){


	FILE* __fStdOut = NULL;
	HANDLE __hStdOut = NULL;

	if(_bConsoleOutput)
		__hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	else
		__fStdOut = fopen(_outFile,"a");
	
	if(__fStdOut == NULL && __hStdOut == NULL)
		return 0;

	char s[300];
	va_list argptr;
	int cnt;

	va_start(argptr, fmt);
	cnt = vsprintf(s, fmt, argptr);
	va_end(argptr);

	DWORD cCharsWritten;

	if(__hStdOut)
		WriteConsole(__hStdOut, s, strlen(s), &cCharsWritten, NULL);

	if(__fStdOut){
		fprintf(__fStdOut, s);
		fclose(__fStdOut);
	}
	return(cnt);
}