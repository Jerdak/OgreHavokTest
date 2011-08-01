#pragma once
#include <windows.h>

#ifndef THEOC
	#define log_errors printf
#else
	#include "util.h"

#endif
#ifdef _TIMER_USE_MACROS
	#define TIMER_BEGIN(NAME) ProfTimer NAME;\
							NAME.Start(); \
							double NAME##d1, NAME##d2;

	#define TIMER_BEFORE(NAME) NAME.Stop();\
							   NAME##d1 = NAME.GetDurationInSecs();

	#define TIMER_AFTER(NAME) NAME.Stop();\
							NAME##d2 = NAME.GetDurationInSecs();

	#define TIMER_OUTPUT(NAME) printf("Delay of %s timer: %f\n",#NAME,NAME##d2-NAME##d1);
	#define TIMER_OUTPUT_MSG(NAME,MSG) log_errors("%s: %f\n",#MSG,NAME##d2-NAME##d1);
	#define TIMER_GET(NAME) (NAME##d2-NAME##d1)

	#define TIMER_CLASS_INIT(NAME)	ProfTimer NAME;\
									double NAME##d1, NAME##d2;
	#define TIMER_CLASS_START(NAME) NAME.Start();

#else
	#define TIMER_BEGIN(NAME)
	#define TIMER_BEFORE(NAME)
	#define TIMER_AFTER(NAME)
	#define TIMER_OUTPUT(NAME)
#endif

struct ProfTimer {
    void Start(void) {
		//Make sure we are using a single core on a dual core machine, otherwise timings will be off.
		GetProcessAffinityMask(GetCurrentProcess(),&procAffMask,&sysAffMask);
		SetThreadAffinityMask(GetCurrentThread(), 1);

		if(!QueryPerformanceFrequency(&ticksPerSecond))printf("Hires timer freq. not supported\n");
        if(!QueryPerformanceCounter(&tick))printf("Hires timer counter not supported\n");

		SetThreadAffinityMask(GetCurrentThread(),procAffMask);
    };
    void Stop(void) {
		//Make sure we are using a single core on a dual core machine, otherwise timings will be off.
		GetProcessAffinityMask(GetCurrentProcess(),&procAffMask,&sysAffMask);
		SetThreadAffinityMask(GetCurrentThread(), 1);
        if(!QueryPerformanceCounter(&tock))printf("Hires timer counter not supported\n");

		SetThreadAffinityMask(GetCurrentThread(),procAffMask);
    };
    double GetDurationInSecs(void)
    {
        double duration = (double)(tock.QuadPart-tick.QuadPart)/(double)ticksPerSecond.QuadPart;
		return duration;
    }
    LONGLONG GetDurationInMSecs(void)
    {
		LONGLONG duration = (tock.QuadPart % ticksPerSecond.QuadPart)-(tick.QuadPart % ticksPerSecond.QuadPart);
        return duration;
    }
  	LARGE_INTEGER ticksPerSecond;
	LARGE_INTEGER tick;   // A point in time
	LARGE_INTEGER tock;
	LARGE_INTEGER time;   // For converting tick into real time
	
	DWORD procAffMask,sysAffMask;
};