// I2C test program for linux

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <linux/i2c-dev.h>

// I2C Linux device handle
int g_i2cFile;

//--------------------------------------------------------------------------------
unsigned int atoh(char *a)
{
	char ch = *a;
	unsigned int cnt = 0, hex = 0;

	while(((ch != '\r') && (ch != '\n')) || (cnt == 0)) {
		ch  = *a;

		if (! ch)
			break;

		if (cnt != 8) {
			if (ch >= '0' && ch <= '9') {
				hex = (hex<<4) + (ch - '0');
				cnt++;
			} else if (ch >= 'a' && ch <= 'f') {
				hex = (hex<<4) + (ch-'a' + 0x0a);
				cnt++;
			} else if (ch >= 'A' && ch <= 'F') {
				hex = (hex<<4) + (ch-'A' + 0x0A);
				cnt++;
			}
		}
		a++;
	}
	return hex;
}

//--------------------------------------------------------------------------------
void print_usage(void)
{
    printf( "usage: options\n"
            "-w	write, default read 		\n"
            "-p	port i2c-n, default i2c-0 	\n"
            "-a	HEX address					\n"
            "-r	register   					\n"
            "-c	loop count			 		\n"
    		"-m mode select, 0 = W: addr/data						\n"
    		"                    R: addr/data						\n"
    		"                1 = W: addr/reg/data					\n"
    		"                    R: addr/reg     [s] addr/data		\n"
    		"                2 = W: addr/reg/data/data				\n"
    		"                    R: addr/reg     [s] addr/data/data	\n"
    		"                3 = W: addr/reg/reg/data/data			\n"
    		"                    R: addr/reg/reg [s] addr/data/data	\n"
            );
}

// open the Linux device
void i2cOpen(int port)
{
  	char path[20];
	
	//----------------------------------------------------------------------------
  	snprintf(path, 19, "/dev/i2c-%d", port);
	g_i2cFile = open(path, O_RDWR);

	if (g_i2cFile < 0) {
		perror("i2cOpen fail");
		exit(1);
	}
}

// close the Linux device
void i2cClose()
{
	close(g_i2cFile);
}

// set the I2C slave address for all subsequent I2C device transfers
void i2cSetAddress(unsigned char address)
{
	if (ioctl(g_i2cFile, I2C_SLAVE, address) < 0) {
		perror("i2cSetAddress");
		exit(1);
	}
}

// write a 16 bit value to a register pair
// write low byte of value to register reg,
// and high byte of value to register reg+1
void i2cWrite(uint8_t reg, uint16_t value, int len)
{
	uint8_t data[4];

	switch (len) {
		case 1:
			data[0] = value & 0xff;
			break;
		case 2:
			data[0] = reg & 0xff;
			data[1] = value & 0xff;
			break;
		case 3:
			data[0] = reg & 0xff;
			data[1] = value & 0xff;
			data[2] = (value >> 8) & 0xff;
			break;
		case 4:
			data[0] = reg & 0xff;
			data[1] = (reg >> 8) & 0xff;
			data[2] = value & 0xff;
			data[3] = (value >> 8) & 0xff;
			break;
		default:
			break;
	}
		
	if (write(g_i2cFile, data, len) != len) {
		perror("i2cWrite");
	}
}

// read a 16 bit value from a register pair
uint16_t i2cRead(uint8_t reg, int w_len, int r_len)
{
	uint8_t data[4];
	uint16_t readval = 0;
	int len = w_len + r_len;

	switch (len) {
		case 1:
			data[0] = 0;
			break;
		case 2:
			data[0] = reg & 0xff;
			break;
		case 3:
			data[0] = reg & 0xff;
			break;
		case 4:
			data[0] = reg & 0xff;
			data[1] = (reg >> 8) & 0xff;
			break;
		default:
			break;
	}

	if (w_len > 0) {
		if (write(g_i2cFile, data, w_len) != w_len) {
			perror("i2cRead set register");
		}
	}
	if (read(g_i2cFile, data, r_len) != r_len) {
		perror("i2cRead read value");
	}
	
	if (2 == r_len) {
		readval = (data[0]<<8 & 0xFF00) | (data[1] & 0xFF);
		printf("DATA=0x%04x\n", readval & 0xFFFF);
	} else {
		readval = data[0];
		printf("DATA=0x%02x\n", readval & 0xFF);
	}

	return readval;
}

