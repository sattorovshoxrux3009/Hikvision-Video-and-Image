#include "logs.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

// Eski fayllarni arxiv papkasidan o'chirish
void check_and_cleanup_old_files() {
#ifdef _WIN32
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW((std::wstring(ARCHIVE_DIR) + L"*").c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    std::vector<WIN32_FIND_DATAW> files;
    do {
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;   // "." va ".." papkalarini o'tkazib yuborish
        }
        files.push_back(findFileData);
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);
    std::sort(files.begin(), files.end(), [](const WIN32_FIND_DATAW& f1, const WIN32_FIND_DATAW& f2) {
        return CompareFileTime(&f1.ftCreationTime, &f2.ftCreationTime) < 0;
        });
    if (files.size() > MAX_ARCHIVE_LOG_NUMBER) {
        wchar_t fullFilePath[MAX_PATH];
        wcscpy_s(fullFilePath, MAX_PATH, ARCHIVE_DIR);
        wcscat_s(fullFilePath, MAX_PATH, files[0].cFileName);
        DeleteFileW(fullFilePath);
    }
#else  // Linux
    DIR* dir = opendir(ARCHIVE_DIR);
    if (!dir) {
        return;
    }

    struct dirent* entry;
    std::vector<std::string> files;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;  // Faqat fayllarni olish
        }
        files.push_back(entry->d_name);
    }
    closedir(dir);

    if (files.size() > MAX_ARCHIVE_LOG_NUMBER) {
        // Fayllarni yaratish vaqtiga qarab saralash
        std::sort(files.begin(), files.end());

        // Eski faylni o'chirish
        std::string fullFilePath = std::string(ARCHIVE_DIR) + files[0];
        if (remove(fullFilePath.c_str()) != 0) {
            std::cerr << "Faylni o'chirishda xatolik: " << strerror(errno) << std::endl;
        }
    }
#endif
}

// Log faylini hajmini tekshirish va arxivga ko'chirish
void check_and_archive_log() {
#ifdef _WIN32
    HANDLE hFile = CreateFile(LOGFILE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return;
    }
    CloseHandle(hFile);
    if (fileSize.QuadPart >= MAX_LOG_SIZE_B) {
        if (GetFileAttributes(ARCHIVE_DIR) == INVALID_FILE_ATTRIBUTES) {
            CreateDirectory(ARCHIVE_DIR, NULL);
        }
        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t archive_file_name[MAX_PATH];
        swprintf_s(archive_file_name, MAX_PATH, L"%sLog_%02d-%02d-%04d_%02d-%02d-%02d.txt",
            ARCHIVE_DIR, st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
        MoveFile(LOGFILE_NAME, archive_file_name);
        check_and_cleanup_old_files();
        std::ofstream new_log_file(LOGFILE_NAME, std::ios::trunc);
    }

#else  // Linux
    struct stat fileStat;
    if (stat(LOGFILE_NAME, &fileStat) != 0) {
        return;
    }

    if (fileStat.st_size >= MAX_LOG_SIZE_B) {
        // Arxiv papkasini yaratish (agar mavjud bo'lmasa)
        struct stat st = { 0 };
        if (stat(ARCHIVE_DIR, &st) == -1) {
            mkdir(ARCHIVE_DIR, 0700);
        }

        time_t now = time(0);
        struct tm* tstruct = localtime(&now);
        char archive_file_name[128];
        strftime(archive_file_name, sizeof(archive_file_name), "%Y-%m-%d_%H-%M-%S.txt", tstruct);

        std::string archivePath = std::string(ARCHIVE_DIR) + archive_file_name;
        rename(LOGFILE_NAME, archivePath.c_str());

        check_and_cleanup_old_files();  // Eski fayllarni tozalash

        std::ofstream new_log_file(LOGFILE_NAME, std::ios::trunc);
    }
#endif
}

// Log ma'lumotini faylga yozish
void log_to_file(const std::string& message) {
    check_and_archive_log();
    std::ofstream log_file(LOGFILE_NAME, std::ios::app);
    if (!log_file) {
        return;
    }
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timestamp[64];
    snprintf(timestamp, sizeof(timestamp), "%02d-%02d-%04d %02d:%02d:%02d.%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    log_file << timestamp << message << std::endl;
    log_file.close();
}
