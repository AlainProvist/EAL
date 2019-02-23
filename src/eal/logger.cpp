/* 
 * This file is part of the Eidolon Auto Link distribution (https://github.com/AlainProvist/EAL).
 * Copyright (c) 2019 AlainProvist.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#include "logger.h"

const char* const s_strings[LT_MAX+1] = 
{ 
	"System",
	"Eidolons",
	""
};

static const char* const GetTagString(LogTag tag)
{
	return s_strings[tag];
}


std::string StringFormat(const char * fmt,...)
{
	va_list va;
	va_start(va,fmt);
	char buffer[1024];
	int ret = vsnprintf(buffer,1024,fmt,va);
	//assert(ret >= 0);
	va_end(va);
	return buffer;
}

static unsigned int utf8froma(char* dst, unsigned dstlen, const char* src, unsigned srclen) 
{
	const char* p = src;
	const char* e = src+srclen;
	unsigned count = 0;
	if (dstlen) 
	{
		for (;;) 
		{
			unsigned char ucs;
			if (p >= e) 
			{
				dst[count] = 0; 
				return count;
			}
			ucs = *(unsigned char*)p++;
			if (ucs < 0x80U || ucs == 0x9CU) 
			{
				dst[count++] = ucs;
				if (count >= dstlen) 
				{
					dst[count-1] = 0; 
					break;
				}
			} 
			else 
			{ /* 2 bytes (note that CP1252 translate could make 3 bytes!) */
				if (count+2 >= dstlen) 
				{
					dst[count] = 0; 
					count += 2; 
					break;
				}
				dst[count++] = 0xc0 | (ucs >> 6);
				dst[count++] = 0x80 | (ucs & 0x3F);
			}
		}
	}
	/* we filled dst, measure the rest: */
	while (p < e) 
	{
		unsigned char ucs = *(unsigned char*)p++;
		if (ucs < 0x80U) 
		{
			count++;
		} 
		else 
		{
			count += 2;
		}
	}
	return count;
}

#define MAX_ENCODING_SIZE 4096
std::string GetUTF8GameString(const std::string& gameStr)
{
	char encodedText[MAX_ENCODING_SIZE];
	int size = strlen(gameStr.c_str());
	if(size > MAX_ENCODING_SIZE)
		size = MAX_ENCODING_SIZE;
	int encodedSize = utf8froma(encodedText, 4096, gameStr.c_str(), size);
	if(encodedSize > MAX_ENCODING_SIZE)
	{
		LogMessage(System, StringFormat("Encoded string \"%s\" was too long and got truncated !", encodedText));
		encodedSize = MAX_ENCODING_SIZE;
	}
	return std::string(encodedText, encodedSize);
}

bool LogInFile(const std::string& message, const std::string& filename/* = "logs.txt"*/)
{
	static bool first = true;

	std::string path = GetDllPath();

	std::string longname = path + filename;

	FILE* file = fopen(longname.c_str(), first? "w":"a+");
	if (!file)
		return false;
	first = false;

	fprintf( file, "%s\n", message.c_str() );

	fclose(file);

	return true;
}

#ifdef USE_LOGS

DeclareScopeLock(Logger);
Logger Logger::ms_Instance = Logger();

LogEntry::LogEntry(LogTag tag, const std::string& mes) : Tag(tag), Message(mes)
{
	GetSystemTimeAsFileTime(&Timestamp); //Gets the current system time
}

std::string LogEntry::GetFormatedLog() const
{
	std::string str;

	SYSTEMTIME stime;
	FILETIME ltime;
	
	FileTimeToLocalFileTime (&Timestamp, &ltime);//convert in local time and store in ltime
	FileTimeToSystemTime(&ltime, &stime);//convert in system time and store in stime

	return StringFormat("%02d/%02d %02d:%02d:%02d.%03d - [%s]%s \n", 
		stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds,
		GetTagString(Tag), Message.c_str());
}

void Logger::AddLog(LogTag tag, const std::string& log)
{
	UseScopeLock(Logger);
	if(m_Logs.size() >= LOG_HISTORY)
		m_Logs.erase(m_Logs.begin());
	LogEntry entry(tag, log);
	m_Logs.push_back(entry);
	m_HasChanged = true;

	if(m_LogInFile)
		::LogInFile(entry.GetFormatedLog());
}
std::string Logger::GetLogsText(bool& hasChanged)
{
	UseScopeLock(Logger);
	hasChanged = m_HasChanged;
	m_HasChanged = false;
	std::string str;
	for(u32 i = m_Logs.size(); i > 0; --i)
	{
		const LogEntry& entry = m_Logs[i-1];
		if((u32)entry.Tag >= LT_MAX || m_Options.Display[entry.Tag])
			str += entry.GetFormatedLog();
	}
	return str;
}
void Logger::ClearLogs()
{
	UseScopeLock(Logger);
	m_Logs.clear();
	m_HasChanged = true;
}
void Logger::ForceRefresh()
{
	UseScopeLock(Logger);
	m_HasChanged = true;
}

void Logger::GetDisplayOptions(DisplayOptions& options)
{
	UseScopeLock(Logger);
	options = m_Options;
}
void Logger::SetDisplayOptions(const DisplayOptions& options)
{
	UseScopeLock(Logger);
	m_Options = options;
}

DeclareScopeLock(AutoRefreshLogs);
void Logger::AutoRefreshLogs(bool state)
{
	UseScopeLock(AutoRefreshLogs);
	m_AutoRefresh = state;
}
bool Logger::AutoRefreshLogs()
{
	UseScopeLock(AutoRefreshLogs);
	return m_AutoRefresh;
}


#endif //#ifdef USE_LOGS
