
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

#if (MESSAGE)
#define CPU_MSG(msg...)		{ printf("[CPU MSG] : " msg); }
#else
#define CPU_MSG(msg...)		do {} while (0)
#endif 

#if (DEBUG)
#define CPU_DBG(msg...)		{ printf("[CPU DBG] : " msg); }
#else
#define CPU_DBG(msg...)		do {} while (0)
#endif 

/*
 * name		: cpu_rate_prepare
 * usage 	: Get CPU and Process Task stat info 
 *
 * param 	:
 * 		[in]
 *  			id 		= Process ID 
 * 		[out]
 *  			info 	= CPU Process infomation.
 *
 * return 	:	0 = success, -1 = not exist task, -2 = fail parsing Proc Info -3 invalid struct info 
 */

int cpu_rate_prepare( unsigned int id, struct cpu_rate_info *info)
{
	
	int ret;
	char *fret;
	char buf[255];
	char string[30];
	char procbuf[1000];
	char cpu[5];
	/* CPU Info */
	unsigned long nice, user, sys, idle;
	/* for parsing process info */
	int pid_pid ,  pid_ppid, pid_pgrp, pid_session,  pid_tty_nr, tpgid;
	char pid_comm[128];
	char pid_state;
	long pid_flags, pid_minfit, pid_cminflt, pid_majflt,pid_cmajflt, pid_utime , pid_stime;
	unsigned long pid_cutime, pid_cstime, pid_priority, pid_nice;


	FILE *cpuinfo;
	FILE *procinfo;

	if(info  == NULL)
	{
		CPU_MSG(" info is NULL \n");
		return -3;
	}

	/* Get Cpu info */
	char *s = string;
	cpuinfo = fopen("/proc/stat","r");
	if(cpuinfo== NULL)
	{
		CPU_MSG(" Open fail CPU Info \n");
		return -2;
	}

	fret = fgets(buf,255,cpuinfo);
	if(fret == NULL)
	{
		CPU_MSG(" Get fail CPU INFO \n");
		return -2;
	}

	ret = sscanf(buf,"%s %lu %lu %lu %lu", cpu, &user, &sys, &nice, &idle);
	if(ret < 0)
	{
		CPU_MSG(" Parsing fail CPU INFO \n");
		return -2;
	}
	CPU_DBG("%lu %lu %lu %lu\n", user, sys, nice, idle);
	
	/* Get Proc Info */

	s += sprintf(s,"/proc/%d/task/%d/stat",id,id);
	procinfo = fopen(string,"r");	

	sleep(1);
	if(procinfo == NULL)
	{
		CPU_MSG(" Open fail PID Proc Info \n");
		return -2;
	}
	fret = fgets(procbuf,1000,procinfo);
	if(fret == NULL)
	{
		CPU_MSG(" Get fail PID Proc INFO \n");
		return -2;
	}
	ret = sscanf(procbuf,"%d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld",
		&pid_pid , pid_comm, &pid_state, &pid_ppid, &pid_pgrp, &pid_session, &pid_tty_nr, &tpgid,
		&pid_flags, &pid_minfit, &pid_cminflt, &pid_majflt,&pid_cmajflt, &pid_utime ,&pid_stime,
		&pid_cutime, &pid_cstime, &pid_priority, &pid_nice
		);
	if(ret < 0)
	{
		CPU_MSG(" Parsing PID Proc CPU INFO \n");
		return -2;
	}

	CPU_DBG("%d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld\n",
		pid_pid , pid_comm, pid_state, pid_ppid, pid_pgrp, pid_session, pid_tty_nr, pid_tty_nr,
		pid_flags, pid_minfit, pid_cminflt, pid_majflt, pid_cmajflt, pid_utime, pid_stime,
		pid_cutime, pid_cstime, pid_priority, pid_nice
		);
	info->cpu_user = user;
	info->cpu_sys = sys;
	info->cpu_nice = nice;
	info->cpu_idle = idle;

	info->pid_utime = pid_utime;	
	info->pid_stime = pid_stime;	

	fclose(cpuinfo);
	fclose(procinfo);

	return 0;
		
}

/*
 * name		: cpu_rate_calc
 * usage 	: Update CPU & Process task stat info 
 *			  calulate Pid process use CPU uttiliztion rate.
 * param 	:
 * 		[in]
 *  			id 			= Process ID 
 * 		[out]
 *  			info 		= CPU Process infomation.
 *				user_rate 	= user use cpu rate. 
 * 				sys_rate 	= system use cpu rate.
 *				total_rate	= total use cpu rate.
 *
 * return 	:	0 = success, -1 = not exist task, -2 = fail parsing Proc Info -3 invalid struct info
 */
