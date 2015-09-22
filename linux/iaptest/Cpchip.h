struct ipodIoParam_t {
	unsigned long ulRegAddr;
	char* pBuf;
	unsigned int nBufLen;
};

void SysIpodCpchipReset(void);
void SysIpodCpchipRead(ipodIoParam_t* param);
void SysIpodCpchipWrite(ipodIoParam_t* param);

