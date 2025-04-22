#include <windows.h>
#include <urlmon.h>
#include <iostream>
#include <string>

#pragma comment(lib, "urlmon.lib")

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

int main() {
    setConsoleColor(13);

    std::string tempPath = getEnvVar("TEMP");
    std::string extractPath = tempPath + "\\installer_output";
    std::string exePath = extractPath + "\\Loader\\SkidClientLoader.exe";

    if (isFileExist(exePath)) {
        ShellExecuteA(NULL, "open", exePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        exit(0);
    }

    std::string zipUrl = "https://cheatz.lol/downloads/Loader.zip";
    std::string zipPath = tempPath + "\\installer_payload.zip";

    std::cout << "Downloading\n";
    HRESULT hr = URLDownloadToFileA(NULL, zipUrl.c_str(), zipPath.c_str(), 0, NULL);

    if (FAILED(hr)) {
        std::cerr << "Error. HRESULT: " << std::hex << hr << "\n";
        return 1;
    }

    std::cout << "Extracting\n";
    std::string command = "powershell -Command \"Expand-Archive -Path '" + zipPath + "' -DestinationPath '" + extractPath + "' -Force\"";
    system(command.c_str());

    std::cout << "Download Successfully\n";
    ShellExecuteA(NULL, "open", exePath.c_str(), NULL, NULL, SW_SHOWNORMAL);

    exit(0);

    return 0;
}
