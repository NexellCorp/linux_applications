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
 *	���>
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
 * 	O_SYNC�� ����� ���� ��Ȯ�� 4096����Ʈ(8 * 512)�� ��������.
 * 	�׷��� �� ������ �б�� �߻����� �ʾҴ�.
 *	�̴� ���� ������ ����Ǹ鼭 �� �����Ͱ� OS ���ۿ� ĳ���Ǿ���,
 *	���� ���� ���� �б� ������ ��ũ�� ���� �������� �ʰ� ĳ������
 * 	�����͸� ������ ����߱� �����̴�.
 *
 *	�ݸ鿡 O_DIRECT�� ��Ȯ�� 4096����Ʈ�� ���� �о���.
 *	�̴� OS ���۰� ���� ������ �ʰ� ������ �����ش�.

 *	mmap�� ������ �޸𸮿� �����ؼ� ������ ������ �а� �� �� �ִ� ����ε�
 *	���� ���Ͻý��� �� �ִ� ����, Ư�� ��Ÿ�ӿ� ���� ũ�Ⱑ Ŀ���� ��쿡��
 * 	�� ���� ���� ������ ����� �� ����.
 *  ������ ������̽� ����(��: /dev/sdb1 ��)�� ���� ��� mmap���� �����Ѵٸ�
 *  ����̽��� ũ�Ⱑ ��Ÿ�ӿ� ����Ǵ� ���� �������� ����� �����ϴ�.
 *  �׷��� mmap�� �ι�° ������ ������ ������ ���̿� ������̽��� ũ�⸦ �Ѱܾ�
 *  �ϴµ� �̰� size_t Ÿ���̶� 32��Ʈ������ �ִ� 4GB, 64��Ʈ������ �ִ�
 *  2^64��Ʈ(����?? ^^)���� ����� �����ϴ�. ��, 32��Ʈ������ 4GB �Ѵ� ������̽���
 *  �ѹ��� ������ ���Ѵ�.
 *
 *	mmap�� ���� ��ü�� �Ѳ����� ���ø����̼� �޸𸮷� �����ϴ°� �ƴ϶�
 *  ���Ͽ� ������ ������ �ش� ���� �޸𸮿� ���εǾ� ���� ������ ������ �Ϻκ��� �о
 *  �����ϴ� ����� ����Ѵ�. �׷��� �� ���� ����� ���� 4096����Ʈ ���� ������ �ߴµ�
 *  ���� 256���� ����(128KB)�� �о���̴� ������ ����Ǿ���. �׸��� ���� �Ŀ� ��û�ߴ�
 *  4096����Ʈ�� ���� ������ ����Ǿ���.
 *
 *	�̴� mmap�� I/O�� ��û�� �κ��� �����ϰ� ���� �ʾƼ� �� �κ��� ��â �о
 *  �޸𸮿� ������ �ϰ�, �޸𸮿��� ���� �۾��� ������ ���̴�. �׸��� ������
 *  ������ ����� �޸𸮰� ������ ��ũ�� �ݿ��Ǵ°� Ȯ���� �� �ִ�.
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
    * O_DIRECT�� ���� ������ ����Ϸ��� �޸� ����, ������ ������, ������ ũ�Ⱑ
    * ��� ��ũ ���� ũ���� ����� ���ĵǾ� �־�� �Ѵ�. ���� �޸𸮴�
    * posix_memalign�� ����ؼ� ���� ũ��� ���ĵǵ��� �ؾ� �Ѵ�.
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
    * O_DIRECT�� ���� ������ mmap���� �����ϴ� ��쿡�� read/writeó�� ����ũ����
    * ����� ���缭 �������� �ʾƵ� �ȴ�.
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