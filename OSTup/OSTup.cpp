#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem> 
#include <windows.h> 
#include <shlobj.h> 
#include <cstdlib>
#include <powrprof.h>
#include <tchar.h>
#include <cstdint>
#include <winreg.h>
#include <initguid.h>
#include <GPEdit.h>
#include <array>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "Powrprof.lib")
#pragma comment(lib, "Shell32.lib") 

struct App {
    std::string cmd;
    std::string name;
};

bool IsRunAsAdmin()
{
    BOOL isRunAsAdmin = FALSE;
    PSID adminGroupSid = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroupSid))
    {
        if (!CheckTokenMembership(NULL, adminGroupSid, &isRunAsAdmin))
        {
            isRunAsAdmin = FALSE;
        }
        FreeSid(adminGroupSid);
    }

    return isRunAsAdmin;
}

std::string executeCommand(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string trim(const std::string& str)
{
    auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    return (start < end) ? std::string(start, end) : std::string();
}

void listRegistryKey(HKEY hkey, LPCTSTR subkey) {
    HKEY keyHandle;
    LONG result = RegOpenKeyEx(hkey, subkey, 0, KEY_READ, &keyHandle);
    if (result == ERROR_SUCCESS) {
        TCHAR valueName[256];
        DWORD valueNameSize, valueType;
        BYTE valueData[1024];
        DWORD valueDataSize = sizeof(valueData);
        DWORD i = 0;

        while (RegEnumValue(keyHandle, i, valueName, &(valueNameSize = sizeof(valueName)), NULL, &valueType, valueData, &valueDataSize) == ERROR_SUCCESS) {
            std::wcout << valueName << std::endl;
            i++;
        }

        RegCloseKey(keyHandle);
    }
    else {
        std::cerr << "Failed to open registry key." << std::endl;
    }
}

void listStartupFolder(int csidl) {
    TCHAR path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, csidl, NULL, 0, path))) {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            std::wcout << entry.path().filename().wstring() << std::endl;
        }
    }
    else {
        std::cerr << "Failed to get Startup folder path." << std::endl;
    }
}
void listStartupPrograms() {
    listRegistryKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"));
    listRegistryKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"));
}

void listFilesInDirectory(const std::string& path, WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (!entry.is_directory()) {
            std::wcout << "    " << entry.path().filename() << std::endl;
        }
    }

    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
}

void executeFile(const std::wstring& filePath) {
    ShellExecute(NULL, L"open", filePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void installPrograms(const std::string& selectedPath) {
    system("cls");
    std::cout << "Installing programs" << std::endl;

    std::string downloadsPath = selectedPath;
    listFilesInDirectory(downloadsPath, FOREGROUND_GREEN);
    std::cout << "\nDownloading files in Downloads folder..." << std::endl;
    for (const auto& entry : std::filesystem::directory_iterator(downloadsPath)) {
        if (!entry.is_directory()) {
            executeFile(entry.path());
        }
    }

    std::cout << "Installation complete." << std::endl;

    system("pause");
    system("cls");
}

void setupProgram(const std::string& selectedPath) {
    system("cls");
    std::cout << "Setting up programs" << std::endl;

    std::vector<App> apps = {
        {"powershell -Command \"iwr -useb https://raw.githubusercontent.com/spicetify/spicetify-marketplace/main/resources/install.ps1 | iex\"", "Spicetify"},
        {"https://github.com/Vencord/Installer/releases/latest/download/VencordInstallerCli.exe", "VenCord"},
    };

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    for (const App& app : apps) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
        std::cout << "  - " << app.name << "\n";
        SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
    }

    std::string appChoice;
    std::cout << "\nEnter the name of the program you want to setup: ";
    std::cin >> appChoice;

    if (appChoice == "Spicetify") {
        std::cout << "\nConfiguring Spicetify" << std::endl;
        std::string powershellCommand = "powershell -Command \"iwr -useb https://raw.githubusercontent.com/spicetify/spicetify-marketplace/main/resources/install.ps1 | iex\"";
        system(powershellCommand.c_str());
    }
    else if (appChoice == "VenCord") {
        PWSTR appDataPath;
        SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath);
        std::wstring appDataPathW(appDataPath);
        std::string appDataPathA(appDataPathW.begin(), appDataPathW.end());
        CoTaskMemFree(appDataPath);

        std::string osmanSetupPath = appDataPathA + "\\OsmanSetup";
        std::string downloadsPath = osmanSetupPath + "\\Downloads";
        std::filesystem::create_directories(downloadsPath);

        system("cls");

        std::cout << "\nConfiguring VenCord" << std::endl;
        std::string vencordURL = "https://github.com/Vencord/Installer/releases/latest/download/VencordInstallerCli.exe";
        std::string curlCommand = "curl -L -o " + downloadsPath + "\\VencordInstallerCli.exe " + vencordURL;
        std::cout << "Downloading VenCord..." << std::endl;
        system(curlCommand.c_str());

        system("cls");
        std::cout << "\nRunning VenCord" << std::endl;
        std::string vencordPath = downloadsPath + "\\VencordInstallerCli.exe";
        executeFile(std::wstring(vencordPath.begin(), vencordPath.end()));
    }

    std::cout << "\nSetup complete." << std::endl;

    system("pause");
    system("cls");
}

