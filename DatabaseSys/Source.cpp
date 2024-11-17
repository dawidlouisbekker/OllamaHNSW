#include <iostream>
#include <windows.h>
#include <winhttp.h>
#include <cstdlib>  
#include <cstdio>
#include <iostream>
#include <windows.h>
#include <chrono>
#include <thread>
#include <string>
#include <fstream>
#include <vector>
#include <regex>
#include <filesystem>
#include <locale>
#include <codecvt>



//#include <poppler/cpp/poppler-document.h>
//#include <poppler/cpp/poppler-page.h>

#include "hnswlist.h"
#include <smmintrin.h>



#pragma comment(lib, "winhttp.lib")

using namespace std;

//extern "C" char GetCharAtAddress(void* address, int count);

class StringManip {
public:
    StringManip(string Target) {
        target = Target;
    }

    bool FoundI(char ch) {
        return true;
    }


private:
    string target;
};

std::string normalizeForJson(const std::string& input) {
    std::string output = input;
    output = std::regex_replace(output, std::regex("\r?\n"), " ");
    output = std::regex_replace(output, std::regex(R"(")"), "'");
    output = std::regex_replace(output, std::regex(R"(\\)"), R"(\\\\)");

    return output;
}



class Directories {
public:

    Directories() {
        wchar_t buffer[MAX_PATH];
        GetModuleFileName(NULL, buffer, MAX_PATH);

        char charBuffer[MAX_PATH];
        size_t numConverted;
        wcstombs_s(&numConverted, charBuffer, buffer, MAX_PATH);
        ////////might not be 0 terminated
        std::string execPath(charBuffer);
        std::string::size_type pos = execPath.find_last_of("\\/");
        ExecutableDirectory = execPath.substr(0, pos);

    }
   
    void CreateModelDirectory(string& model, int VecSize) {
        currentDirectory = ExecutableDirectory;
        AddDirectory(model);
    }

    string GetExeDir() {
        return ExecutableDirectory;
    }

    string GetCurrentDir() {
        return currentDirectory;
    }

    int ListDirectories(std::vector<std::string>& models) {
        string path = ExecutableDirectory + "\\models";
        // Check if the given path exists and is a directory
        if (filesystem::exists(path) && filesystem::is_directory(path)) {
            for (const auto& entry : filesystem::directory_iterator(path)) {
                // Check if the entry is a directory
                if (filesystem::is_directory(entry.path())) {
                    models.push_back(entry.path().filename().string());
                }
            }
        }
        else {
            return -1;
        }

        return 1;
    }

    int ListCollections(std::vector<std::string>& collections) {
        cout << currentDirectory << endl;
        if (filesystem::exists(currentDirectory) && filesystem::is_directory(currentDirectory)) {
            for (const auto& entry : filesystem::directory_iterator(currentDirectory)) {
                // Check if the entry is a directory
                if (filesystem::is_directory(entry.path())) {
                    collections.push_back(entry.path().filename().string());
                }
            }
        }
        else {
            return -1;
        }

        return 1;
    }

    int ListFiles(std::vector<std::string>& files) {
        if (std::filesystem::exists(currentDirectory) && std::filesystem::is_directory(currentDirectory)) {
            for (const auto& entry : std::filesystem::directory_iterator(currentDirectory)) {
                if (std::filesystem::is_regular_file(entry.path())) {
                    std::string fileName = entry.path().stem().string(); 
                    files.push_back(fileName); 
                }
            }
        }
        else {
            return -1; 
        }
        return 1;  
    }

    int AddFile(string FilePath) {
        filesystem::path entry(FilePath);
        if (std::filesystem::is_regular_file(entry)) {
            std::string fileName = entry.stem().string();
            string path = currentDirectory + "\\" + fileName + ".bin";
            FileBinFile = path;
            ofstream file(path);
            file.close();
            return 1;
        }
        return -1;
    }

    int SetModelDir(const std::string& model) {
        currentDirectory = ExecutableDirectory + "\\models\\" + model;
        modelBinFile  = currentDirectory + "\\" + "size.bin";
        std::ifstream VecSizeFile(modelBinFile, std::ios::binary | std::ios::ate);
        if (!VecSizeFile) {
            //  std::cerr << "Error: Unable to open file." << std::endl; 
            return -2; // Indicate an error
        }
        std::streamsize fileSize = VecSizeFile.tellg(); VecSizeFile.seekg(0, std::ios::beg); // Check if file size is at least 8 bytes (2 integers of 4 bytes each) 
        if (fileSize < sizeof(int) * 2) {
            std::cerr << "Error: File size is less than required 8 bytes." << std::endl;
            return -1; // Indicate an error 
        } // Read two integers from the file 
        int VecSize;
        int NodeAmnt;
        VecSizeFile.read(reinterpret_cast<char*>(&VecSize), sizeof(int));
        if (VecSizeFile) {
            return VecSize;
        }
        else {
            std::cerr << "Error: Unable to read integers from file." << std::endl;
            return -1; // Indicate an error 
        }
    }

    string getModelBinFile() {
        return modelBinFile;
    }

    void SetCollectionDir(string& Name) {
        // string dir = GetExecutableDirectory();

        currentDirectory = currentDirectory + "\\" + Name;
        CollectionBinFile = currentDirectory + "\\" + "collec.bin";

        return;
    }

    string GetCollectionBinFile() {
        return CollectionBinFile;
    }

    void SetCurrentFile(string filePath) {
        std::filesystem::path path(filePath);
        string stem = path.stem().string();
        FileBinFile = currentDirectory + "\\" + stem + ".bin";
    }

    string GetFileBin() {
        return FileBinFile;
    }

    void CreateCollection(string& name) {
        //currentDirectory = currentDirectory+ "\\" + name;
        cout << currentDirectory << endl;
        AddDirectory(name);

    }

    void AddModel() {
        //////////
    }








private:
    string currentDirectory = "";
    string modelBinFile = "";
    string CollectionBinFile = "";
    string FileBinFile = "";
    string ExecutableDirectory = "";
    void AddDirectory(string& Dir) {
        //  const char* exeDirC = GetExecutableDirectory().c_str();
        cout << currentDirectory << endl;
        std::string newDirectory = currentDirectory + "\\" + Dir; // Create the new directory 
        cout << endl << "New dir:" << newDirectory;
        if (CreateDirectoryA(newDirectory.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError()) {
            std::cout << "Directory created successfully or already exists: " << newDirectory << endl;
            currentDirectory = newDirectory;

        }
        else
        {
            std::cerr << "Error: Unable to create directory." << std::endl;
        }
        return;
    }
};

//////Setfile before refrencing heap Directories Class
class MemoryMap {
public:
    MemoryMap(int VecSize, shared_ptr<Directories> &refDir, string& message, bool newModel = false) : vecSize(0), initialized(false), hFile(NULL), fileSize(0), hMapping(NULL), pMap(NULL) {
        vecSize = VecSize;
        Dir = refDir;
    }

    void GetHNSW(shared_ptr<HNSW> &Hnsw ) {
        Hnsw = hnsw;
    }

    ~MemoryMap() {
        if (pMap && *pMap) {
            UnmapViewOfFile(*pMap);
            delete pMap;
        }
        if (hMapping && *hMapping) {
            CloseHandle(*hMapping);
            delete hMapping;
        }
        if (hFile && *hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(*hFile);
            delete hFile;
        }
        if (fileSize) {
            delete fileSize;
        }

    }//C:\vsCode\HSWN Vector search\ITAIA1-B33_Week 7_Slides 1.bin
    bool MapFile(std::string& message) {
        string path = Dir->GetFileBin();
        cout << endl << path << endl;
        const char* testPath = "C:\\Users\\dawid\\source\\repos\\DatabaseSys\\x64\\Debug\\models\\nomic-embed-text\\ITCTA\\ITCTA1-44 Week 7.bin";
        // Open the file
        hFile = new HANDLE;
        *hFile = CreateFileA(testPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (*hFile == INVALID_HANDLE_VALUE) {
            message = "Error opening file.";
            delete hFile;
            hFile = nullptr;
            return false;
        }

        // Get the file size
        fileSize = new DWORD;
        *fileSize = GetFileSize(*hFile, NULL);
        if (*fileSize == INVALID_FILE_SIZE) {
            message = "Error getting file size.";
            CloseHandle(*hFile);
            delete hFile;
            hFile = nullptr;
            delete fileSize;
            fileSize = nullptr;
            return false;
        }

        // Create a file mapping object
        hMapping = new HANDLE;
        *hMapping = CreateFileMappingA(*hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
        if (*hMapping == NULL) {
            message = "Error creating file mapping.";
            CloseHandle(*hFile);
            delete hFile;
            hFile = nullptr;
            delete fileSize;
            fileSize = nullptr;
            return false;
        }

        // Map the file into memory
        pMap = new LPVOID;
        *pMap = MapViewOfFile(*hMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        if (*pMap == NULL) {
            message = "Error mapping file into memory.";
            CloseHandle(*hMapping);
            delete hMapping;
            hMapping = nullptr;
            CloseHandle(*hFile);
            delete hFile;
            hFile = nullptr;
            delete fileSize;
            fileSize = nullptr;
            return false;
        }

        message = "File successfully mapped.";
        return true;
    }

    bool CreateHNSW(string& message) {
        if (this->fileSize == nullptr) {
            cout << "failed";
            return false;
        }
        hnsw = make_shared<HNSW>(pMap, 768, 10, *fileSize, 5,5);
       // hnsw->ShowList();
      //  hnsw->ShowNeighbours0();
        return true;
    }

    bool isInitialized() const { return initialized; }



private:
    shared_ptr<Directories> Dir;
 //   HttpHander* httpHandler;
    int vecSize;
    struct Node {
        int key;
        float* embeddingsStartPntr;
        int TextSize;
        char* embeddingsTextStartPntr;
        vector<vector<int>> NeighbourKeys; //1D is the layer number, 2D is the keys for the neighbours which will be used to retieve the pointers of the start of the neighbours data entry
        vector<vector<int*>> neighborKeyPntrs;  //1D is the layer number, 2D is the pointer to neighbors (inside the the layer) embeddings start 
    };
    shared_ptr<HNSW> hnsw;
    HANDLE* hFile;
    DWORD* fileSize;
    HANDLE* hMapping;
    LPVOID* pMap;
    bool initialized;
};

class HttpHandler {
public:
    HttpHandler(int& Status, shared_ptr<Directories> &refDir, const wchar_t* url = L"127.0.0.1", const int port = 11434) : hSession(nullptr), hConnect(nullptr), hRequest(nullptr), dwSize(nullptr), dwDownloaded(nullptr), bResults(nullptr), Url(L"127.0.0.1"), Port(0) {
        hSession = new HINTERNET;
      //  memMap = new MemoryMap();
        Dir = refDir;
        Url = url;
        Port = port;
        // Set timeout values for long-lived sessions
        HINTERNET hSession = WinHttpOpen(L"User-Agent: WinHTTP Example/1.1",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        HINTERNET hConnect = WinHttpConnect(hSession, url, port, 0);
        if (!hConnect) {
            std::cerr << "Error: Unable to connect to server." << std::endl;
            WinHttpCloseHandle(hSession);
            return;
        }

        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        Status = 0;
    }


    ~HttpHandler() {
        CleanUp();
    }

    int GetQueryEmbeddings(string text, string model, float* &vec) {
        auto start = std::chrono::high_resolution_clock::now();
        cout << "Generating query context" << endl;

        const wchar_t* endpoint = L"/api/embed";



        text = normalizeForJson(text);

        std::string json = "{\"model\": \"" + model + "\", \"input\": \"" + text + "\"}";

        cout << json;
        HINTERNET hSession = WinHttpOpen(L"User - Agent: WinHTTP Example / 1.1",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) {
            std::cerr << "Error: Unable to open WinHTTP session." << std::endl;
            return 1;
        }

        HINTERNET hConnect = WinHttpConnect(hSession, Url, Port, 0);
        if (!hConnect) {
            std::cerr << "Error: Unable to connect to server." << std::endl;
            WinHttpCloseHandle(hSession);
            return 1;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", endpoint,
            NULL, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_BYPASS_PROXY_CACHE);
        if (!hRequest) {
            std::cerr << "Error: Unable to open HTTP request." << std::endl;
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return 1;
        }

        // Set headers for JSON content type in UTF-8
        const wchar_t* headers = L"Content-Type: application/json; charset=utf-8";

        // Send the request with the JSON data as a UTF-8 string
        BOOL bResults = WinHttpSendRequest(hRequest, headers, -1L,
            (LPVOID)json.c_str(), json.length(),
            json.length(), 0);
        if (!bResults) {
            std::cerr << "Error: Unable to send HTTP request." << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return 1;
        }

        // Receive the response
        bResults = WinHttpReceiveResponse(hRequest, NULL);
        if (!bResults) {
            std::cerr << "Error: Unable to receive HTTP response." << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return 1;
        }

        //  vector<float>* 
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        std::string response;
        bool Numbers = false;
        int BracketCount = 0;
        bool added = false;
        bool valid = true;
        int count = 0;
        std::string val = "";
        int key = 1;
        int endKey = 111;
        float number;
        int pos = 0;
        do {
            // Check for available data
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                std::cerr << "Error: QueryDataAvailable failed." << std::endl;
                break;
            }
            //////////////////////use streaming instead maybe 
            // Allocate space for the buffer
            char* buffer = new char[dwSize + 1];
            if (!buffer) {
                std::cerr << "Error: Memory allocation failed." << std::endl;
                break;
            }

            // Read the data
            ZeroMemory(buffer, dwSize + 1);
            if (!WinHttpReadData(hRequest, (LPVOID)buffer, dwSize, &dwDownloaded)) {
                std::cerr << "Error: ReadData failed." << std::endl;
                delete[] buffer;
                break;
            }


            if (dwDownloaded > 0) {


                for (int i = 0; i < dwDownloaded; i++) {
                    // file << buffer[i];
                    if (buffer[i] == ']')
                    {
                        if (val != "") {
                            number = std::stof(val);
                            vec[pos] = number;
                            pos++;
                            val = "";
                        }

                        BracketCount = 0;
                    }

                    if (BracketCount == 2 && buffer[i] != ' ' && buffer[i] != '[' && buffer[i] != ']') {      //&& buffer[i] != ' ' && buffer[i] != '[' && buffer[i] != ']'
                        if (buffer[i] == ',') {
                            try {
                                number = std::stof(val);
                                // Dimensions.push_back(number);
                                vec[pos] = number;
                                pos++;
                            }
                            catch (const std::invalid_argument& e) {
                                std::cerr << "Error: Invalid argument - the string does not represent a valid float." << std::endl;
                                valid = false;
                                break;
                            }
                            catch (const std::out_of_range& e) {
                                std::cerr << "Error: Out of range - the string represents a float that is out of range." << std::endl;
                                valid = false;
                                break;
                            }
                            catch (...) {
                                std::cerr << "Error: An unknown error occurred during the conversion." << std::endl;
                                valid = false;
                                break;
                            }
                            // write to file instead   textVec->push_back(number);
                            val = "";
                        }
                        else if (buffer[i] != ',') {

                            val += buffer[i];
                        }

                    }
                    else {
                        //   size--;

                    }

                    if (buffer[i] == '[')
                    {
                        BracketCount++;
                    }

                }


            }
            response.append(buffer, dwDownloaded);
            delete[] buffer;
        } while (dwSize > 0);

        cout << "repnonse: " << endl;
        cout << response << endl;
        
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);


        auto end = std::chrono::high_resolution_clock::now();

        // Calculate the elapsed time
        std::chrono::duration<double> elapsed = end - start;

        // Output the elapsed time in seconds
        std::cout << "Elapsed time: " << elapsed.count() << " seconds." << std::endl << endl;

        return 1;
    }

    int GetBlockEmbeddings(string &text, string model, int blockSize, int overLap = 0) {

        auto start = std::chrono::high_resolution_clock::now();

        cout << endl << endl << "here " << endl << endl;

        const wchar_t* endpoint = L"/api/embed";

        text = normalizeForJson(text);

        std::string json = "{\"model\": \"" + model + "\", \"input\": \"" + text + "\"}";

        cout << json;
        HINTERNET hSession = WinHttpOpen(L"User - Agent: WinHTTP Example / 1.1",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) {
            std::cerr << "Error: Unable to open WinHTTP session." << std::endl;
            return 1;
        }

        HINTERNET hConnect = WinHttpConnect(hSession, Url, Port, 0);
        if (!hConnect) {
            std::cerr << "Error: Unable to connect to server." << std::endl;
            WinHttpCloseHandle(hSession);
            return 1;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", endpoint,
            NULL, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_BYPASS_PROXY_CACHE);
        if (!hRequest) {
            std::cerr << "Error: Unable to open HTTP request." << std::endl;
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return 1;
        }

        // Set headers for JSON content type in UTF-8
        const wchar_t* headers = L"Content-Type: application/json; charset=utf-8";

        // Send the request with the JSON data as a UTF-8 string
        BOOL bResults = WinHttpSendRequest(hRequest, headers, -1L,
            (LPVOID)json.c_str(), json.length(),
            json.length(), 0);
        if (!bResults) {
            std::cerr << "Error: Unable to send HTTP request." << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return 1;
        }

        // Receive the response
        bResults = WinHttpReceiveResponse(hRequest, NULL);
        if (!bResults) {
            std::cerr << "Error: Unable to receive HTTP response." << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return 1;
        }

      //  vector<float>* 
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        std::string response;
        bool Numbers = false;
        int BracketCount = 0;
        bool added = false;
        bool valid = true;
        int count = 0;
        std::string val = "";
        cout << endl << endl << "writing to: " << Dir->GetFileBin();
        ofstream file(Dir->GetFileBin(), ios::app | ios::binary );// 
        if (!file.is_open()) {
            cout << "Error opening file";
            return -1;
        }
        int key = 1;
        int endKey = 111;
        float number;
        do {
            // Check for available data
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                std::cerr << "Error: QueryDataAvailable failed." << std::endl;
                break;
            }
            //////////////////////use streaming instead maybe 
            // Allocate space for the buffer
            char* buffer = new char[dwSize + 1];
            if (!buffer) {
                std::cerr << "Error: Memory allocation failed." << std::endl;
                break;
            }

            // Read the data
            ZeroMemory(buffer, dwSize + 1);
            if (!WinHttpReadData(hRequest, (LPVOID)buffer, dwSize, &dwDownloaded)) {
                std::cerr << "Error: ReadData failed." << std::endl;
                delete[] buffer;
                break;
            }


            if (dwDownloaded > 0) {


                for (int i = 0; i < dwDownloaded; i++) {
                    // file << buffer[i];
                    if (buffer[i] == ']')
                    {
                        if (val != "") {
                            number = std::stof(val);
                            file.write(reinterpret_cast<const char*>(&number), sizeof(float));
                            val = "";
                        }

                        BracketCount = 0;
                    }

                    if (BracketCount == 2 && buffer[i] != ' ' && buffer[i] != '[' && buffer[i] != ']') {      //&& buffer[i] != ' ' && buffer[i] != '[' && buffer[i] != ']'
                        if (buffer[i] == ',') {
                            count++;
                            if (count == 78 || count == 79 || count == 80) {
                                cout << endl << val << endl;
                            }
                            try { 
                                number = std::stof(val);
                                file.write(reinterpret_cast<const char*>(&number), sizeof(float));
                                // Dimensions.push_back(number);

                            }
                            catch (const std::invalid_argument& e) {
                                std::cerr << "Error: Invalid argument - the string does not represent a valid float." << std::endl;
                                valid = false;
                                break;
                            }
                            catch (const std::out_of_range& e) {
                                std::cerr << "Error: Out of range - the string represents a float that is out of range." << std::endl;
                                valid = false;
                                break;
                            }
                            catch (...) {
                                std::cerr << "Error: An unknown error occurred during the conversion." << std::endl;
                                valid = false;
                                break;
                            }
                         // write to file instead   textVec->push_back(number);
                            val = "";
                        }
                        else if (buffer[i] != ',') {

                            val += buffer[i];
                        }

                    }
                    else {
                        //   size--;

                    }

                    if (buffer[i] == '[')
                    {
                        BracketCount++;
                    }

                }


            }
            response.append(buffer, dwDownloaded);
            delete[] buffer;
        } while (dwSize > 0);

        int size = text.size();
        cout << "written size: "<< size;
        file.write(reinterpret_cast<const char*>(&size),sizeof(int));
        for (char ch : text) {
            file.write(reinterpret_cast<const char*>(&ch), sizeof(char));
        }
        std::cout << response << std::endl;
        file.close();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);


        auto end = std::chrono::high_resolution_clock::now();

        // Calculate the elapsed time
        std::chrono::duration<double> elapsed = end - start;

        // Output the elapsed time in seconds
        std::cout << "Elapsed time: " << elapsed.count() << " seconds." << std::endl;

        return 1;
    }


private:
    shared_ptr<Directories> Dir;
    shared_ptr<MemoryMap> memMap;
    void FreeRequestHeap() {

        if (hRequest) {
            delete hRequest;
            hRequest = nullptr;
        }

        if (dwSize) {
            delete dwSize;
            dwSize = nullptr;
        }

        if (dwDownloaded) {
            delete dwDownloaded;
            dwDownloaded = nullptr;
        }

        if (bResults) {
            delete bResults;
            bResults = nullptr;
        }
    }

    void CleanUp() {
        if (hSession) {
            WinHttpCloseHandle(hSession);
        }
        if (hConnect) {
            WinHttpCloseHandle(hConnect);
        }
        if (hRequest) {
            WinHttpCloseHandle(hRequest);
        }
        delete hSession;
        delete hConnect;
        delete hRequest;
        delete dwSize;
        delete dwDownloaded;
        delete bResults;
    }

    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;
    DWORD* dwSize;
    DWORD* dwDownloaded;
    BOOL* bResults;
    const wchar_t* Url;
    int Port = 11434;
    const wchar_t* headers = L"Content-Type: application/json\r\nConnection: Keep-Alive\r\n";
};



class PDFReader {
public:

    PDFReader(shared_ptr<Directories> &dir) : httpHandler(nullptr), Dir(nullptr) {
        Dir = dir;
        int status;
        cout << "Here";
        httpHandler = new HttpHandler(status, Dir);

        string executableDirectory = Dir->GetExeDir();
        std::wstring wstr(executableDirectory.begin(), executableDirectory.end());
        wstr += L"\\include\\readerdll.dll";

        std::wcout << L"Loading DLL from: " << wstr << std::endl;

        // Load the DLL using the wide-character version
        hModule = LoadLibraryW(wstr.c_str());
        if (hModule == NULL) {
            std::cerr << "Failed to load DLL!" << std::endl;
            return;
        }
    }
    /*
    std::string read_pdf_page_to_utf8() {
        // Open the PDF document
        std::unique_ptr<poppler::document> doc(poppler::document::load_from_file("C:\\vsCode\\HSWN Vector search\\VectorSearchStorageEngine\\tests\\ITCTA1-44 Week 3.pdf"));
        if (!doc) {
            throw std::runtime_error("Error loading PDF document.");
            return "";
        }
        const int pagesNbr = doc->pages();

        poppler::ustring text_ustring;

        string characters;

        for (int i = 1; i < 3; i++) {
            text_ustring = doc->create_page(i)->text();
            characters = text_ustring.to_latin1().c_str();

            for (int i = 0; i < characters.size() - 1; i++) {
                if (characters[i] < 128 && characters[i] != 34) {
                    std::cout << characters[i];
                }

            }
        }
        return "";
    }
*/
    string readEduvosITCTAStructuredPDF(const char* file_path) {
        const wchar_t* dllPath = L"C:\\vsCode\\Rust\\PDFReader\\readerdll\\target\\release\\readerdll.dll";
        HMODULE hModule = LoadLibrary(dllPath);
        if (hModule == NULL) {
            std::cerr << "Failed to load DLL!" << std::endl;
            return "1";
        }

        auto stream_pdf_lines_func = (bool (*)(const char*, uint8_t*, size_t))GetProcAddress(hModule, "stream_pdf_lines");
        if (!stream_pdf_lines_func) {
            std::cerr << "Failed to find function!" << std::endl;
            FreeLibrary(hModule);
            return "1";
        }
        // const char* file_path = "C:\\vsCode\\HSWN Vector search\\VectorSearchStorageEngine\\tests\\ITCTA1-44 Week 3.pdf";
        size_t shared_mem_size = 1024 * 1024;
        uint8_t* shared_mem_ptr = new uint8_t[shared_mem_size];
        string file = "";
        bool success = stream_pdf_lines_func(file_path, shared_mem_ptr, shared_mem_size);
        bool scan = false;
        bool colonFound = false;
        bool record = false;
        string Lesson;
        string LessonTopic;
        int count = 0;

        if (success) {
            for (size_t i = 0; i < shared_mem_size; ++i) {
                unsigned char ch = shared_mem_ptr[i];

                if (ch == 205) {
                    break;
                }
                if (ch < 127) {
                    file += ch;
                }

                // Detect "Lesson"
                if (!scan && (ch == 'L' || ch == 'l')) {
                    Lesson = ch;
                    scan = true;
                    count = 1;
                }
                else if (scan) {
                    if (count == 1 && (ch == 'e' || ch == 'E')) {
                        Lesson += ch;
                        count++;
                    }
                    else if ((count == 2 || count == 3) && (ch == 's' || ch == 'S')) {
                        Lesson += ch;
                        count++;
                    }
                    else if (count == 4 && (ch == 'o' || ch == 'O')) {
                        Lesson += ch;
                        count++;
                    }
                    else if (count == 5 && (ch == 'n' || ch == 'N')) {
                        Lesson += ch;
                        count++;
                        // record = true;
                        // scan = false;
                    }
                    else if (count == 6) {
                        if (ch >= '0' && ch <= '9') {
                            Lesson += " ";
                            Lesson += ch;
                            record = true;
                            scan = false;
                            cout << "Lesson: " << Lesson << " ";
                        }
                        count++;
                    }
                    else if (count == 7) {
                        if (ch >= '0' && ch <= '9') {
                            Lesson += " ";
                            Lesson += ch;
                            record = true;
                            scan = false;
                            cout << "Lesson: " << Lesson << " ";
                        }
                        count++;
                    }
                    else {
                        Lesson.clear();
                        scan = false;

                        count = 0;
                    }
                }

                // Accumulate topic after finding "Lesson:"
                if (record) {
                    if (ch == ':') {
                        colonFound = true;
                        LessonTopic.clear(); // Clear previous topic content
                    }
                    else if (colonFound && ch != '\n') {
                        LessonTopic += ch;
                    }
                    else if (colonFound && ch == '\n') {
                        // End of Lesson topic line
                        cout << "Topic: " << LessonTopic << endl;
                        record = false;
                        colonFound = false;
                        LessonTopic.clear();
                    }
                }
            }
        }
        else {
            std::cerr << "Failed to extract PDF text." << std::endl;
        }
        /*  while ((pos = text.find(substring, pos)) != std::string::npos) {
              ++count;
              pos += substring.length(); positions.push_back(pos); // Store the position after the end of the substring }
          }*/
        cout << file;
        delete[] shared_mem_ptr;
        FreeLibrary(hModule);
        return file;
    }


    string readPDFPages(string file_path, string model, int blockSize, int overlap = 0) {
        // const wchar_t* dllPath = Dir.GetReader();
      //  cout << "file:" << Dir->GetFileBin() << endl << endl;
      //  return "f";
       


        auto stream_pdf_lines_func = (bool (*)(const char*, uint8_t*, size_t))GetProcAddress(hModule, "stream_pdf_lines");
        if (!stream_pdf_lines_func) {
            std::cerr << "Failed to find function!" << std::endl;
            FreeLibrary(hModule);
            return "1";
        }
        // const char* file_path = "C:\\vsCode\\HSWN Vector search\\VectorSearchStorageEngine\\tests\\ITCTA1-44 Week 3.pdf";
        size_t shared_mem_size = 1024 * 1024;
        uint8_t* shared_mem_ptr = new uint8_t[shared_mem_size];
        string block = "";
        bool success = stream_pdf_lines_func(file_path.c_str(), shared_mem_ptr, shared_mem_size);
        int count = 0;
        int testCount = 0;
      //  string filename = GetFileName(file_path);
        if (success) {
            for (size_t i = 0; i < shared_mem_size; ++i) {
                unsigned char ch = shared_mem_ptr[i];

                if (ch == 205) {
                    break;
                }
                if (ch < 127) {
                    block += ch;
                    count++;
                }
                //    if (ch == '')
                if (count >= blockSize) {
                    if (ch == ' ') {
                        httpHandler->GetBlockEmbeddings(block, model, blockSize, overlap);
                      //  GetEmbeddingsFromText(block, filename, collectionName, model);
                        count = 0;
                        testCount++;
                        block = "";
                    }

                }


            }
            if (block != "") {
              //  GetEmbeddingsFromText(block, filename, collectionName, model);
            }
        }
        else {
            std::cerr << "Failed to extract PDF text." << std::endl;
        }
        /*  while ((pos = text.find(substring, pos)) != std::string::npos) {
              ++count;
              pos += substring.length(); positions.push_back(pos); // Store the position after the end of the substring }
          }*/
        delete[] shared_mem_ptr;
        FreeLibrary(hModule);
        return "1";
    }
private:
    HMODULE hModule;
    shared_ptr<Directories> Dir;
    HttpHandler* httpHandler;
};






class OllamaAPI {
public:

    void CreateHNSW() {
        string message ="";
        memMap = make_shared<MemoryMap>(modelVecSize,Dir,message);
        cout << message;
        bool result = memMap->MapFile(message);
        if (result == false) {
            cout << "Error ";
        }
        cout << message;
        memMap->CreateHNSW(message);
        cout << message;
        memMap->GetHNSW(hnsw);
        hnsw->DisplayNeighbors();

    }

    void QueryHNSW(string text) {
        float* vec;
        vec = new float[modelVecSize];
        httpHandler->GetQueryEmbeddings(text, *model ,vec);
        //hnsw->DisplayNeighbors();
        vector<string> Answer = hnsw->searchKNN(vec, 1);
        if (Answer.size() == 0) {
            cout << "Size is 0" << endl;
        }
        for (string ans : Answer) {
            cout << "asnwer: " << ans << endl;
        }
       
        cout << endl;
        delete[] vec;
    }
    ///////////make classes shared pointers
    OllamaAPI(std::string& message, const wchar_t* Url = L"127.0.0.1", int Port = 11434) : pdfReader(nullptr), url(nullptr), port(nullptr), model(nullptr), memMap(nullptr), httpHandler(nullptr), modelVecSize(0), hnsw(nullptr) {
       // url = new wchar_t[256]; // Assuming maximum URL length wcscpy_s(url, 256
        url = new const wchar_t*[256];       // Allocate memory for 'url' based on 'urlLength'
        *url = Url;
        port = new int(Port);
        model = new string;
        Dir = make_shared<Directories>();
        *model = "";
        //model = 
        const char* command= "ollama > NUL 2>&1";
        int result = system(command);
        if (result != 0) { 
            Cleanup();
            message = "you do not have ollama on your system";
            return;
        }
        int status = 0;
        httpHandler = make_shared<HttpHandler>(status, Dir ,Url,Port);
        if (status == 0) {
            message = "Success";
            return;
        }
        else if (status == 1) {
            message = "Error: Unable to open WinHTTP session.";
        }
        else if (status == 2) {
            message = "Error: Unable to connect to server.";
        }
        httpHandler->~HttpHandler();
        
        return;
    }

    ~OllamaAPI() {
        Cleanup();
    }

    void DownloadOllama() {
        return;
    }





    int DownloadModel(const string& model) {
        std::string command = "ollama show " + model;
        std::vector<std::string> outputLines;

        // Open a pipe to run the command and read its output
        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Failed to run command." << std::endl;
            return -1;
        }

        // Read each line from the pipe and store it in the outputLines vector
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            outputLines.emplace_back(buffer);
        }

        // Close the pipe
        int pipeResult = _pclose(pipe);

        // Call Dir.SetModel(model) or any other processing with outputLines
        //Dir.SetModel(model);
        int count = 0;
        string target = "embedding";
        // Debug output
        for (const auto& line : outputLines) {
            for (char ch : line) {
                // if ()
            }
            std::cout << "line :" << line;
        }
        return pipeResult;
    }

    void PullModel(const std::string& Model, std::string& message) {
        auto start = std::chrono::high_resolution_clock::now();
        std::string command = "start cmd /c \"ollama pull " + Model + " && echo Successfully pulled " + Model + "\"";

        // Execute the command in a new console window
        int result = system(command.c_str());

        // Check if the command succeeded
        if (result == 0) {
            message = "Command executed successfully in a new console window.";
        }
        else {
            message = "Failed to execute command in a new console window.";
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        std::cout << "Elapsed time: " << elapsed.count() << " seconds." << std::endl;
        int i;
        cin >> i;
    }
    /*
    int createDirectory(string name) {
        if (std::filesystem::create_directory(name)) {
            return 1;
        }
        else {
            return -1;
        }

    }
    */

    void ListModels() {
        vector<string> models;
        Dir->ListDirectories(models);

        for (string str : models) {
            cout << str << endl;
        }
    }

    void ListCollections() {
        vector<string> collections;
        Dir->ListCollections(collections);

        for (string str : collections) {
            cout << str << endl;
        }
    }

    void ListFiles() {
        vector<string> files;
        int result = Dir->ListFiles(files);
        for (string str : files) {
            cout << str << endl;
        }
    }

    int SetModel(const std::string& Model) {

        int result = Dir->SetModelDir(Model);
        if (result == -2) {
            return -2;
        }
        else if (result == -1) {
            return -1;
        }
        else {
            *model = Model;
            modelVecSize = result;
            return 1;
        }



        // Dir.SetModel(model);
        // return result;
    }

    void SetCollection(string Collection) {
        Dir->SetCollectionDir(Collection);
        string dir = Dir->GetCurrentDir();
        string message;
        memMap = make_shared<MemoryMap>(modelVecSize, Dir, message);
        cout << message;


    }

    void CreateCollection(string Collection) {
        Dir->CreateCollection(Collection);
    }

    void SetFile(string FilePath) {
        Dir->SetCurrentFile(FilePath);
    }

    void CreateBlockEmbeddings(string model, string CollectionName, string fileName, int blockSize, int overlap = 0) {
        int fileSize = fileName.size();
        string end = "";
        std::size_t size = fileName.size();
        if (size >= 3) { 
            end = fileName.substr(size - 3);
        }
      //  if (end == "pdf") {
      //      pdfReader.readPDFPages()
      //  }


    }




    void AddEmbeddings(string FilePath) {

    }

    void NewEmbeddings(string FilePath) {
        int fileSize = FilePath.size();
        string end = "";
        std::size_t size = FilePath.size();
        if (size >= 3) {
            end = FilePath.substr(size - 3);
        }
        Dir->AddFile(FilePath);
        cout << Dir->GetFileBin();
        cout << modelVecSize <<endl;
        pdfReader = make_shared<PDFReader>(Dir); 
        pdfReader->readPDFPages(FilePath, *model, 2000);

    }

private: 
    shared_ptr<PDFReader> pdfReader;
    shared_ptr<Directories> Dir;
    shared_ptr<HttpHandler> httpHandler;
    shared_ptr<MemoryMap> memMap;
    shared_ptr<HNSW> hnsw;
    

    void Cleanup() {
        delete model;
        delete port;
        delete[] url;
        httpHandler->~HttpHandler();
    }

    string* model;
    int* port;
    int modelVecSize;
    const wchar_t** url;
   // const wchar_t** endpoint;
    vector<string> values = {};
    vector<vector<string>> listValues = {};
};

int GetEmbeddingsFromText(string text, string filename, string collectionName, string model, bool shared = false) {
    auto start = std::chrono::high_resolution_clock::now();
    string textStr = string(text);
    textStr = normalizeForJson(textStr);
    std::string modifiedString = textStr.substr(319);
    /*   size_t pos = 0;
        while ((pos = textStr.find("\n", pos)) != std::string::npos) {
                textStr.replace(pos, 1, "\\n");
                pos += 2;  // Move past the replaced escape sequence
        }*/
    const wchar_t* url = L"127.0.0.1";
    const int port = 11434;
    const wchar_t* endpoint = L"/api/embed";


    std::string json = "{\"model\": \"" + model + "\", \"input\": \"" + modifiedString + "\"}";
    cout << json;
    HINTERNET hSession = WinHttpOpen(L"User - Agent: WinHTTP Example / 1.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        std::cerr << "Error: Unable to open WinHTTP session." << std::endl;
        return 1;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, url, port, 0);
    if (!hConnect) {
        std::cerr << "Error: Unable to connect to server." << std::endl;
        WinHttpCloseHandle(hSession);
        return 1;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", endpoint,
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_BYPASS_PROXY_CACHE);
    if (!hRequest) {
        std::cerr << "Error: Unable to open HTTP request." << std::endl;
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    // Set headers for JSON content type in UTF-8
    const wchar_t* headers = L"Content-Type: application/json; charset=utf-8";

    // Send the request with the JSON data as a UTF-8 string
    BOOL bResults = WinHttpSendRequest(hRequest, headers, -1L,
        (LPVOID)json.c_str(), json.length(),
        json.length(), 0);
    if (!bResults) {
        std::cerr << "Error: Unable to send HTTP request." << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    // Receive the response
    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults) {
        std::cerr << "Error: Unable to receive HTTP response." << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    // Read the response data
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    std::string response;
    bool Numbers = false;

    //file.seekp(-static_cast<int>(sizeof(int)), std::ios::end);
    //searchKey;
    int BracketCount = 0;
    bool added = false;
    bool valid = true;
    int count = 0;
    std::string val = "";
    Directories Dir;

    Dir.CreateCollection(collectionName);
    Dir.SetCollectionDir(collectionName);
    Dir.SetCurrentFile(filename + ".bin");
    //string filePath = path + "\\" + filename + ".bin";
   // cout << endl << filePath << endl;
    ofstream openFile(Dir.GetCurrentDir(), ios::app);
    openFile.close();

    std::fstream file(Dir.GetCurrentDir(), std::ios::app | std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file." << std::endl;

    }
    int size = text.size();
    int key = 1;
    int endKey = 111;
    float number;
    do {
        // Check for available data
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
            std::cerr << "Error: QueryDataAvailable failed." << std::endl;
            break;
        }
        //////////////////////use streaming instead maybe 
        // Allocate space for the buffer
        char* buffer = new char[dwSize + 1];
        if (!buffer) {
            std::cerr << "Error: Memory allocation failed." << std::endl;
            break;
        }

        // Read the data
        ZeroMemory(buffer, dwSize + 1);
        if (!WinHttpReadData(hRequest, (LPVOID)buffer, dwSize, &dwDownloaded)) {
            std::cerr << "Error: ReadData failed." << std::endl;
            delete[] buffer;
            break;
        }


        if (dwDownloaded > 0) {


            for (int i = 0; i < dwDownloaded; i++) {
                // file << buffer[i];
                if (buffer[i] == ']')
                {
                    if (val != "") {
                        number = std::stof(val);
                        file.write(reinterpret_cast<const char*>(&number), sizeof(float));
                        val = "";
                    }

                    BracketCount = 0;
                }

                if (BracketCount == 2) {      //&& buffer[i] != ' ' && buffer[i] != '[' && buffer[i] != ']'
                    if (buffer[i] == ',') {
                        count++;
                        if (count == 78 || count == 79 || count == 80) {
                            cout << endl << val << endl;
                        }
                        // Num = std::stof(val);
                         //add error catcher for stof for prod
                        try { // Attempt to convert the string to a float 
                            // response += (val + ",");
                            number = std::stof(val);
                           // Dimensions.push_back(number);
                            
                            file.write(reinterpret_cast<const char*>(&number), sizeof(float));
                        }
                        catch (const std::invalid_argument& e) {
                            std::cerr << "Error: Invalid argument - the string does not represent a valid float." << std::endl;
                            valid = false;
                            break;
                        }
                        catch (const std::out_of_range& e) {
                            std::cerr << "Error: Out of range - the string represents a float that is out of range." << std::endl;
                            valid = false;
                            break;
                        }
                        catch (...) {
                            std::cerr << "Error: An unknown error occurred during the conversion." << std::endl;
                            valid = false;
                            break;
                        }
                        // cout << val;
                        val = "";

                        //file.write(reinterpret_cast<const char*>(&Num), sizeof(float));

                        val = "";
                    }
                    else if (buffer[i] != ',') {

                        val += buffer[i];
                    }

                }
                else {
                    //   size--;

                }

                if (buffer[i] == '[')
                {
                    BracketCount++;
                }

            }


        }
        response.append(buffer, dwDownloaded);
        delete[] buffer;
    } while (dwSize > 0);

    file.write(reinterpret_cast<const char*>(&size), sizeof(int));
    file.write(text.data(), size);

    std::cout << response << std::endl;
   // std::cout << "vec size: " << Dimensions.size() << endl;
    //CreateCollection(model,collec);
  //  ofstream refFile("vectors.txt", ios::app);
  //  refFile << response << endl;
   // refFile.close();
    cout << filename << endl;

    // std::streamoff offset = -static_cast<std::streamoff>(sizeof(int));
 /*   if (valid == true) {
        //  file.seekp(0, ios::end);  // Seek to the end to find the last integer

          // Write the new key at the last position (endKey) in the file
       //   file.seekp(offset, ios::cur);  // Move the pointer back by the size of an integer
        file.write(reinterpret_cast<const char*>(&key), sizeof(int));
        file.write(reinterpret_cast<const char*>(&size), sizeof(int));
        for (float val : Dimensions) {
            file.write(reinterpret_cast<const char*>(&val), sizeof(float));
        }
        file.write(reinterpret_cast<const char*>(&endKey), sizeof(int));
        cout << endl << "Wrote to file" << endl;
    }
    else {
        cout << endl << "Did not write to file because received content is invalid" << endl;
    }

*/

    file.close();
    // Output the response
    //std::cout << "Response: " << response << std::endl;

    // Clean up
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);


    auto end = std::chrono::high_resolution_clock::now();

    // Calculate the elapsed time
    std::chrono::duration<double> elapsed = end - start;

    // Output the elapsed time in seconds
    std::cout << "Elapsed time: " << elapsed.count() << " seconds." << std::endl;

    return 1;
}







//#define BT_PORT 5 
// RFCOMM channel (you can choose any channel from 1 to 30)




// Get the specified page
// std::unique_ptr<poppler::page> page(doc->create_page(page_number));
// if (!page) {
//     throw std::runtime_error("Error creating PDF page.");
//  }
  // poppler::ustring& ustr =;
   // Extract text from the page and convert it to UTF-8 std::string
    //poppler::byte_array byte_array;
    //char ch;
    //std::string text = convert_to_utf8(page->text());










int MemoryMapFunc() {
    //C:\Users\dawid\source\repos\DatabaseSys\x64\Debug\models\nomic-embed-text\ITCTA
    auto start = std::chrono::high_resolution_clock::now();
    const char* filePath = "C:\\Users\\dawid\\source\\repos\\DatabaseSys\\x64\\Debug\\models\\nomic-embed-text\\ITAIA\\ITAIA1-B33_Week 7_Slides 1.bin";

    HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        std::cerr << "Error getting file size." << std::endl;
        CloseHandle(hFile);
        return 1;
    }

    // Create a file mapping object
    HANDLE hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapping == NULL) {
        std::cerr << "Error creating file mapping." << std::endl;
        CloseHandle(hFile);
        return 1;
    }

    LPVOID pMap = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (pMap == NULL) {
        std::cerr << "Error mapping file into memory." << std::endl;
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 1;
    }
    int key = 0;
    int size = 768;
    int count = 0;

    int textSize = 0;
   //
    cout << "file size:" << fileSize << endl;
    while (count < fileSize) {

        float* floatStart = reinterpret_cast<float*>(reinterpret_cast<char*>(pMap) + count);
        for (size_t i = 0; i < size; i++) {
            //count = count  + sizeof(float)
            count = count + sizeof(float);
            std::cout << floatStart[i] << " ";
        }
        cout << endl;
        textSize = *reinterpret_cast<int*>(reinterpret_cast<char*>(pMap) + count);
        cout << "text size: " << textSize << endl;
        count = count + sizeof(int);
        char* textstart = reinterpret_cast<char*>(reinterpret_cast<char*>(pMap) + count);
        for (size_t i = 0; i < textSize; i++) {
            count = count + sizeof(char);
            cout << textstart[i];

        }
      //  count = count + textSize * sizeof(char);

    }

 //   do {
        // Read the key and size
      //  key = *reinterpret_cast<int*>(pMap);
     //   size = *reinterpret_cast<int*>(reinterpret_cast<char*>(pMap) + sizeof(int));
     //   std::cout << "Key: " << key << std::endl;
     //   std::cout << "Size (in bytes): " << size << std::endl;

        // Calculate the starting position for floats, skipping the first two integers

    //    int count = 0;
        // Loop through each float value


   // } while (key != 111);


  //  for (SkipList* skipList : Embeddings) {
  //      std::cout << skipList->Dimension;
  //  }

    //delete[] Addresses;         


    auto end = std::chrono::high_resolution_clock::now();

    // Calculate the elapsed time
    std::chrono::duration<double> elapsed = end - start;

    // Output the elapsed time in seconds
    std::cout << "Elapsed time: " << elapsed.count() << " seconds." << std::endl;
    return 1;

}

