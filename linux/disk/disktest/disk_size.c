#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h> 			/* error */
#include <float.h>
#include <sys/vfs.h>

void print_usage(void)
{
    printf( "usage: options\n"
    		"-d	directory path, default current path \n"
    		"-k	print kbyte \n"
    		"-m	print mbyte \n"
    	);
}

#define	KBYTE		(1024)
#define	MBYTE		(1024 * KBYTE)
#define	GBYTE		(1024 * MBYTE)

#define	DISK_PATH	"./"

int main(int argc, char **argv)
{
    long long total;
    long long used;
    long long free;
    long long avail;
    double used_percent;
    struct statfs fs;

	int opt;
	char * path = DISK_PATH;
	int  kb  = 1, mb = 1, gb = 1;
	int  div = 1;
    while (-1 != (opt = getopt(argc, argv, "hd:kmg"))) {
		switch(opt) {
        case 'h':   print_usage();  exit(0);   	break;
        case 'd':   path = optarg;				break;
        case 'k':   kb = KBYTE;					break;
        case 'm':   mb = MBYTE;					break;
        case 'g':   gb = GBYTE;					break;
        default:
        	break;
         }
	}

    if (0 > statfs(path, &fs)) {
    	printf("Error: path=%s \n", path);
		return -1;
	}

	div   = (kb * mb * gb);
    total = (fs.f_bsize * fs.f_blocks)/div;
    free  = (fs.f_bsize * fs.f_bfree)/div;
    avail = (fs.f_bsize * fs.f_bavail)/div;
    used  = (total - free);
    used_percent = ((double)used/(double)total)*100;

	printf("PATH         : %s\n", path);
    printf("Total size   : %16lld %c\n", total, div==1?'B':(div==KBYTE?'K':(div==MBYTE?'M':((div==GBYTE?'G':'B')))));
    printf("Free  size   : %16lld %c\n", free , div==1?'B':(div==KBYTE?'K':(div==MBYTE?'M':((div==GBYTE?'G':'B')))));
   	printf("Avail size   : %16lld %c\n", avail, div==1?'B':(div==KBYTE?'K':(div==MBYTE?'M':((div==GBYTE?'G':'B')))));
    printf("Used  size   : %16lld %c\n", used , div==1?'B':(div==KBYTE?'K':(div==MBYTE?'M':((div==GBYTE?'G':'B')))));
    printf("Used  percent: %16f %%  \n", used_percent);

    return 0;
}
