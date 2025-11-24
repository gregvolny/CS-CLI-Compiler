/*
 * CSProCompile - Command-line CSPro Application Compiler
 * 
 * This tool compiles CSPro applications from the command line,
 * enabling integration with VS Code and other text editors.
 * 
 * Based on CSPro source code architecture:
 * - Uses CRunAplBatch/CBatchIFaz for compilation
 * - Mimics CSBatch.exe compilation behavior
 * - Provides JSON output for editor integration
 * 
 * Usage:
 *   CSProCompile <application.ent|.bch> [options] /* Has been changed to use entry compilation instead of Batch */
 *   CSProCompile <application.ent> [options]
 * 
 * Options:
 *   -o <file>     Output compilation results to JSON file
 *   -v            Verbose mode
 *   --check-only  Only check syntax, don't generate binaries
 *   --json        Output errors in JSON format (for VS Code)
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>
#include "../include/CompilerInterface.h"

// For compatibility with legacy code
namespace CSPro {
    using CompilationError = CSProCompiler::DiagnosticMessage;
    using CompilationResult = CSProCompiler::CompilationResult;
}

class CSProCommandLineCompiler {
private:
    std::string inputFile;
    std::string outputFile;
    bool verboseMode;
    bool checkOnly;
    bool jsonOutput;
    std::vector<CSPro::CompilationError> errors;

public:
    CSProCommandLineCompiler() : verboseMode(false), checkOnly(false), jsonOutput(false) {}

    void setInputFile(const std::string& file) { inputFile = file; }
    void setOutputFile(const std::string& file) { outputFile = file; }
    void setVerboseMode(bool mode) { verboseMode = mode; }
    void setCheckOnly(bool mode) { checkOnly = mode; }
    void setJsonOutput(bool mode) { jsonOutput = mode; }

    bool validateInputFile() {
        if (!std::filesystem::exists(inputFile)) {
            std::cerr << "Error: Input file not found: " << inputFile << std::endl;
            return false;
        }

        std::string ext = std::filesystem::path(inputFile).extension().string();
        if (ext != ".ent" && ext != ".bch" && ext != ".pff") {
            std::cerr << "Error: Invalid file type. Expected .ent, .bch, or .pff" << std::endl;
            return false;
        }

        return true;
    }

    CSPro::CompilationResult compile() {
        if (verboseMode) {
            std::cout << "Compiling: " << inputFile << std::endl;
            if (checkOnly) {
                std::cout << "Mode: Syntax check only" << std::endl;
            }
        }

        // Use real CSPro compiler engine
        auto engine = CSProCompiler::createCompilerEngine();
        
        if (!engine->initialize()) {
            CSPro::CompilationResult result;
            result.success = false;
            result.compilationTimeMs = 0.0;
            CSProCompiler::DiagnosticMessage msg;
            msg.severity = CSProCompiler::DiagnosticMessage::Severity::Error;
            msg.message = "Failed to initialize CSPro compiler";
            result.diagnostics.push_back(msg);
            result.errorCount = 1;
            return result;
        }

        // Set compilation options
        CSProCompiler::CompilerOptions options;
        options.inputFile = inputFile;
        options.verboseOutput = verboseMode;
        options.checkSyntaxOnly = checkOnly;
        
        // Compile
        auto result = engine->compile(options);
        
        // Shutdown engine
        engine->shutdown();
        
        // Save errors to compileErrors.txt in the same folder as the .ent file
        if (!result.diagnostics.empty()) {
            std::filesystem::path entPath(inputFile);
            std::filesystem::path errorFilePath = entPath.parent_path() / "compileErrors.txt";
            std::filesystem::path formattedFilePath = entPath.parent_path() / "compileErrorsFormatted.txt";
            
            // Save detailed format (original)
            std::ofstream errorFile(errorFilePath);
            if (errorFile.is_open()) {
                errorFile << "CSPro Compilation Errors/Warnings\n";
                errorFile << "==================================\n";
                errorFile << "File: " << inputFile << "\n";
                errorFile << "Date: " << __DATE__ << " " << __TIME__ << "\n";
                errorFile << "Total Errors: " << result.errorCount << "\n";
                errorFile << "Total Warnings: " << result.warningCount << "\n";
                errorFile << "\n";
                
                for (const auto& diag : result.diagnostics) {
                    std::string severity = (diag.severity == CSProCompiler::DiagnosticMessage::Severity::Error) ? "ERROR" : "WARNING";
                    errorFile << severity << " at line " << diag.line << ", column " << diag.column << ":\n";
                    errorFile << "  " << diag.message << "\n";
                    errorFile << "  Location: " << diag.file << "\n";
                    errorFile << "\n";
                }
                
                errorFile.close();
                
                if (verboseMode) {
                    std::cout << "Errors/warnings saved to: " << errorFilePath.string() << std::endl;
                }
            }
            
            // Save Designer-compatible format
            std::ofstream formattedFile(formattedFilePath);
            if (formattedFile.is_open()) {
                for (const auto& diag : result.diagnostics) {
                    std::string severity = (diag.severity == CSProCompiler::DiagnosticMessage::Severity::Error) ? "ERROR" : "WARNING";
                    
                    // Format like CSPro Designer: SEVERITY(ProcName, line): message
                    if (!diag.procName.empty() && diag.line > 0) {
                        formattedFile << severity << "(" << diag.procName << ", " << diag.line << "): " << diag.message << "\n";
                    } else if (!diag.procName.empty()) {
                        formattedFile << severity << "(" << diag.procName << "): " << diag.message << "\n";
                    } else if (diag.line > 0) {
                        formattedFile << severity << "(" << diag.line << "): " << diag.message << "\n";
                    } else {
                        formattedFile << severity << ": " << diag.message << "\n";
                    }
                }
                
                formattedFile.close();
                
                if (verboseMode) {
                    std::cout << "Formatted errors saved to: " << formattedFilePath.string() << std::endl;
                }
            }
        }
        
        return result;
    }

public:
    void outputResults(const CSPro::CompilationResult& result) {
        if (jsonOutput) {
            outputJson(result);
        } else {
            outputText(result);
        }
    }

private:
    void outputJson(const CSPro::CompilationResult& result) {
        std::ostream* out = &std::cout;
        std::ofstream file;

        if (!outputFile.empty()) {
            file.open(outputFile);
            out = &file;
        }

        *out << "{\n";
        *out << "  \"success\": " << (result.success ? "true" : "false") << ",\n";
        *out << "  \"compilationTime\": " << result.compilationTimeMs / 1000.0 << ",\n";
        *out << "  \"errors\": [\n";

        for (size_t i = 0; i < result.diagnostics.size(); i++) {
            const auto& diag = result.diagnostics[i];
            std::string severity = (diag.severity == CSProCompiler::DiagnosticMessage::Severity::Error) ? "error" : "warning";
            *out << "    {\n";
            *out << "      \"file\": \"" << diag.file << "\",\n";
            *out << "      \"line\": " << diag.line << ",\n";
            *out << "      \"column\": " << diag.column << ",\n";
            *out << "      \"message\": \"" << diag.message << "\",\n";
            *out << "      \"severity\": \"" << severity << "\"\n";
            *out << "    }";
            if (i < result.diagnostics.size() - 1) *out << ",";
            *out << "\n";
        }

        *out << "  ]\n";
        *out << "}\n";

        if (file.is_open()) {
            file.close();
        }
    }

