#ifndef __GDEBUGGER_HEADER__
#define __GDEBUGGER_HEADER__

#include "Singleton.h"


#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

#pragma once

#define DBGLINE GDebugger::GetSingleton().WriteToLog("Line: %d of %s\n",__LINE__,__FILE__);
#define dprintf GDebugger::GetSingleton().WriteToLog

class GDebugger : public HK::Singleton<GDebugger> {
	friend class HK::Singleton<GDebugger>;
protected:
	GDebugger(): _bConsoleOutput(false){
		sprintf(_outFile,"GGeneralDbg.txt");
		FILE *stream = fopen(_outFile,"w");
		fclose(stream);
	}
	~GDebugger(){
	}
public:
	void ChangeFile(char *str){
		sprintf(_outFile,"%s",str);
		FILE *stream = fopen(_outFile,"w");
		fclose(stream);
	}
	int WriteToLog(char *fmt,...);
private:
	bool _bConsoleOutput;
	char _outFile[256];
};
#endif // __GDEBUGGER_HEADER__