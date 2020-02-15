#include "sys.hpp"

namespace sys {
#ifdef LINUX
	long long getTotalMem(){
		struct sysinfo memInfo;
		sysinfo (&memInfo);
		long long totalPhysMem = memInfo.totalram;
		//Multiply in next statement to avoid int overflow on right hand side...
		totalPhysMem *= memInfo.mem_unit;
		return totalPhysMem;
	}
	long long getUsedMem(){
		struct sysinfo memInfo;
		sysinfo (&memInfo);
		long long physMemUsed = memInfo.totalram - memInfo.freeram;
		//Multiply in next statement to avoid int overflow on right hand side...
		physMemUsed *= memInfo.mem_unit;
		return physMemUsed;
	}
	long long getFreeMem(){
		struct sysinfo memInfo;
		sysinfo (&memInfo);
		long long physMemFree = memInfo.freeram;
		//Multiply in next statement to avoid int overflow on right hand side...
		physMemFree *= memInfo.mem_unit;
		return physMemFree;
	}
	long long getCurrentMem(){
	    FILE* file = fopen("/proc/self/status", "r");
	    int result = -1;
	    char line[128];

	    while (fgets(line, 128, file) != NULL){
        	if (strncmp(line, "VmRSS:", 6) == 0){
	            result = parseLine(line);
	            break;
	        }
	    }
	    fclose(file);
	    return result*1024;
	}
	float getTotalCpuUsage(){
	    float percent;
	    FILE* file;
	    unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;

	    file = fopen("/proc/stat", "r");
	    int r = fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow, &totalSys, &totalIdle);
	    fclose(file);

	    if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
	        totalSys < lastTotalSys || totalIdle < lastTotalIdle){
	        //Overflow detection. Just skip this value.
	        percent = -1.0;
	    }
	    else{
	        total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
	            (totalSys - lastTotalSys);
	        percent = total;
	        total += (totalIdle - lastTotalIdle);
	        percent /= total;
	        percent *= 100;
	    }

	    lastTotalUser = totalUser;
	    lastTotalUserLow = totalUserLow;
	    lastTotalSys = totalSys;
	    lastTotalIdle = totalIdle;

	    return percent;
	}
	float getCpuUsage(){
	    struct tms timeSample;
	    clock_t now;
	    float percent;

	    now = times(&timeSample);
	    if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
	        timeSample.tms_utime < lastUserCPU){
	        //Overflow detection. Just skip this value.
	        percent = -1.0;
	    }
	    else{
	        percent = (timeSample.tms_stime - lastSysCPU) +
	            (timeSample.tms_utime - lastUserCPU);
	        percent /= (now - lastCPU);
	        percent /= numProcessors;
	        percent *= 100;
	    }
	    lastCPU = now;
	    lastSysCPU = timeSample.tms_stime;
	    lastUserCPU = timeSample.tms_utime;

	    return percent;
	}

	void init(){
	    FILE* file = fopen("/proc/stat", "r");
	    int r = fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow, &lastTotalSys, &lastTotalIdle);
	    fclose(file);
	    ////
        struct tms timeSample;
	    char line[128];

	    lastCPU = times(&timeSample);
	    lastSysCPU = timeSample.tms_stime;
	    lastUserCPU = timeSample.tms_utime;

	    file = fopen("/proc/cpuinfo", "r");
	    numProcessors = 0;
	    while(fgets(line, 128, file) != NULL){
	        if (strncmp(line, "processor", 9) == 0) numProcessors++;
	    }
	    fclose(file);
	}

	int parseLine(char *line){
	    // This assumes that a digit will be found and the line ends in " Kb".
	    int i = strlen(line);
	    const char* p = line;
	    while (*p <'0' || *p > '9') p++;
	    line[i-3] = '\0';
	    i = atoi(p);
	    return i;
	}
#endif

	std::string getTimeStr(){
		time_t rawtime;
		std::string result(128, '\0');

		time(&rawtime);
		struct tm *timeinfo = localtime(&rawtime);

		strftime(&result[0], result.length(), "%d.%m.%Y %H:%M:%S", timeinfo);
		return result.c_str();
	}
}