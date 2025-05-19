// JavaDetector.h
#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <shlobj.h>
#include <iostream>

class JavaDetector {
public:
    static std::vector<std::wstring> FindJavaPaths(bool debug = false) {
        JavaDetector detector(debug);
        detector.CheckEnvironmentVariable();
        detector.CheckRegistry();
        detector.ScanDefaultDirectories();
        detector.ScanCustomDirectories();
        return detector.RemoveDuplicates(detector.javaPaths_);
    }

private:
    std::vector<std::wstring> javaPaths_;
    bool debug_;

    JavaDetector(bool debug) : debug_(debug) {}

    //--------------------------------------------------
    // 工具函数
    //--------------------------------------------------
    bool PathExists(const std::wstring& path) {
        DWORD attrib = GetFileAttributesW(path.c_str());
        bool exists = (attrib != INVALID_FILE_ATTRIBUTES &&
            !(attrib & FILE_ATTRIBUTE_DIRECTORY));
        if (debug_) {
            std::wcout << L"[DEBUG] 路径检查: " << path
                << (exists ? L" 存在" : L" 不存在") << std::endl;
        }
        return exists;
    }

    std::wstring ReadRegistryString(HKEY hKey, const std::wstring& subKey,
        const std::wstring& valueName, DWORD flags = 0) {
        HKEY key;
        wchar_t value[MAX_PATH] = { 0 };
        DWORD valueSize = sizeof(value);
        DWORD type = 0;

        LONG result = RegOpenKeyExW(hKey, subKey.c_str(), 0, KEY_READ | flags, &key);
        if (result != ERROR_SUCCESS) {
            if (debug_) {
                std::wcout << L"[DEBUG] 注册表打开失败: " << subKey
                    << L" 错误码: " << result << std::endl;
            }
            return L"";
        }

        result = RegQueryValueExW(key, valueName.c_str(), nullptr, &type,
            reinterpret_cast<LPBYTE>(value), &valueSize);
        RegCloseKey(key);

        if (result != ERROR_SUCCESS || type != REG_SZ) {
            if (debug_) {
                std::wcout << L"[DEBUG] 注册表读取失败: " << subKey << L"\\" << valueName
                    << L" 错误码: " << result << std::endl;
            }
            return L"";
        }

        if (debug_) {
            std::wcout << L"[DEBUG] 注册表读取成功: " << subKey << L"\\" << valueName
                << L" 值: " << value << std::endl;
        }
        return value;
    }

    std::vector<std::wstring> RemoveDuplicates(const std::vector<std::wstring>& paths) {
        std::vector<std::wstring> uniquePaths;
        for (const auto& path : paths) {
            if (std::find(uniquePaths.begin(), uniquePaths.end(), path) == uniquePaths.end()) {
                uniquePaths.push_back(path);
            }
        }
        return uniquePaths;
    }

    //--------------------------------------------------
    // 检测逻辑
    //--------------------------------------------------
    void CheckEnvironmentVariable() {
        wchar_t javaHome[MAX_PATH] = { 0 };
        DWORD javaHomeLen = GetEnvironmentVariableW(L"JAVA_HOME", javaHome, MAX_PATH);

        if (javaHomeLen == 0) {
            if (debug_) std::wcout << L"[DEBUG] 环境变量 JAVA_HOME 未设置" << std::endl;
            return;
        }

        std::wstring javaPath = std::wstring(javaHome) + L"\\bin\\java.exe";
        if (PathExists(javaPath)) {
            javaPaths_.push_back(javaPath);
            if (debug_) std::wcout << L"[DEBUG] 通过 JAVA_HOME 找到: " << javaPath << std::endl;
        }
        else {
            if (debug_) std::wcout << L"[DEBUG] JAVA_HOME 指向无效路径: " << javaPath << std::endl;
        }
    }

