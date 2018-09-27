#include "StdAfx.h"
#include "NTPTime.h"
#include <time.h>
#include "winsock2.h"  
#pragma comment(lib, "WS2_32.lib")  

char*  CNTPTime::m_sNTPServers[6]={"time.apple.com","time.windows.com","time1.aliyun.com","pool.ntp.org","time1.google.com","cn.pool.ntp.org"};
unsigned long CNTPTime::m_ulTime[6]={0,0,0,0,0,0};

CNTPTime::CNTPTime(void)
{
	 
}


CNTPTime::~CNTPTime(void)
{
}



DWORD WINAPI  CNTPTime::Thread_GetTime(IN LPVOID Param)
{
	int nIndex=(int)Param;
	if(nIndex<0 || nIndex>=6)
		return 0;

	char sAddress[16]={0};

	int portno=123; 
	
	unsigned char msg[48]={010,0,0,0,0,0,0,0,0};
	unsigned long buf[1024];
	struct sockaddr_in server_addr;

	DWORD dwError=0;

	SOCKET s=INVALID_SOCKET;
 
	 
	int nRet=0;
	do
	{  
		if(!Domain2IpAddress(m_sNTPServers[nIndex],sAddress))
			break;

		if(IsLocalIP(sAddress))
			break;
 
		int iTimeout = 2000;
        
		memset( &server_addr, 0, sizeof( server_addr ));
		server_addr.sin_family=AF_INET;
		server_addr.sin_addr.s_addr = inet_addr(sAddress);
		server_addr.sin_port=htons(portno);

		for(int i=0;i<5;i++)
		{
			s=socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if(s==INVALID_SOCKET)
			{
				dwError=WSAGetLastError();
				break;
			}

			nRet=setsockopt( s,SOL_SOCKET,SO_RCVTIMEO,(const char *)&iTimeout,sizeof(iTimeout));
			if(nRet==SOCKET_ERROR)
				break;

			// send the data to the timing server
			nRet=sendto(s,(const char*)msg,sizeof(msg),0,(struct sockaddr *)&server_addr,sizeof(server_addr));
			if(nRet<=0)
				break;

 			// get the data back
			struct sockaddr saddr;
			int saddr_l = sizeof (saddr);
			// here we wait for the reply and fill it into our buffer
			nRet=recvfrom(s,(char*)buf,48,0,&saddr,&saddr_l);
			if(nRet>0)
			{
				m_ulTime[nIndex]=ntohl((time_t)buf[4]); //# get transmit time
				break;
			}

			if(s!=INVALID_SOCKET)
			{
				closesocket(s);
				s=INVALID_SOCKET;
			}
		}
		

	}while(FALSE);


	if(s!=INVALID_SOCKET)
	{
		closesocket(s);
	}
 
    
	return 0;

}
/************************************************************************/
// function£ºget time from NTP Server                                    
// params£ºNone
// return£ºseconds since 1900-1-1 0:0:0 .  0 is falied
/************************************************************************/
unsigned long CNTPTime::GetTimeFromServer()
{  
 	for(int i=0;i<6;i++)
		m_ulTime[i]=0;

	HANDLE hThreads[6]={0};
 
	for(int i=0;i<6;i++)
	{
		 hThreads[i] = CreateThread(NULL, 0, Thread_GetTime, (LPVOID)i, 0, NULL);
		 Sleep(10);
 	}
	unsigned long ulRet=0;
	DWORD dwWait=0;
	int nIndex=0;
	DWORD dwCount=6;
 	do
	{
		dwWait=WaitForMultipleObjects(dwCount,hThreads,FALSE,INFINITE);
		if(dwWait>=WAIT_OBJECT_0 && dwWait<WAIT_OBJECT_0+dwCount)
		{ 
			nIndex=dwWait-WAIT_OBJECT_0;

			CloseHandle(hThreads[nIndex]);
			hThreads[nIndex]=NULL;

 			for(int i=0;i<6;i++)
			{
				if(m_ulTime[nIndex]>0)
				{
					ulRet=m_ulTime[nIndex];
					break;
				}
			}

			if(ulRet>0)
				break;

			for(int i=0;i<dwCount;i++)
			{
				if(i>nIndex)
				{
					hThreads[i-1]=hThreads[i];
					hThreads[i]=NULL;
				}
			}
 
			
			
			dwCount--;
 		}
		else
			break;
 
		if(dwCount<=0)
			break;

	}while(TRUE);

	for(int i=0;i<6;i++)
	{
		if(hThreads[nIndex])
		{
			::TerminateThread(hThreads[nIndex],0);
			CloseHandle(hThreads[nIndex]);
			hThreads[nIndex]=NULL;
		}
	}

	return ulRet;
}
/************************************************************************/  
/* domain to IP                               */  
/************************************************************************/ 
BOOL  CNTPTime::Domain2IpAddress(char *sDomain,char* sIpAddress)
{
	struct hostent *remoteHost= gethostbyname(sDomain);
	if(remoteHost)
	{
		if (remoteHost->h_addrtype == AF_INET)
		{
			struct in_addr addr;
			int i=0;
			while(remoteHost->h_addr_list[i] != 0) 
			{
				addr.s_addr = *(u_long *) remoteHost->h_addr_list[i++];
				strcpy(sIpAddress,inet_ntoa(addr));

				return TRUE;
			}
		}
		
	}

	return FALSE;
}
/************************************************************************/  
/*  seconds to SYSTEMTIME                                            */  
/************************************************************************/  
SYSTEMTIME CNTPTime::Time2System(DWORD serverTime)  
{  
     FILETIME      ftNew ;       
     SYSTEMTIME    stNew ;       
  
     stNew.wYear         = 1900 ;  
     stNew.wMonth        = 1 ;  
     stNew.wDay          = 1 ;  
     stNew.wHour         = 0 ;  
     stNew.wMinute       = 0 ;  
     stNew.wSecond       = 0 ;  
     stNew.wMilliseconds = 0 ;  
     ::SystemTimeToFileTime (&stNew, &ftNew);  
   
  
     LARGE_INTEGER li ;          
     li = * (LARGE_INTEGER *) &ftNew;  
     li.QuadPart += (LONGLONG) 10000000 * serverTime;   
     ftNew = * (FILETIME *) &li;  
     ::FileTimeToSystemTime (&ftNew, &stNew);  
   
     return stNew;  
}  
/************************************************************************/
// function£ºget time                     
// params£ºbLocal(return local time)
// return£ºFALSE is falied
/************************************************************************/
BOOL CNTPTime::GetTime(SYSTEMTIME &st,BOOL bLocal)
{
	BOOL bRet=FALSE;
	u_long ulTime = GetTimeFromServer();
	if (ulTime != 0)
	{ 
		st=Time2System(ulTime);
		if(bLocal)
		{
			FILETIME ft1,ft2;
			SystemTimeToFileTime(&st,&ft1);
			FileTimeToLocalFileTime(&ft1,&ft2);
			FileTimeToSystemTime(&ft2,&st);
		}
		bRet=TRUE;
 	}

	return bRet;
}
BOOL CNTPTime::IsLocalIP(char *sIP)
{
	BOOL bRet=TRUE;
	DWORD dwIP=htonl(inet_addr(sIP));

	DWORD dwLocalIP00=htonl(inet_addr("10.0.0.0"));
 	DWORD dwLocalIP01=htonl(inet_addr("10.255.255.255"));
 	DWORD dwLocalIP10=htonl(inet_addr("172.16.0.0"));
	DWORD dwLocalIP11=htonl(inet_addr("172.31.255.255"));
 	DWORD dwLocalIP20=htonl(inet_addr("192.168.0.0"));
	DWORD dwLocalIP21=htonl(inet_addr("192.168.255.255"));
 	DWORD dwLocalIP30=htonl(inet_addr("224.0.0.0"));
	DWORD dwLocalIP31=htonl(inet_addr("255.255.255.255"));
 	DWORD dwLocalIP40=htonl(inet_addr("127.0.0.0"));
	DWORD dwLocalIP41=htonl(inet_addr("127.255.255.255"));

	do 
	{
		if(dwIP==0)
			break;

		if(dwIP>=dwLocalIP00 && dwIP<=dwLocalIP01)
			break;

		if(dwIP>=dwLocalIP10 && dwIP<=dwLocalIP11)
			break;

		if(dwIP>=dwLocalIP20 && dwIP<=dwLocalIP21)
			break;

		if(dwIP>=dwLocalIP30 && dwIP<=dwLocalIP31)
			break;

		if(dwIP>=dwLocalIP40 && dwIP<=dwLocalIP41)
			break;

		bRet=FALSE;

	} while(FALSE);

	 
	if(!bRet)
	{ 
		char szHostName[128];
	 	if(gethostname(szHostName, 128) == 0 )
		{  
			struct hostent * pHost; int i;
			pHost = gethostbyname(szHostName);
			for( i = 0; pHost!= NULL && pHost->h_addr_list[i]!= NULL; i++ )  
			{ 
				LPCSTR psz=inet_ntoa (*(struct in_addr *)pHost->h_addr_list[i]);

				if(strcmp(psz,sIP)==0)
				{
					bRet=TRUE;
					break;
				}
			}
		}

	}
	

	return bRet;

}