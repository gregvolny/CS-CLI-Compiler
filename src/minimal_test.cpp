#include <iostream>
#include <windows.h>

#define WIN_DESKTOP
#include <afx.h>

int main() {
    std::cerr << "Starting minimal test..." << std::endl;
    
    try {
        std::cerr << "About to initialize MFC..." << std::endl;
        if (!AfxGetApp()) {
            AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0);
        }
        std::cerr << "MFC initialized successfully!" << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown exception!" << std::endl;
        return 1;
    }
    
    std::cerr << "Test completed successfully!" << std::endl;
    return 0;
}