    void CheckRegistry() {
        const std::vector<std::wstring> registryKeys = {
            // Oracle
            L"SOFTWARE\\JavaSoft\\Java Runtime Environment",
            L"SOFTWARE\\JavaSoft\\Java Development Kit",
            // Adoptium/Temurin
            L"SOFTWARE\\Eclipse Adoptium\\JDK",
            L"SOFTWARE\\Eclipse Foundation\\JDK",
            // AdoptOpenJDK
            L"SOFTWARE\\AdoptOpenJDK\\JDK",
            // Azul Zulu
            L"SOFTWARE\\Azul Systems\\Zulu",
            // Microsoft
            L"SOFTWARE\\Microsoft\\JDK"
        };

        for (const auto& baseKey : registryKeys) {
            for (DWORD flags : {KEY_WOW64_64KEY, KEY_WOW64_32KEY}) {
                if (debug_) {
                    std::wcout << L"[DEBUG] 扫描注册表: " << baseKey
                        << (flags == KEY_WOW64_64KEY ? L" (64位)" : L" (32位)") << std::endl;
                }

                std::wstring currentVersion = ReadRegistryString(
                    HKEY_LOCAL_MACHINE, baseKey, L"CurrentVersion", flags);

                if (currentVersion.empty()) continue;

                std::wstring subKey = baseKey + L"\\" + currentVersion;
                std::wstring javaHome = ReadRegistryString(
                    HKEY_LOCAL_MACHINE, subKey, L"JavaHome", flags);

                if (javaHome.empty()) continue;

                std::wstring javaPath = javaHome + L"\\bin\\java.exe";
                if (PathExists(javaPath)) {
                    javaPaths_.push_back(javaPath);
                    if (debug_) std::wcout << L"[DEBUG] 注册表找到有效路径: " << javaPath << std::endl;
                }
            }
        }
    }

    void ScanDefaultDirectories() {
        wchar_t* folders[] = { nullptr, nullptr };
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr, 0, (LPWSTR)&folders[0])) ){
            folders[1] = folders[0]; // 兼容 x86 系统
        }
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILESX86, nullptr, 0, (LPWSTR)&folders[1]))) {}

        for (wchar_t* programFiles : folders) {
            if (!programFiles) continue;

            std::wstring javaDir = std::wstring(programFiles) + L"\\Java";
            if (debug_) std::wcout << L"[DEBUG] 扫描默认目录: " << javaDir << std::endl;

            WIN32_FIND_DATAW findData;
            HANDLE hFind = FindFirstFileW((javaDir + L"\\*").c_str(), &findData);
            if (hFind == INVALID_HANDLE_VALUE) continue;

            do {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    std::wstring dirName = findData.cFileName;
                    if (dirName == L"." || dirName == L"..") continue;

                    if (dirName.find(L"jdk") != std::wstring::npos ||
                        dirName.find(L"jre") != std::wstring::npos ||
                        dirName.find(L"java") != std::wstring::npos) {
                        std::wstring javaPath = javaDir + L"\\" + dirName + L"\\bin\\java.exe";
                        if (PathExists(javaPath)) {
                            javaPaths_.push_back(javaPath);
                            if (debug_) std::wcout << L"[DEBUG] 目录扫描找到: " << javaPath << std::endl;
                        }
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }

    void ScanCustomDirectories() {
        const std::vector<std::wstring> customPaths = {
            L"D:\\Java",
            L"C:\\Program Files\\AdoptOpenJDK",
            L"C:\\Program Files\\Eclipse Foundation",
            L"C:\\Program Files\\Zulu"
        };

        for (const auto& path : customPaths) {
            if (debug_) std::wcout << L"[DEBUG] 扫描自定义目录: " << path << std::endl;

            WIN32_FIND_DATAW findData;
            HANDLE hFind = FindFirstFileW((path + L"\\*").c_str(), &findData);
            if (hFind == INVALID_HANDLE_VALUE) continue;

            do {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    std::wstring dirName = findData.cFileName;
                    if (dirName == L"." || dirName == L"..") continue;

                    std::wstring javaPath = path + L"\\" + dirName + L"\\bin\\java.exe";
                    if (PathExists(javaPath)) {
                        javaPaths_.push_back(javaPath);
                        if (debug_) std::wcout << L"[DEBUG] 自定义目录找到: " << javaPath << std::endl;
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }
};