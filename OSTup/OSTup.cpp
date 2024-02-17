#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem> 
#include <windows.h> 
#include <shlobj.h> 

#pragma comment(lib, "Shell32.lib") 

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

void installPrograms(const std::string& diskPath) {
    system("cls");
    std::cout << "Installing programs" << std::endl;

    std::string downloadsPath = diskPath + "Reset/Downloads";
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

void setupProgram(const std::string& diskPath) {
    system("cls");
    std::cout << "Setting up programs" << std::endl;

    std::cout << "\nConfiguring Spicetify" << std::endl;
    std::string powershellCommand = "powershell -Command \"iwr -useb https://raw.githubusercontent.com/spicetify/spicetify-marketplace/main/resources/install.ps1 | iex\"";
    system(powershellCommand.c_str());

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


    std::cout << "\nSetup complete." << std::endl;

    system("pause");
    system("cls");
}

int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

    std::cout << "Welcome to OsmanSetup!\n";
    std::cout << "-------------------------------------------------------------" << std::endl;
    std::cout << "Choose one of your disks." << std::endl;

    FILE* pipe = _popen("wmic logicaldisk get name, volumename", "r");
    if (!pipe) {
        std::cerr << "Error executing command." << std::endl;
        return 1;
    }

    char buffer[128];
    std::vector<std::pair<std::string, std::string>> disks;
    int count = 1;
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        std::stringstream ss(buffer);
        std::string drive, volume;
        ss >> drive >> volume;
        if (!drive.empty() && drive != "Name") {
            disks.push_back(std::make_pair(drive, volume));
            std::cout << count << ". " << drive << " (" << volume << ")" << std::endl;
            count++;
        }
    }

    _pclose(pipe);

    std::cout << "\nEnter the number corresponding to the disk you want to select: ";
    int diskChoice;
    std::cin >> diskChoice;

    if (diskChoice < 1 || diskChoice > static_cast<int>(disks.size())) {
        std::cerr << "Invalid disk choice." << std::endl;
        return 1;
    }

    std::string selectedDisk = disks[diskChoice - 1].first;
    std::string diskPath = selectedDisk + "\\";

    system("cls");

    std::cout << "\nYou have chosen a disk: ";
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
    std::cout << selectedDisk << std::endl;
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

    // Menu
    int choice;
    do {
        system("cls");
        std::cout << "Menu:\n";
        std::cout << "1. Install Programs\n";
        std::cout << "2. Setup Program\n";
        std::cout << "3. Exit\n";
        std::cout << "Enter your choice: ";

        std::cin >> choice;

        // Menu selection handling
        switch (choice) {
        case 1:
            installPrograms(diskPath);
            break;
        case 2:
            setupProgram(diskPath);
            break;
        case 3:
            system("cls");

            std::cout << "\nExiting program." << std::endl;
            break;
        default:
            std::cerr << "Invalid choice. Please try again." << std::endl;
        }
    } while (choice != 3);

    std::cout << "\n-------------------------------------------------------------" << std::endl;

    return 0;
}
