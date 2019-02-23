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


#ifndef LOGGER_H
#define LOGGER_H

#include "version.h"
#include <string>


#define USE_LOGS
//#define LOG_WARNINGS



#ifdef USE_LOGS
#define LOG_HISTORY 1000
#define CONCAT2(a,b) a##b
#define LogMessage(Tag, mes) Logger::GetInstance().AddLog(CONCAT2(LT_, Tag), mes)
#else
#define LogMessage(Tag, mes) 
#endif

bool LogInFile(const std::string& message, const std::string& filename = "logs.txt");

#ifdef LOG_WARNINGS
#define WarningMessage(mes) LogWarning(mes)
#else
#define WarningMessage(mes) 
#endif
#define DumpMessage(mes) LogInFile(mes)


#ifdef USE_LOGS

#include "utils.h"
#include "objects.h"

#include <string>
#include <vector>
#include <algorithm>

enum LogTag 
{
	LT_System, 
	LT_Eidolons,

	LT_MAX
};

struct LogEntry
{
	LogEntry() : Tag(LT_MAX) {}
	LogEntry(LogTag tag, const std::string& mes);

	std::string GetFormatedLog() const;

	LogTag Tag;
	std::string Message;
	FILETIME Timestamp;
};

class Logger
{
public:
	struct DisplayOptions
	{
		DisplayOptions() {for(u32 i = 0; i < LT_MAX; ++i) Display[i] = true;}

		bool Display[LT_MAX];
	};

	static Logger& GetInstance() { return ms_Instance; }

	void AddLog(LogTag tag, const std::string& log);
	std::string GetLogsText(bool& hasChanged);
	void ClearLogs();
	void ForceRefresh();

	void GetDisplayOptions(DisplayOptions& options);
	void SetDisplayOptions(const DisplayOptions& options);

	void AutoRefreshLogs(bool state);
	bool AutoRefreshLogs();

	void LogInFile(bool state) {m_LogInFile = state;}
	bool LogInFile() {return m_LogInFile;}

private:
	Logger() : m_HasChanged(false), m_AutoRefresh(true), m_LogInFile(true) {}
	~Logger() {}

	std::vector<LogEntry> m_Logs;

	DisplayOptions m_Options;

	bool m_HasChanged;
	bool m_AutoRefresh;
	bool m_LogInFile;

	static Logger ms_Instance;
};

#endif //#ifdef USE_LOGS

#endif //LOGGER_H
