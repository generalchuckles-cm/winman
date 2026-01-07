#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <winhttp.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tchar.h>

// Link necessary libraries
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")

using namespace std;

// TLS 1.2 definition for older VS2010 headers
#ifndef WINHTTP_OPTION_SECURE_PROTOCOLS
#define WINHTTP_OPTION_SECURE_PROTOCOLS 94
#endif
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0x00000800
#endif

// Structure for packages
struct PackageFile {
    wstring fullName;
    wstring shortName;
    wstring version;
    wstring downloadUrl;
};

// --- Helper: Update User PATH ---
void AddToPath(wstring newPath) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
        wchar_t oldPath[4096];
        DWORD pathLen = sizeof(oldPath);
        DWORD type = REG_EXPAND_SZ;

        if (RegQueryValueExW(hKey, L"Path", NULL, &type, (LPBYTE)oldPath, &pathLen) != ERROR_SUCCESS) {
            oldPath[0] = L'\0';
        }

        wstring pathStr(oldPath);
        if (pathStr.find(newPath) == wstring::npos) {
            wstring updatedPath = pathStr;
            if (!updatedPath.empty() && updatedPath[updatedPath.length() - 1] != L';') {
                updatedPath += L';';
            }
            updatedPath += newPath;

            RegSetValueExW(hKey, L"Path", 0, REG_EXPAND_SZ, (LPBYTE)updatedPath.c_str(), (DWORD)((updatedPath.length() + 1) * sizeof(wchar_t)));
            
            // Notify system of change
            DWORD_PTR dwReturnValue;
            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, &dwReturnValue);
            
            wcout << L"[+] Path updated: " << newPath << endl;
        }
        RegCloseKey(hKey);
    }
}

// --- Windows 7 compatible Zip Extraction ---
void ExtractZip(wstring zipFile, wstring targetDir) {
    wcout << L"[*] Extracting to: " << targetDir << endl;
    CreateDirectoryW(targetDir.c_str(), NULL);
    
    IShellDispatch *pISD;
    Folder *pToFolder;
    VARIANT vDir, vFile, vOpt, vItems;

    CoInitialize(NULL);
    if (CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void **)&pISD) == S_OK) {
        VariantInit(&vDir);
        vDir.vt = VT_BSTR; 
        vDir.bstrVal = SysAllocString(targetDir.c_str());
        pISD->NameSpace(vDir, &pToFolder);

        VariantInit(&vFile);
        vFile.vt = VT_BSTR; 
        vFile.bstrVal = SysAllocString(zipFile.c_str());
        
        Folder *pFromFolder;
        pISD->NameSpace(vFile, &pFromFolder);

        if (pFromFolder && pToFolder) {
            FolderItems *pItems;
            pFromFolder->Items(&pItems);
            
            VariantInit(&vOpt);
            vOpt.vt = VT_I4; 
            vOpt.lVal = 4 | 16; 

            VariantInit(&vItems);
            vItems.vt = VT_DISPATCH;
            vItems.pdispVal = (IDispatch*)pItems;

            pToFolder->CopyHere(vItems, vOpt);
            pFromFolder->Release();
            pToFolder->Release();
        }
        pISD->Release();
        SysFreeString(vDir.bstrVal);
        SysFreeString(vFile.bstrVal);
        CoUninitialize();
    }
}

// --- Download File via WinHTTP (with TLS 1.2 for Win7) ---
bool DownloadFile(wstring url, wstring savePath) {
    // Note: URL parsing and full WinHTTP implementation needed here
    // For now, this is a placeholder indicating where the download logic resides
    wcout << L"[i] Downloading: " << url << endl;
    return false; 
}

// --- Search for Binaries/Architecture ---
void SmartInstallZip(wstring pkgName, wstring extractedDir) {
    wstring binPath = extractedDir + L"\\bin";
    wstring scriptsPath = extractedDir + L"\\Scripts";

    if (PathIsDirectoryW(binPath.c_str())) {
        AddToPath(binPath);
    } else if (PathIsDirectoryW(scriptsPath.c_str())) {
        AddToPath(scriptsPath);
    } else {
        AddToPath(extractedDir);
    }
}

// --- GitHub API Scanner (Simulated for this structure) ---
vector<PackageFile> FetchRepoFolder(wstring folder) {
    vector<PackageFile> results;
    // Implementation: Use WinHTTP to GET https://api.github.com/repos/generalchuckles-cm/winman/contents/[folder]
    // Parse JSON manually for "name" and "download_url"
    return results;
}

void ProcessPackage(wstring targetPkg, wstring targetVer) {
    vector<wstring> searchPath;
    searchPath.push_back(L"uni");
    searchPath.push_back(L"win11");
    searchPath.push_back(L"win10");
    searchPath.push_back(L"win8");
    searchPath.push_back(L"win7");

    PackageFile bestMatch;
    bestMatch.fullName = L"";
    bestMatch.version = L"";

    for (size_t i = 0; i < searchPath.size(); ++i) {
        vector<PackageFile> files = FetchRepoFolder(searchPath[i]);
        
        for (size_t j = 0; j < files.size(); ++j) {
            // Logic: Split by '.' and match first part
            if (files[j].shortName == targetPkg) {
                if (targetVer.empty() || files[j].version == targetVer) {
                    if (files[j].version >= bestMatch.version) {
                        bestMatch = files[j];
                    }
                }
            }
        }
        if (!bestMatch.fullName.empty()) break;
    }

    if (bestMatch.fullName.empty()) {
        wcout << L"[!] Package " << targetPkg << L" not found." << endl;
        return;
    }

    wstring tempFile = L"C:\\Windows\\Temp\\" + bestMatch.fullName;
    if (DownloadFile(bestMatch.downloadUrl, tempFile)) {
        if (bestMatch.fullName.find(L".zip") != wstring::npos) {
            wstring installDir = L"C:\\winman\\apps\\" + targetPkg;
            ExtractZip(tempFile, installDir);
            SmartInstallZip(targetPkg, installDir);
        } else {
            wstring flags = L"/quiet /S";
            if (targetPkg == L"python") flags = L"/quiet InstallAllUsers=1 PrependPath=1";
            ShellExecuteW(NULL, L"open", tempFile.c_str(), flags.c_str(), NULL, SW_SHOW);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "winman install <package> [version]" << endl;
        return 1;
    }

    string cmd = argv[1];
    if (cmd == "install") {
        string pName = argv[2];
        wstring pkg(pName.begin(), pName.end());
        wstring ver = L"";
        if (argc == 4) {
            string pVer = argv[3];
            ver.assign(pVer.begin(), pVer.end());
        }
        ProcessPackage(pkg, ver);
    }

    return 0;
}