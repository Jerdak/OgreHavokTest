#pragma once

#ifndef __UTOOL_UTIL__
#define __UTOOL_UTIL__

#include <windows.h>
#include <direct.h>
#include <stdio.h>
#include <tchar.h>
//String functions
char *append_strings(const char *str1, const char *str2);
char *append_strings(const char *str1, const char *str2, const char *str3);
char *copy_string(const char *str);
char *string_to_tcl(const char *str);
char *tail(const char *name);

//directory functions
char *gritty_resolve(char *path);

//display functions
int dprintf(char *fmt, ...);
int dprintf(int,char *fmt,...);
int log_errors(char *fmt, ...);

void clear_logs();

//Misc functions
UINT HexToDec(const TCHAR *szValue);


//Global Variables
extern char *sUtilLog;
extern char sErrorLog[256];
extern bool _LOG;
extern int _VERBOSITY;

#endif