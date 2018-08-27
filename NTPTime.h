#pragma once
class CNTPTime
{
public:
	CNTPTime(void);
	~CNTPTime(void);
	static BOOL  GetTime(SYSTEMTIME &st,BOOL bLocal);
private:
	static DWORD WINAPI  Thread_GetTime(IN LPVOID Param);

	static unsigned long GetTimeFromServer();
	static SYSTEMTIME    Time2System(DWORD serverTime);
	static BOOL          Domain2IpAddress(char *sDomain,char* sIpAddress);
	static BOOL          IsLocalIP(char *sIP);//Simply prevent building a local NTP server
private:
	static char*  m_sNTPServers[6];
	static unsigned long m_ulTime[6];
};

