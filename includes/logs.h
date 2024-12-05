#ifndef LOGS_H
#define LOGS_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <cstring>
#include <algorithm>

// Platformaga mos bo'lgan include fayllar
#ifdef _WIN32
	#include <windows.h>
#else
	#include <dirent.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif

#define LOGFILE_NAME L"Logs.txt"                  // Log fayl nomi (Windows uchun wide-character format)
#define ARCHIVE_DIR  L"ArchivedLogs\\"             // Arxiv papkasi (Windows uchun wide-character format)
#define MAX_LOG_SIZE_B (2 * 1024 * 1024)         // 2MB hajm limiti
#define MAX_ARCHIVE_LOG_NUMBER 8                 // 8 ta faylni arxivga saqlaydi

// Funksiya deklaratsiyalari
void check_and_cleanup_old_files();
void check_and_archive_log();
void log_to_file(const std::string& message);

#endif // LOGS_H