// Loop through the mapped file and store addresses of '!' characters
/* for (DWORD i = 0; i < 1; ++i) {
      void* address = reinterpret_cast<char*>(pMap) + i;  // Get the address of each byte
      std::cout << " Content: " << *byteAddress << std::hex <<  "Address: " << static_cast<void*>(byteAddress) << std::endl;
      if (*byteAddress == '|') {
          std::cout << "line" << std::endl;
      }
      if (*byteAddress == '!') {
          Addresses[count] = byteAddress;
          ++count;
      }

      std::cout << result;
  }

  // Output the addresses of all '!' characters found
  std::cout << "\nAddresses of '!' characters:\n";
  for (DWORD i = 0; i < count; ++i) {
      std::cout << "Address: " << static_cast<void*>(Addresses[i]) << std::endl;
   }*/





void AddDocument(string& model, string& Name) {


}



int EmbedDocumentContents(){
    return 1;
}

//L"C:\\vsCode\\Rust\\PDFReader\\readerdll\\target\\release\\readerdll.dll"




int readVal() {
    std::vector<float> values;
    std::ifstream file("vectors.txt", std::ios::binary); // Open file in binary mode

    if (!file) {
        std::cerr << "Failed to open file.\n";
        return 1;
    }

    float num;
    bool Index = false;
    
    while (file.read(reinterpret_cast<char*>(&num), sizeof(float))) {
      //  values.push_back(num); // Add each float to the vector
        if (num == 1) {
            Index = true;
        }

        std::cout << num << std::endl;
    }

    file.close(); // Close the file after reading

    // Optional: Display values
 //   for (const auto& value : values) {
 //       std::cout << value << " ";
  //  }
    std::cout << std::endl;

    return 0;
}