    void outputText(const CSPro::CompilationResult& result) {
        if (result.success) {
            std::cout << "Compilation successful!" << std::endl;
            if (verboseMode) {
                std::cout << "Compilation time: " << (result.compilationTimeMs / 1000.0) << " seconds" << std::endl;
            }
        } else {
            std::cerr << "Compilation failed with " << result.errorCount << " error(s)";
            if (result.warningCount > 0) {
                std::cerr << " and " << result.warningCount << " warning(s)";
            }
            std::cerr << ":" << std::endl;
            
            for (const auto& diag : result.diagnostics) {
                std::string severity = (diag.severity == CSProCompiler::DiagnosticMessage::Severity::Error) ? "error" : "warning";
                std::cerr << diag.file << "(" << diag.line << "," << diag.column << "): ";
                std::cerr << severity << ": " << diag.message << std::endl;
            }
        }
    }
};

void printUsage(const char* programName) {
    std::cout << "CSProCompile - Command-line CSPro Application Compiler\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << programName << " <application.ent|.bch|.pff> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <file>     Output compilation results to JSON file\n";
    std::cout << "  -v            Verbose mode\n";
    std::cout << "  --check-only  Only check syntax, don't generate binaries\n";
    std::cout << "  --json        Output errors in JSON format (for VS Code)\n";
    std::cout << "  -h, --help    Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " myapp.ent\n";
    std::cout << "  " << programName << " myapp.bch -v --json\n";
    std::cout << "  " << programName << " myapp.pff -o results.json\n";
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            printUsage(argv[0]);
            return 1;
        }

        CSProCommandLineCompiler compiler;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "-v") {
            compiler.setVerboseMode(true);
        }
        else if (arg == "--check-only") {
            compiler.setCheckOnly(true);
        }
        else if (arg == "--json") {
            compiler.setJsonOutput(true);
        }
        else if (arg == "-o") {
            if (i + 1 < argc) {
                compiler.setOutputFile(argv[++i]);
            } else {
                std::cerr << "Error: -o requires an output filename\n";
                return 1;
            }
        }
        else if (arg[0] != '-') {
            // Input file
            compiler.setInputFile(arg);
        }
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // Validate and compile
    if (!compiler.validateInputFile()) {
        return 1;
    }

        CSPro::CompilationResult result = compiler.compile();
        compiler.outputResults(result);

        return result.success ? 0 : 1;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Error: Unknown exception occurred" << std::endl;
        return 1;
    }
}