int cpu_rate_calc(int id, struct cpu_rate_info *info, float *user_rate, float *sys_rate, float *total_rate)
{
	
	int ret;
	char *fret;
	char cbuf[255];
	char string[20];
	char procbuf[1000];
	char cpu[5];
	/* CPU Info */
	unsigned long nice, user, sys, idle;
	/* for parsing process info */
	int pid_pid ,  pid_ppid, pid_pgrp, pid_session,  pid_tty_nr, tpgid;
	char pid_comm[128];
	char pid_state;
	long pid_flags, pid_minfit, pid_cminflt, pid_majflt,pid_cmajflt, pid_utime , pid_stime;
	unsigned long pid_cutime, pid_cstime, pid_priority, pid_nice;
	char *s = string;
	/* Caculate CPU Utilization rate */
	unsigned long pre_cpusum;
	unsigned long cur_cpusum;
	unsigned long cur_utime,cur_stime;
	unsigned long cpu_total;
	
	float user_use, system_use, total_use;

	FILE *cpuinfo;
	FILE *procinfo;

	if(info  == NULL)
	{
		CPU_MSG(" info is NULL \n");
		return -3;
	}

	/* Get Cpu info */
	
	cpuinfo = fopen("/proc/stat","r");
	if(cpuinfo == NULL)
	{
		CPU_MSG(" Open fail CPU Info \n");
		return -2;
	}

	fret = fgets(cbuf,255,cpuinfo);
	if(fret == NULL)
	{
		CPU_MSG(" Get fail CPU INFO \n");
		return -2;
	}

	ret = sscanf(cbuf,"%s %lu %lu %lu %lu",cpu ,&user, &sys, &nice, &idle);
	if(ret <= 0)
	{
		CPU_MSG(" Parsing fail CPU INFO \n");
		return -2;
	}
	CPU_DBG("%s %lu %lu %lu %lu\n",cpu, user, sys, nice, idle);

	/* Get Proc Info */

	s += sprintf(s,"/proc/%d/task/%d/stat",id,id);

	procinfo = fopen(string,"r");	
	if(procinfo == NULL)
	{
		CPU_MSG(" Open fail PID Proc Info \n");
		return -2;
	}
	fret = fgets(procbuf,1000,procinfo);
	if(fret == NULL)
	{
		CPU_MSG(" Get fail PID Proc INFO \n");
		return -2;
	}
	printf("TTT\n");
	ret = sscanf(procbuf,"%d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld",
		&pid_pid , pid_comm, &pid_state, &pid_ppid, &pid_pgrp, &pid_session, &pid_tty_nr, &tpgid,
		&pid_flags, &pid_minfit, &pid_cminflt, &pid_majflt,&pid_cmajflt, &pid_utime ,&pid_stime,
		&pid_cutime, &pid_cstime, &pid_priority, &pid_nice
		);
	if(ret <= 0)
	{
		CPU_MSG(" Parsing PID Proc CPU INFO \n");
		return -2;
	}
	CPU_DBG("%d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld\n",
		pid_pid , pid_comm, pid_state, pid_ppid, pid_pgrp, pid_session, pid_tty_nr, pid_tty_nr,
		pid_flags, pid_minfit, pid_cminflt, pid_majflt, pid_cmajflt, pid_utime, pid_stime,
		pid_cutime, pid_cstime, pid_priority, pid_nice
		);

	pre_cpusum = info->cpu_user + info->cpu_sys + info->cpu_nice + info->cpu_idle;
	cur_cpusum = user + sys + nice + idle;
	cpu_total = cur_cpusum -  pre_cpusum;

	cur_utime = pid_utime - info->pid_utime;
	cur_stime = pid_stime - info->pid_stime ;

	user_use = ((float)cur_utime/(float) cpu_total)*100 ;
	system_use = ((float)cur_stime/(float) cpu_total)*100 ;
	total_use = user_use + system_use;

	if(user_rate != NULL)
		*user_rate = user_use;
	if(sys_rate != NULL)
		*sys_rate = system_use;
	if(total_rate != NULL)
		*total_rate = total_use;

	/* Update CPU Info */

	info->cpu_user = user;
	info->cpu_sys = sys;
	info->cpu_nice = nice;
	info->cpu_idle = idle;

	info->pid_utime = pid_utime;	
	info->pid_stime = pid_stime;	
	
	fclose(cpuinfo);
	fclose(procinfo);

	return 0;
}