int main(int argc, char **argv)
{
  	int   opt;
  	int   port   =  0; 				/* 0, 1, 2 */
	unsigned char  addr   =  0; 				/* The I2C slave address */
	int   w_mode =  0;
	int   mode   = -1;
	int	  count  =  3;
	int   verify =  0;
  	short reg = -1, dat = -1;
  	int   w_len = 0, r_len = 0, i = 0;
  	char  buf[256];
	int   readval = 0;
	
    while (-1 != (opt = getopt(argc, argv, "hw:p:a:r:m:c:v:"))) {
		switch(opt) {
        case 'h':   print_usage();  exit(0);	break;
        case 'w':   w_mode = 1;
        			dat    = atoh(optarg); 		break;
        case 'p':   port   = atoi(optarg);		break;
        case 'a':   addr   = atoh(optarg);		break;
        case 'r':   reg    = atoh(optarg);		break;
		case 'm':   mode   = atoi(optarg);		break;
		case 'c':   count  = atoi(optarg);		break;
		case 'v':   verify = atoh(optarg); 		break;
        default:
        	break;
         }
	}

	//----------------------------------------------------------------------------
	/* check input params */
	if (-1 == mode){
    	printf("no i2c-%d invalid mode %d \n", port, mode);
    	print_usage();
		goto __end;
	}

	if (!mode && -1 == reg) {
    	printf("no i2c-%d register	\n", port);
    	print_usage();
		goto __end;
	}

	if (0 >= count)
		count = 1;

	// open Linux I2C device
	i2cOpen(port);

	// set slave address of I2C device
	i2cSetAddress(addr >> 1);


	//----------------------------------------------------------------------------
	if (w_mode) {

		w_len = mode + 1;

		for (i = 0; count > i; i++) {

			if (0 == mode) {
				printf("[%s], I2C-%d, ADDR=0x%02x, DATA=0x%02x\n",
					w_mode ? "WRITE" : "READ", 
					port, addr, dat&0xFF);
			}

			if (1 == mode) {
				printf("[%s], I2C-%d, ADDR=0x%02x, REG=0x%02x, DATA=0x%02x\n",
					w_mode ? "WRITE" : "READ", 
					port, addr, reg&0xFF, dat&0xFF);
			}

			if (2 == mode) {
				printf("[%s], I2C-%d, ADDR=0x%02x, REG=0x%02x, DATA=0x%02x%02x\n",
					w_mode ? "WRITE" : "READ",
					port, addr, (reg>>0)&0xFF, (dat>>8)&0xFF, (dat>>0)&0xFF);
			}

			if (3 == mode) {
				printf("[%s], I2C-%d, ADDR=0x%02x, REG=0x%04x, DATA=0x%02x%02x\n",
					w_mode ? "WRITE" : "READ", 
					port, addr, reg, (dat>>8)&0xFF, (dat>>0)&0xFF);
			}
			i2cWrite(reg, dat, w_len);
 		}

	} else {

		for (i = 0; count > i; i++) {

			if (0 == mode) {
				printf("[%s], I2C-%d, ADDR=0x%02x, DAT=0x%02x, ",
					w_mode ? "WRITE" : "READ", port, addr, (dat>>0)&0xFF);
				/* 1 setp : read [ADDR, DAT] */
				w_len  = 0;
				r_len  = 1;
			}

			if (1 == mode) {
				printf("[%s], I2C-%d, ADDR=0x%02x, REG=0x%02x, ",
					w_mode ? "WRITE" : "READ", port, addr, (reg>>0)&0xFF);
				/* 1 setp : write [ADDR, REG] */
				w_len  = 1;
				r_len  = 1;
			}

			if (2 == mode) {
				printf("[%s], I2C-%d, ADDR=0x%02x, REG=0x%04x, ",
					w_mode ? "WRITE" : "READ", port, addr, (reg>>0)&0xFF);
				/* 1 setp : write [ADDR, REG] */
				w_len  = 1;
				r_len  = 2;
			}

			if (3 == mode) {
				printf("[%s], I2C-%d, ADDR=0x%02x, REG=0x%04x, ",
					w_mode ? "WRITE" : "READ", port, addr, reg);

				/* 1 setp : write [ADDR, REG(MSB), REG(LSB)] */
				w_len  = 2;
				r_len  = 2;
			}

			readval  = i2cRead(reg, w_len, r_len);
			if (verify == 0)
				return 0;
			if(readval  == verify)
			{
				printf(" return 0 read : %x verify: %x \n", dat, verify);
				return 0;
			}
			else{

				printf(" return err!  read :%x verify : %x \n", dat, verify);
			//	return -1;
			}
			usleep(100000);
		}
 	}
	return -1;
__end:
	if (g_i2cFile)
		i2cClose();

  	return 0;
}