int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    std::cout << std::endl;
    std::cout << "  /$$$$$$                                               /$$$$$$              /$$                              \n";
    std::cout << " /$$__  $$                                             /$$__  $$            | $$                              \n";
    std::cout << "| $$  \\ $$  /$$$$$$$ /$$$$$$/$$$$   /$$$$$$  /$$$$$$$ | $$  \\__/  /$$$$$$  /$$$$$$   /$$   /$$  /$$$$$$       \n";
    std::cout << "| $$  | $$ /$$_____/| $$_  $$_  $$ |____  $$| $$__  $$|  $$$$$$  /$$__  $$|_  $$_/  | $$  | $$ /$$__  $$      \n";
    std::cout << "| $$  | $$|  $$$$$$ | $$ \\ $$ \\ $$  /$$$$$$$| $$  \\ $$ \\____  $$| $$$$$$$$  | $$    | $$  | $$| $$  \\ $$      \n";
    std::cout << "| $$  | $$ \\____  $$| $$ | $$ | $$ /$$__  $$| $$  | $$ /$$  \\ $$| $$_____/  | $$ /$$| $$  | $$| $$  | $$      \n";
    std::cout << "|  $$$$$$/ /$$$$$$$/| $$ | $$ | $$|  $$$$$$$| $$  | $$|  $$$$$$/|  $$$$$$$  |  $$$$/|  $$$$$$/| $$$$$$$/      \n";
    std::cout << " \\______/ |_______/ |__/ |__/ |__/ \\_______/|__/  |__/ \\______/  \\_______/   \\___/   \\______/ | $$____/       \n";
    std::cout << "                                                                                              | $$            \n";
    std::cout << "                                                                                              | $$            \n";
    std::cout << "                                                                                              |__/            \n";

    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); 

    if (!IsRunAsAdmin())
    {
        std::cout << "Please run this program as an administrator!" << std::endl;
        return 1;
    }


    std::cout << "-------------------------------------------------------------" << std::endl;
    std::cout << "Input the path of your installion downloads folder:" << std::endl;

    std::string selectedPath;
    std::cin >> selectedPath;
    std::string selectedFolder = selectedPath.substr(selectedPath.find_last_of("\\") + 1); 


    system("cls");

    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
    std::cout << "\nYou have chosen this folder: ";
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
    std::cout << selectedFolder << std::endl;
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
    Sleep(1000);

    // Initialize Variables
    uint32_t payload = 1;
    std::string motherboard = executeCommand("wmic baseboard get product");
    motherboard = trim(motherboard);
    std::string url = "https://www.google.com/search?q=" + motherboard + "+motherboard+drivers";


    // Menu
    int choice;
    do {
        system("cls");
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::cout << std::endl;
        std::cout << "  /$$$$$$                                               /$$$$$$              /$$                              \n";
        std::cout << " /$$__  $$                                             /$$__  $$            | $$                              \n";
        std::cout << "| $$  \\ $$  /$$$$$$$ /$$$$$$/$$$$   /$$$$$$  /$$$$$$$ | $$  \\__/  /$$$$$$  /$$$$$$   /$$   /$$  /$$$$$$       \n";
        std::cout << "| $$  | $$ /$$_____/| $$_  $$_  $$ |____  $$| $$__  $$|  $$$$$$  /$$__  $$|_  $$_/  | $$  | $$ /$$__  $$      \n";
        std::cout << "| $$  | $$|  $$$$$$ | $$ \\ $$ \\ $$  /$$$$$$$| $$  \\ $$ \\____  $$| $$$$$$$$  | $$    | $$  | $$| $$  \\ $$      \n";
        std::cout << "| $$  | $$ \\____  $$| $$ | $$ | $$ /$$__  $$| $$  | $$ /$$  \\ $$| $$_____/  | $$ /$$| $$  | $$| $$  | $$      \n";
        std::cout << "|  $$$$$$/ /$$$$$$$/| $$ | $$ | $$|  $$$$$$$| $$  | $$|  $$$$$$/|  $$$$$$$  |  $$$$/|  $$$$$$/| $$$$$$$/      \n";
        std::cout << " \\______/ |_______/ |__/ |__/ |__/ \\_______/|__/  |__/ \\______/  \\_______/   \\___/   \\______/ | $$____/       \n";
        std::cout << "                                                                                              | $$            \n";
        std::cout << "                                                                                              | $$            \n";
        std::cout << "                                                                                              |__/            \n";

        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        std::cout << std::endl;

        std::cout << "Menu:\n";
        std::cout << std::endl;
        std::cout << "1. Install Programs\n";
        std::cout << "2. Setup Program\n";
        std::cout << "3. Optimizations\n";
        std::cout << "4. Find your drivers\n";
        std::cout << "5. Exit\n";
        std::cout << std::endl;
        std::cout << "Enter your choice: ";

        std::cin >> choice;

        // menu options
        switch (choice) {
        case 1:
            installPrograms(selectedPath);
            break;
        case 2:
            setupProgram(selectedPath);
            break;
        case 3:
            system("cls");
            std::cout << "Optimizing..." << std::endl;

            if (system("powercfg /LIST") != 0) {
                std::cerr << "\nFailed to get the list of power schemes." << std::endl;
            }

            if (system("powercfg /SETACTIVE e08daac4-7f48-4819-97cf-ce5c27a203df") != 0) {
                std::cerr << "\nFailed to set the power scheme." << std::endl;
            }
            else {
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                std::cout << "\nPower scheme set to Ultimate Performance." << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
            }

            std::cout << std::endl;
            std::cout << std::endl;
            std::cout << std::endl;

            std::cout << "You should disable the following startup programs...\n" << std::endl;
            listStartupPrograms();

            std::cout << "\nDisabling Windows Defender...\n" << std::endl;
            HKEY key;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows Defender", 0, KEY_ALL_ACCESS, &key)) {
                std::cout << "[-] failed to open registry.\n";
                system("pause");
                return 0;
            }

            if (RegSetValueEx(key, L"DisableAntiSpyware", 0, REG_DWORD, (LPBYTE)&payload, sizeof(payload))) {
                std::cout << "[-] failed to write to registry.\n";
                system("pause");
                return 0;
            }

            HKEY new_key;
            if (RegCreateKeyEx(key, L"Real-Time Protection", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &new_key, 0)) {
                std::cout << "[-] failed to write to create new key.\n";
                system("pause");
                return 0;
            }
            key = new_key;

            if (RegSetValueEx(key, L"DisableRealtimeMonitoring", 0, REG_DWORD, (LPBYTE)&payload, sizeof(payload))) {
                std::cout << "[-] failed to write to registry.\n";
                system("pause");
                return 0;
            }

            if (RegSetValueEx(key, L"DisableBehaviorMonitoring", 0, REG_DWORD, (LPBYTE)&payload, sizeof(payload))) {
                std::cout << "[-] failed to write to registry.\n";
                system("pause");
                return 0;
            }

            if (RegSetValueEx(key, L"DisableOnAccessProtection", 0, REG_DWORD, (LPBYTE)&payload, sizeof(payload))) {
                std::cout << "[-] failed to write to registry.\n";
                system("pause");
                return 0;
            }

            if (RegSetValueEx(key, L"DisableScanOnRealtimeEnable", 0, REG_DWORD, (LPBYTE)&payload, sizeof(payload))) {
                std::cout << "[-] failed to write to registry.\n";
                system("pause");
                return 0;
            }

            if (RegSetValueEx(key, L"DisableIOAVProtection", 0, REG_DWORD, (LPBYTE)&payload, sizeof(payload))) {
                std::cout << "[-] failed to write to registry.\n";
                system("pause");
                return 0;
            }

            RegCloseKey(key);

            std::cout << "[+] registry values written\nDefender will be changed on restart.\n";

            system("pause");
            break;
        case 4:
            system("cls");

            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
            std::cout << "\nCPU, GPU and Motherboard Info:\n" << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

            system("wmic cpu get name");
			std::cout << std::endl; 

            system("wmic path win32_videocontroller get name");
          

            std::cout << std::endl;
            system("wmic baseboard get product");

            std::cout << "\n" << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            std::cout << "You should download the latest drivers for your components:\n" << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

            if (!system("wmic cpu get name | findstr /i \"intel\"")) {
                std::cout << "CPU: https://downloadcenter.intel.com/" << std::endl;
            }
            else {
                std::cout << "CPU: https://www.amd.com/en/support" << std::endl;
            }

            std::cout << std::endl;
            if (!system("wmic path win32_videocontroller get name | findstr /i \"nvidia\"")) {
                std::cout << "\nGPU: https://www.nvidia.com/en-us/geforce/geforce-experience/" << std::endl;
            }
            else if (!system("wmic path win32_videocontroller get name | findstr /i \"amd\"")) {
                std::cout << "GPU: https://www.amd.com/en/support" << std::endl;
            }
            else {
                std::cout << "GPU: Integrated" << std::endl;
            }

            std::cout << std::endl;
            std::cout << "Motherboard: " << url << std::endl;

            system("pause");
            break;
        case 5:
            system("cls");
            std::cout << "Exiting..." << std::endl;
            break;
        default:
            std::cout << "Invalid choice. Please try again." << std::endl;
            break;
        }
	} while (choice != 5);


    std::cout << "\n-------------------------------------------------------------" << std::endl;

    return 0;
}
