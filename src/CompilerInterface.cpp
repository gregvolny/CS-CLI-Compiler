/*
 * CompilerInterface.cpp - Real CSPro Compiler Engine Implementation
 * Using CCompiler (Designer Compiler)
 */

#include "../include/CompilerInterface.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <locale.h>
#include <mbctype.h>

#ifdef CSPRO_SDK_AVAILABLE
// CSPro standard system includes (MFC, Windows, C++17 std lib, string utilities)
#define WIN_DESKTOP
#include <engine/StandardSystemIncludes.h>

// CSPro Designer Compiler API headers
#include <zAppO/Application.h>
#include <zEngineO/FileApplicationLoader.h>
#include <zEngineO/ApplicationBuilder.h>
#include <zToolsO/CSProException.h>
#include <Zsrcmgro/Compiler.h>
#include <Zsrcmgro/DesignerCompilerMessageProcessor.h>
#include <zLogicO/ParserMessage.h>
#include <zMessageO/SystemMessages.h>
#include <zMessageO/MessageManager.h>
#include <zMessageO/MessageFile.h>
#include <Zsrcmgro/SrcCode.h>
#include <engine/Comp.h>
#include <zToolsO/Tools.h>
#include <zEngineO/Versioning.h>
#include <zToolsO/Serializer.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

namespace CSProCompiler {

class CSProEngineImpl : public ICompilerEngine {
private:
    bool m_initialized;
    
#ifdef CSPRO_SDK_AVAILABLE
    std::unique_ptr<Application> m_application;
    std::unique_ptr<CCompiler> m_compiler;
#endif

public:
    CSProEngineImpl() : m_initialized(false) {}
    
    virtual ~CSProEngineImpl() {
#ifdef CSPRO_SDK_AVAILABLE
        m_compiler.reset();
        m_application.reset();
#endif
    }
    
    bool initialize() override {
        setlocale(LC_ALL, "");
        _setmbcp(_MB_CP_LOCALE);

        if (m_initialized) return true;
        
#ifdef CSPRO_SDK_AVAILABLE
        try {
            if (!AfxGetApp()) {
                AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0);
            }
            
            // Enable SystemMessages
            try {
                SystemMessages::LoadMessages(L"", {}, true);
            } catch (...) {
                // Ignore failure to load system messages
            }
            
            m_initialized = true;
            return true;
        }
        catch (...) {
            return false;
        }
#else
        return false;
#endif
    }
    
    void shutdown() override {
#ifdef CSPRO_SDK_AVAILABLE
        m_compiler.reset();
        m_application.reset();
#endif
        m_initialized = false;
    }
    
