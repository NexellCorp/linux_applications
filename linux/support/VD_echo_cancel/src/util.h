#ifndef UTIL_H
#define UTIL_H

#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>

/***************************************************************************************/
#define	__STATIC__	static

/***************************************************************************************/
#define	RUN_TIMESTAMP_US(s) {						\
		struct timeval tv;							\
		gettimeofday(&tv, NULL);					\
		s = (tv.tv_sec*1000000) + (tv.tv_usec);	\
	}

#define	END_TIMESTAMP_US(s, d)	{					\
		struct timeval tv;							\
		gettimeofday(&tv, NULL);					\
		d = (tv.tv_sec*1000000) + (tv.tv_usec);	\
		d = d - s;							\
	}

/***************************************************************************************/
struct list_entry {
    struct list_entry *next, *prev;
    void *data;
};
#define LIST_HEAD_INIT(name, data) { &(name), &(name), data }
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

__STATIC__ inline void INIT_LIST_HEAD(struct list_entry *list, void *data)
{
    list->next = list;
    list->prev = list;
    list->data = data;
}

__STATIC__ inline void __list_add(struct list_entry *list,
					struct list_entry *prev, struct list_entry *next)
{
    next->prev = list, list->next = next;
    list->prev = prev, prev->next = list;
}

/* add to tail */
__STATIC__ inline void list_add(struct list_entry *list, struct list_entry *head)
{
	__list_add(list, head->prev, head);
}

/***************************************************************************************/
#ifdef SUPPORT_RT_SCHEDULE
#include <sched.h> 			/* schedule */
#include <sys/resource.h>
#include <linux/sched.h> 	/* SCHED_NORMAL, SCHED_FIFO, SCHED_RR, SCHED_BATCH */

// set_new_scheduler(getpid(), SCHED_FIFO, 99)
__STATIC__ int set_new_scheduler(pid_t pid, int policy, int priority)
{
	struct sched_param param;
	int maxpri = 0, minpri = 0;
	int ret;

	switch(policy) {
		case SCHED_NORMAL:
		case SCHED_FIFO:
		case SCHED_RR:
		case SCHED_BATCH:	break;
		default:
			printf("invalid policy %d (0~3)\n", policy);
			return -1;
	}

	if(SCHED_NORMAL == policy) {
		maxpri =  20;	// #define NICE_TO_PRIO(nice)	(MAX_RT_PRIO + (nice) + 20), MAX_RT_PRIO 100
		minpri = -20;	// nice
	} else {
		maxpri = sched_get_priority_max(policy);
		minpri = sched_get_priority_min(policy);
	}

	if((priority > maxpri) || (minpri > priority)) {
		printf("E: invalid priority (%d ~ %d)...\n", minpri, maxpri);
		return -1;
	}

	if(SCHED_NORMAL == policy) {
		param.sched_priority = 0;
		ret = sched_setscheduler(pid, policy, &param);
		if(ret) {
			printf("E: set scheduler (%d) %s \n", ret, strerror(errno));
			return ret;
		}
		ret = setpriority(PRIO_PROCESS, pid, priority);
		if(ret)
			printf("E: set priority (%d) %s \n", ret, strerror(errno));
	} else {
		param.sched_priority = priority;
		ret = sched_setscheduler(pid, policy, &param);
		if(ret)
			printf("E: set scheduler (%d) %s \n", ret, strerror(errno));
	}
	return ret;
}
#endif

/***************************************************************************************/
__STATIC__ pid_t gettid(void)
{
	return syscall(__NR_gettid);
}

#include <execinfo.h>

#define BT_ARRAY_SIZE	256
__STATIC__ void sig_handler(int sig)
{
	void *array[BT_ARRAY_SIZE];
	size_t size;	// get void*'s for all entries on the stack

	size = backtrace(array, BT_ARRAY_SIZE);	// print out all the frames to stderr
	fprintf(stderr, "\n\nError: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, 2);	// #include <execinfo.h>

	signal(SIGSEGV, SIG_DFL);
	raise (SIGSEGV);
}

/***************************************************************************************/
static int sys_write(const char *file, const char *buffer, int buflen)
{
	int fd, ret = 0;

	if (0 != access(file, F_OK)) {
		printf("E: cannot access file (%s).\n", file);
		return -errno;
	}

	fd = open(file, O_RDWR|O_SYNC);
	if (0 > fd) {
		printf("E: cannot open file (%s).\n", file);
		return -errno;
	}

	if (0 > write(fd, buffer, buflen)) {
		printf("E: write (file=%s, data=%s)\n", file, buffer);
		ret = -errno;
	}

	close(fd);
	return ret;
}

static int sys_read(const char *file, char *buffer, int buflen)
{
	int fd, ret = 0;

	if (0 != access(file, F_OK)) {
		printf("E: cannot access file (%s).\n", file);
		return -errno;
	}

	fd = open(file, O_RDONLY);
	if (0 > fd) {
		printf("E: cannot open file (%s).\n", file);
		return -errno;
	}

	ret = read(fd, buffer, buflen);
	if (0 > ret) {
		printf("E: read (file=%s, data=%s)\n", file, buffer);
		ret = -errno;
	}

	close(fd);
	return ret;
}

#define CPU_FREQUENCY_PATH	"/sys/devices/system/cpu"
static int cpufreq_set_speed(long khz)
{
	char path[128], data[32];

	/*
	 * change governor to userspace
	 */
	sprintf(path, "%s/cpu0/cpufreq/scaling_governor", CPU_FREQUENCY_PATH);
	if (0 > sys_write(path, "userspace", strlen("userspace")))
		return -errno;

	/*
	 *	change cpu frequency
	 */
	sprintf(path, "%s/cpu0/cpufreq/scaling_setspeed", CPU_FREQUENCY_PATH);
	sprintf(data, "%ld", khz);
	if (0 > sys_write(path, data, strlen(data)))
		return -errno;

	return 0;
}

static long cpufreq_get_speed(void)
{
	char path[128], data[32];
	int ret;

	sprintf(path, "%s/cpu0/cpufreq/scaling_cur_freq", CPU_FREQUENCY_PATH);
	ret = sys_read(path, data, sizeof(data));
	if (0 > ret)
		return ret;
	return strtol(data, NULL, 10);	 /* khz */
}

#endif /* UTIL_H */