int main() {

    auto start = std::chrono::high_resolution_clock::now();
    //768
   // MemoryMapFunc();
   // return 1;
    string model = "nomic-embed-text";
    string message;
    OllamaAPI api(message);
    cout << message << endl;

    api.ListModels();

    int response = api.SetModel(model);
    
    
    if (response == -2) {
        cout << endl << "failure file does not exist. Download model." << endl;

    }
    else if (response == -1) {
        cout << endl << "file is not 8 bytes " << endl;

    }
    else {
        cout << "success" << endl;   
    }
 //   api.CreateCollection("ITAIA"); ///also sets the collection
    api.ListCollections();
//C:\\Users\\dawid\\source\\repos\\DatabaseSys\\x64\\Debug\\models\\nomic - embed - text\\ITAIA\\ITAIA1-B33_Week 7_Slides 1.bin
//    api.SetFile("C:\\vsCode\\HSWN Vector search\\ITAIA1-B33_Week 7_Slides 1.bin");
 //   api.CreateHNSW();
 //   cout << "enter text to exit:";
    string input;
 //   getline(cin, input);
  //  return 1;
  //  api.CreateBlockEmbeddings();
    api.SetCollection("ITCTA");

    
  //  api.NewEmbeddings("C:\\Eduvos\\2024\\ITCTA\\ITCTA1-44 Week 7.pdf");

    cout << "files : " << endl;
    api.ListFiles();
    api.CreateHNSW();
    while (true) {
        cout << "enter query:";
        getline(cin, input);
        api.QueryHNSW(input);
    }


    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return 1;
    //C:\\vsCode\\Rust\\PDFReader\\ITAIA1-B33_Week 7_Slides 1.pdf

}
