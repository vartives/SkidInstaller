#include <windows.h>
#include <urlmon.h>
#include <iostream>
#include <string>
#include <fstream>
#include <wininet.h>
#include <shlwapi.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shlwapi.lib")

std::string getEnvVar(const char* varName) {
    char* value = nullptr;
    size_t len = 0;
    _dupenv_s(&value, &len, varName);
    std::string result = value ? value : "";
    free(value);
    return result;
}

void setConsoleColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

bool isFileExist(const std::string& filePath) {
    DWORD fileAttr = GetFileAttributesA(filePath.c_str());
    return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

std::string readFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();
    return content;
}

std::string downloadString(const std::string& url) {
    HINTERNET hInternet = InternetOpenA("HTTPGET", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "";

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return "";
    }

    std::string result;
    char buffer[1024];
    DWORD bytesRead = 0;

    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        result.append(buffer, bytesRead);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    return result;
}

void deleteDirectoryRecursive(const std::string& path) {
    WIN32_FIND_DATAA findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    std::string searchPath = path + "\\*";

    hFind = FindFirstFileA(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
            continue;
        }

        std::string filePath = path + "\\" + findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            deleteDirectoryRecursive(filePath);
            RemoveDirectoryA(filePath.c_str());
        }
        else {
            DeleteFileA(filePath.c_str());
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
    RemoveDirectoryA(path.c_str());
}

int main() {
    setConsoleColor(13);

    std::string tempPath = getEnvVar("TEMP");
    std::string extractPath = tempPath + "\\installer_output";
    std::string loaderPath = extractPath + "\\Loader";
    std::string exePath = loaderPath + "\\SkidClientLoader.exe";
    std::string versionFilePath = extractPath + "\\version";

    if (isFileExist(exePath) && isFileExist(versionFilePath)) {
        std::string installedVersion = readFileContent(versionFilePath);

        std::string pastebinUrl = "https://cheatz.lol/downloads/version";
        std::string latestVersion = downloadString(pastebinUrl);

        installedVersion.erase(installedVersion.find_last_not_of(" \n\r\t") + 1);
        latestVersion.erase(latestVersion.find_last_not_of(" \n\r\t") + 1);

        if (!installedVersion.empty() && !latestVersion.empty() && installedVersion == latestVersion) {
            std::cout << "Already on latest version (" << installedVersion << "). Launching...\n";
            ShellExecuteA(NULL, "open", exePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
            return 0;
        }
        else {
            std::cout << "Update available (Current: " << installedVersion << " | Latest: " << latestVersion << "). Updating...\n";
            deleteDirectoryRecursive(extractPath);
        }
    }
    else if (isFileExist(exePath)) {
        std::cout << "Version information missing. Forcing update...\n";
        deleteDirectoryRecursive(extractPath);
    }

    std::string zipUrl = "https://cheatz.lol/downloads/Loader.zip";
    std::string zipPath = tempPath + "\\installer_payload.zip";

    std::cout << "Downloading update...\n";
    HRESULT hr = URLDownloadToFileA(NULL, zipUrl.c_str(), zipPath.c_str(), 0, NULL);

    if (FAILED(hr)) {
        std::cerr << "Error downloading update. HRESULT: " << std::hex << hr << "\n";
        return 1;
    }

    CreateDirectoryA(extractPath.c_str(), NULL);

    std::cout << "Extracting files...\n";
    std::string command = "powershell -Command \"Expand-Archive -Path '" + zipPath + "' -DestinationPath '" + extractPath + "' -Force\"";
    system(command.c_str());

    DeleteFileA(zipPath.c_str());

    if (!isFileExist(exePath)) {
        std::cerr << "Error: Failed to extract or locate the executable.\n";
        return 1;
    }

    std::cout << "Update successful!\n";
    ShellExecuteA(NULL, "open", exePath.c_str(), NULL, NULL, SW_SHOWNORMAL);

    return 0;
}
