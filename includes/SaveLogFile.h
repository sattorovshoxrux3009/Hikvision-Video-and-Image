#ifndef SAVELOGFILE_H
#define SAVELOGFILE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>

// Platformani aniqlash
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <cstdio>
#endif

void SaveLogFile(int day) {
    const int DAYS_IN_SECONDS = day * 24 * 60 * 60;
    std::string logFilePath = "./Logs.txt";  // Yangi loglar
    std::string line;

#ifdef _WIN32
    // Windows uchun kod
    std::wstring directory = L"./SDKLog";     // SDK Logs
    std::vector<std::wstring> files;

    // Fayllarni SDKLog papkasidan topish
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((directory + L"\\*").c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        const std::wstring fileOrDirName = findFileData.cFileName;
        if (fileOrDirName != L"." && fileOrDirName != L"..") {
            files.push_back(fileOrDirName); // Fayl nomini saqlash
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    FindClose(hFind);

    // Log fayllarini birlashtirish va o'chirish
    for (const auto& file : files) {
        std::string sdkLogFilePath = "./SDKLog/" + std::string(file.begin(), file.end());
        std::ifstream sdkLogFile(sdkLogFilePath);
        std::ofstream logsFile(logFilePath, std::ios_base::app);
        if (!(logsFile.is_open() && sdkLogFile.is_open())) {
            return;
        }
        while (std::getline(sdkLogFile, line)) {
            logsFile << line << std::endl;
        }
        sdkLogFile.close();
        logsFile.close();
        std::remove(sdkLogFilePath.c_str());
    }

#else
    // Linux uchun kod
    std::string directory = "./SDKLog";      // SDK Logs
    std::vector<std::string> files;

    // Fayllarni SDKLog papkasidan topish
    DIR* dir = opendir(directory.c_str());
    if (!dir) {
        std::cerr << "Papka ochilmadi: " << directory << std::endl;
        return;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string fileName = entry->d_name;
        if (fileName != "." && fileName != "..") {
            files.push_back(fileName); // Fayl nomini saqlash
        }
    }
    closedir(dir);

    // Log fayllarini birlashtirish va o'chirish
    for (const auto& file : files) {
        std::string sdkLogFilePath = directory + "/" + file;
        std::ifstream sdkLogFile(sdkLogFilePath);
        std::ofstream logsFile(logFilePath, std::ios_base::app);
        if (!(logsFile.is_open() && sdkLogFile.is_open())) {
            return;
        }
        while (std::getline(sdkLogFile, line)) {
            logsFile << line << std::endl;
        }
        sdkLogFile.close();
        logsFile.close();
        std::remove(sdkLogFilePath.c_str());
    }

#endif

    // Eski yozuvlarni o'chirish
    std::ifstream logFile(logFilePath);
    std::ofstream tempFile("temp_logs.txt"); // Yangi vaqtinchalik fayl
    if (!(tempFile.is_open() && logFile.is_open())) {
        return;
    }
    time_t now = time(nullptr);
    while (std::getline(logFile, line)) {
        std::string logDate = line.substr(1, 19);
        struct tm logTime = {};
        std::istringstream ss(logDate);
        ss >> std::get_time(&logTime, "%Y-%m-%d %H:%M:%S");
        time_t logTimeInSecs = mktime(&logTime);
        if (difftime(now, logTimeInSecs) < DAYS_IN_SECONDS) {
            tempFile << line << std::endl;
        }
    }
    logFile.close();
    tempFile.close();
    if (std::remove(logFilePath.c_str()) || std::rename("temp_logs.txt", logFilePath.c_str())) {
        std::cerr << "Log faylini yangilashda xato yuz berdi." << std::endl;
        return;
    }
}

#endif // SAVELOGFILE_H
