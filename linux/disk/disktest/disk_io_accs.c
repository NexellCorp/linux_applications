#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define SECTOR_SIZE 512
#define BUF_SIZE (SECTOR_SIZE * 8)

/*
 *	HOST PC command
 * 	#> sudo blktrace /dev/sdb1 -a complete -o - | blkparse -i - -f "%T.%t %d %S %n\n"
 *  #> sudo hdparm -tT /dev/sdb1
 *
 *	결과>
 *		<O_SYNC>
 * 		0.000000000 W 63 8
 *
 *		<O_DIRECT>
 *		3.559173154 W 63 8
 *		3.559389900 R 63 8
 *
 *		<mmap>
 *		8.970557669 R 63 128
 *		8.970921848 R 191 128
 *		32.420494241 W 63 8
 *
 *
 * 	O_SYNC의 결과를 보면 정확히 4096바이트(8 * 512)가 쓰여졌다.
 * 	그러나 그 다음에 읽기는 발생하지 않았다.
 *	이는 쓰기 연산이 수행되면서 그 데이터가 OS 버퍼에 캐쉬되었고,
 *	같은 블럭에 대한 읽기 연산은 디스크에 직접 접근하지 않고 캐쉬에서
 * 	데이터를 가져와 사용했기 때문이다.
 *
 *	반면에 O_DIRECT는 정확히 4096바이트를 쓰고 읽었다.
 *	이는 OS 버퍼가 전혀 사용되지 않고 있음을 보여준다.

 *	mmap은 파일을 메모리에 매핑해서 파일의 내용을 읽고 쓸 수 있는 방법인데
 *	보통 파일시스템 상에 있는 파일, 특히 런타임에 파일 크기가 커지는 경우에는
 * 	몇 가지 이유 때문에 사용할 수 없다.
 *  하지만 블럭디바이스 파일(예: /dev/sdb1 등)을 직접 열어서 mmap으로 매핑한다면
 *  디바이스의 크기가 런타임에 변경되는 경우는 없음으로 사용이 가능하다.
 *  그러나 mmap의 두번째 인자인 매핑할 파일의 길이에 블럭디바이스의 크기를 넘겨야
 *  하는데 이게 size_t 타입이라서 32비트에서는 최대 4GB, 64비트에서는 최대
 *  2^64비트(얼마지?? ^^)까지 사용이 가능하다. 즉, 32비트에서는 4GB 넘는 블럭디바이스는
 *  한번에 매핑을 못한다.
 *
 *	mmap은 파일 전체를 한꺼번에 어플리케이션 메모리로 매핑하는게 아니라
 *  파일에 접근할 때마다 해당 블럭이 메모리에 매핑되어 있지 않으면 파일의 일부분을 읽어서
 *  매핑하는 방식을 사용한다. 그래서 위 실험 결과를 보면 4096바이트 쓰기 연산을 했는데
 *  먼저 256개의 섹터(128KB)를 읽어들이는 연산이 실행되었다. 그리고 한참 후에 요청했던
 *  4096바이트의 쓰기 연산이 실행되었다.
 *
 *	이는 mmap이 I/O가 요청된 부분을 매핑하고 있지 않아서 그 부분을 왕창 읽어서
 *  메모리에 매핑을 하고, 메모리에서 쓰기 작업을 수행한 것이다. 그리고 적절한
 *  시점에 변경된 메모리가 실제로 디스크에 반영되는걸 확인할 수 있다.
 *
 */
int main(int argc, char **argv)
{
   int fd, rv;
   char *addr = NULL;
   void *buf = NULL;

   if (argc != 2) {
       fprintf(stderr, "Usage: %s filename\n", argv[0]);
       exit(EXIT_FAILURE);
   }

   /*
    * O_DIRECT로 열린 파일은 기록하려는 메모리 버퍼, 파일의 오프셋, 버퍼의 크기가
    * 모두 디스크 섹터 크기의 배수로 정렬되어 있어야 한다. 따라서 메모리는
    * posix_memalign을 사용해서 섹터 크기로 정렬되도록 해야 한다.
    */
   rv = posix_memalign(&buf, SECTOR_SIZE, BUF_SIZE);
   if (rv) {
       fprintf(stderr, "Failed to allocate memory: %s\n", strerror(rv));
       exit(EXIT_FAILURE);
   }

   fd = open(argv[1], O_RDWR | O_SYNC | O_CREAT);
   printf("O_SYNC..\n");

   if (fd == -1) {
       fprintf(stderr, "Failed to open %s: %s\n", argv[1], strerror(errno));
       exit(EXIT_FAILURE);
   }
   if (pwrite(fd, buf, BUF_SIZE, 1024) == -1) {
           fprintf(stderr, "Failed to write %s: %s\n", argv[1], strerror(errno));
           exit(EXIT_FAILURE);
   }
   if (pread(fd, buf, BUF_SIZE, 1024) == -1) {
           fprintf(stderr, "Failed to read %s: %s\n", argv[1], strerror(errno));
           exit(EXIT_FAILURE);
   }

   getchar();

   fd = open(argv[1], O_RDWR | O_DIRECT);
   printf("O_DIRECT..\n");

   if (fd == -1) {
       fprintf(stderr, "Failed to open %s: %s\n", argv[1], strerror(errno));
       exit(EXIT_FAILURE);
   }
   if (pwrite(fd, buf, BUF_SIZE, 1024) == -1) {
           fprintf(stderr, "Failed to write %s: %s\n", argv[1], strerror(errno));
           exit(EXIT_FAILURE);
   }
   if (pread(fd, buf, BUF_SIZE, 1024) == -1) {
           fprintf(stderr, "Failed to read %s: %s\n", argv[1], strerror(errno));
           exit(EXIT_FAILURE);
   }

   getchar();

   /*
    * O_DIRECT로 열린 파일을 mmap으로 매핑하는 경우에는 read/write처럼 섹터크기의
    * 배수로 맞춰서 접근하지 않아도 된다.
    */
   addr = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   printf("mmap..\n");

   if (!addr) {
       fprintf(stderr, "Failed to mmap %s: %s\n", argv[1], strerror(errno));
       exit(EXIT_FAILURE);
   }
   memcpy(addr, buf, BUF_SIZE);
   memcpy(buf, addr, BUF_SIZE);

   getchar();

   munmap(addr, BUF_SIZE);
   free(buf);

   return 0;
}