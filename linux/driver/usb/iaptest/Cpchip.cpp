#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "Cpchip.h"
#include "gpio.h"

#define CP_DEVID	0x20	

#define CP_DEVPORT	0		// tnn
#define CP_RESET	PAD_GPIO_E + 9			// tnn

// I2C Linux device handle
int g_i2cFile;

// open the Linux device
void i2cOpen(int port)
{
  	char path[20];
	
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

static bool i2c_write(char regaddr, const void* write_buf, int len)
{
    unsigned char  addr = CP_DEVID;                 /* The I2C slave address */
    int   w_len = 0, r_len = 0, i = 0;
	char reg =  regaddr;
	char*  buf = (char *)malloc(len+1);

	buf[0] = (reg >> 0) & 0xFF;
	memcpy(&buf[1], write_buf, len); 
	w_len  = len + 1;

	if (write(g_i2cFile, buf, w_len) != w_len) {
		perror("i2cWrite");
		return false;
	}
/*
	for (i = 0; i < w_len; i++)
		printf("[%x]", buf[i]);
	printf("\r\n");
*/	
	usleep(1000);

	return true;
}

static bool i2c_read(char regaddr, void* recv_buf, int len)
{
    unsigned char  addr = CP_DEVID;                 /* The I2C slave address */
    int   w_len = 0, r_len = 0, i = 0;
	char reg =  regaddr;
	char*  buf = (char *)malloc(len);
	buf = (char *)recv_buf;

	/* 1 setp : write [ADDR, REG] */
	buf[0] = (reg >> 0) & 0xFF;
	w_len  = 1;
	r_len  = len;

	if (write(g_i2cFile, buf, w_len) != w_len) {
		perror("i2cRead set register");
		return false;
	}
	if (read(g_i2cFile, buf, r_len) != r_len) {
		perror("i2cRead read value");
		return false;
	}

/*
	for (i = 0; i < r_len; i++)
		printf("[%x]", buf[i]);
	printf("\r\n");
*/	
	usleep(1000);

	return true;
}

void SysIpodCpchipReset(void)
{
//	int ret =0;

	printf("====%s\n", __func__);
/*
	if(gpio_export(CP_RESET)!=0)
	{
		printf("gpio open fail\n");
		ret = -ENODEV;
	}

	gpio_dir_out(CP_RESET);

	gpio_set_value(CP_RESET,0);
	usleep(300000);
	gpio_set_value(CP_RESET,1);

	gpio_unexport(CP_RESET);
*/	
}

void SysIpodCpchipRead(ipodIoParam_t* param)
{
	int i = 0;
		
	printf("====%s %x %d\n", __func__, param->ulRegAddr, param->nBufLen);

	i2cOpen(CP_DEVPORT);

	// set slave address of I2C device
    i2cSetAddress(CP_DEVID >> 1);

	for (i = 0; i < 5; i++) {
		if (i2c_read((char)param->ulRegAddr, param->pBuf, param->nBufLen))
			break;
		printf("Err %d ", i);
		usleep(100000);
	}
	if (i == 5) {
		printf("%s fail.\n", __func__);
	}
	i2cClose();
}

void SysIpodCpchipWrite(ipodIoParam_t* param)
{
	int i = 0;
		
	printf("====%s %x %d\n", __func__, param->ulRegAddr, param->nBufLen);

	i2cOpen(CP_DEVPORT);

	// set slave address of I2C device
    i2cSetAddress(CP_DEVID >> 1);

	for (i = 0; i < 5; i++) {
		if (i2c_write((char)param->ulRegAddr, param->pBuf, param->nBufLen))
			break;
		printf("Err %d ", i);
		usleep(100000);
	}
	if (i == 5) {
		printf("%s fail.\n", __func__);
	}
	i2cClose();
}
