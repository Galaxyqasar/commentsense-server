#pragma once

#ifdef LINUX
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/times.h>
#include <sys/vtimes.h>
#endif

#include <string>
#include <ctime>

namespace sys {
#ifdef LINUX
	static unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;
	static clock_t lastCPU, lastSysCPU, lastUserCPU;
	static int numProcessors;

	long long getTotalMem();
	long long getUsedMem();
	long long getFreeMem();
	long long getCurrentMem();
	float getTotalCpuUsage();
	float getCpuUsage();

	void init();
	int parseLine(char *line);
#endif

	std::string getTimeStr();
}