    CompilationResult compile(const CompilerOptions& options) override {
        CompilationResult result;
        auto startTime = std::chrono::high_resolution_clock::now();
        
        if (!m_initialized) {
            if (!initialize()) {
                result.success = false;
                result.diagnostics.push_back({"", 0, 0, "Failed to initialize CSPro engine", "", DiagnosticMessage::Severity::Error});
                return result;
            }
        }
        
#ifdef CSPRO_SDK_AVAILABLE
        try {
            std::wstring wInputFile(options.inputFile.begin(), options.inputFile.end());
            CString csInputFile(wInputFile.c_str());
            m_application = std::make_unique<Application>();
            
            // Force Logic Version 8.0+ to ensure modern syntax support and full error reporting
            Versioning::SetCompiledLogicVersion(Serializer::GetCurrentVersion());
            
            // Set CWD
            std::filesystem::path originalCwd = std::filesystem::current_path();
            std::filesystem::path appDir(wInputFile);
            if (appDir.is_relative()) {
                appDir = originalCwd / appDir;
            }
            appDir = std::filesystem::canonical(appDir);
            std::filesystem::path appFileName = appDir.filename();
            appDir.remove_filename();
            
            std::filesystem::current_path(appDir);

            std::filesystem::path absoluteAppPath = appDir / appFileName;
            m_application->Open(CString(absoluteAppPath.c_str()), true, true);
            
            // Ensure Application object also has V80 settings
            LogicSettings logicSettings = m_application->GetLogicSettings();
            logicSettings.SetVersion(LogicSettings::Version::V8_0);
            m_application->SetLogicSettings(logicSettings);
            
            std::filesystem::current_path(originalCwd);
            
            BuildApplication(std::make_shared<FileApplicationLoader>(m_application.get()));
            
            CSourceCode* pSourceCode = new CSourceCode(*m_application);
            // Load the source code from the application's logic file
            if (!pSourceCode->Load()) {
                result.diagnostics.push_back({options.inputFile, 0, 0, "Failed to load application source code", "", DiagnosticMessage::Severity::Error});
                result.success = false;
                return result;
            }
            m_application->SetAppSrcCode(pSourceCode);
            
            m_compiler = std::make_unique<CCompiler>(m_application.get());
            m_compiler->SetOptimizeFlowTree(true);
            m_compiler->SetFullCompile(true);
            
            // Do NOT call Init() explicitly.
            
            CCompiler::Result compileResult = m_compiler->FullCompile(pSourceCode);
            
            const std::vector<Logic::ParserMessage>& allMessages = CCompiler::GetCurrentSession()->GetParserMessages();
            
            for (const auto& parserMsg : allMessages) {
                DiagnosticMessage msg;
                msg.file = options.inputFile;
                msg.line = static_cast<int>(parserMsg.line_number);
                msg.column = static_cast<int>(parserMsg.position_in_line);
                
                std::string formatted = parserMsg.what();
                if (formatted == "Logic - Parser Message" && !parserMsg.message_text.empty()) {
                    std::wstring wMsgText = parserMsg.message_text;
                    formatted = std::string(wMsgText.begin(), wMsgText.end());
                }
                msg.message = formatted;
                
                if (!parserMsg.proc_name.empty()) {
                    std::wstring wProcName = parserMsg.proc_name;
                    msg.procName = std::string(wProcName.begin(), wProcName.end());
                } else if (!parserMsg.compilation_unit_name.empty()) {
                    std::wstring wUnitName = parserMsg.compilation_unit_name;
                    msg.procName = std::string(wUnitName.begin(), wUnitName.end());
                }
                
                switch (parserMsg.type) {
                    case Logic::ParserMessage::Type::Error:
                        msg.severity = DiagnosticMessage::Severity::Error;
                        result.errorCount++;
                        break;
                    case Logic::ParserMessage::Type::Warning:
                    case Logic::ParserMessage::Type::DeprecationMajor:
                    case Logic::ParserMessage::Type::DeprecationMinor:
                        msg.severity = DiagnosticMessage::Severity::Warning;
                        result.warningCount++;
                        break;
                }
                
                if (!parserMsg.compilation_unit_name.empty()) {
                    std::wstring wUnitName = parserMsg.compilation_unit_name;
                    msg.file = std::string(wUnitName.begin(), wUnitName.end());
                }

                result.diagnostics.push_back(msg);
            }
            
            if (result.errorCount == 0) {
                result.success = true;
                std::filesystem::path inputPath(options.inputFile);
                std::filesystem::path outputPath = inputPath;
                outputPath.replace_extension(".pen");
                result.compiledOutput = outputPath.string();
            } else {
                result.success = false;
            }
            
        }
        catch (const std::exception& ex) {
            result.diagnostics.push_back({options.inputFile, 0, 0, std::string("Exception: ") + ex.what(), "", DiagnosticMessage::Severity::Error});
            result.success = false;
        }
        catch (...) {
            result.diagnostics.push_back({options.inputFile, 0, 0, "Unknown exception during compilation", "", DiagnosticMessage::Severity::Error});
            result.success = false;
        }
#else
        result.success = false;
#endif
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.compilationTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        
        return result;
    }
};

std::unique_ptr<ICompilerEngine> createCompilerEngine() {
    return std::make_unique<CSProEngineImpl>();
}

} // namespace CSProCompiler


