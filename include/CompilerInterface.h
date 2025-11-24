/*
 * CompilerInterface.h - CSPro Compiler Integration Interface
 * 
 * This header provides the integration layer between the command-line tool
 * and the actual CSPro compilation engine.
 * 
 * To build with real CSPro libraries, you need to:
 * 1. Include CSPro SDK headers (from cspro/CSPro source)
 * 2. Link against: zBatchO.lib, zEngineO.lib, Wcompile.lib, etc.
 * 3. Ensure CSPro runtime dependencies are available
 */

#ifndef CSPRO_COMPILER_INTERFACE_H
#define CSPRO_COMPILER_INTERFACE_H

#include <string>
#include <vector>
#include <memory>

namespace CSProCompiler {

// Compilation error/warning structure
struct DiagnosticMessage {
    std::string file;
    int line;
    int column;
    std::string message;
    std::string procName;  // Procedure name where error occurred
    enum class Severity {
        Error,
        Warning,
        Info
    } severity;

    std::string getSeverityString() const {
        switch (severity) {
            case Severity::Error: return "error";
            case Severity::Warning: return "warning";
            case Severity::Info: return "info";
            default: return "unknown";
        }
    }
};

// Compilation options
struct CompilerOptions {
    std::string inputFile;
    std::string outputDirectory;
    bool checkSyntaxOnly;
    bool verboseOutput;
    bool generateDebugInfo;
    
    CompilerOptions() 
        : checkSyntaxOnly(false)
        , verboseOutput(false)
        , generateDebugInfo(true) 
    {}
};

// Compilation result
struct CompilationResult {
    bool success;
    int errorCount;
    int warningCount;
    std::vector<DiagnosticMessage> diagnostics;
    std::string compiledOutput;
    double compilationTimeMs;

    CompilationResult() 
        : success(false)
        , errorCount(0)
        , warningCount(0)
        , compilationTimeMs(0.0) 
    {}
};

// Main compiler interface class
class ICompilerEngine {
public:
    virtual ~ICompilerEngine() = default;

    // Initialize the compiler with CSPro environment
    virtual bool initialize() = 0;

    // Compile a CSPro application
    virtual CompilationResult compile(const CompilerOptions& options) = 0;

    // Clean up resources
    virtual void shutdown() = 0;
};

// Factory function to create compiler engine
// Implementation depends on whether we're linking to real CSPro libraries
std::unique_ptr<ICompilerEngine> createCompilerEngine();

/*
 * INTEGRATION NOTES FOR LINKING TO CSPRO:
 * 
 * When implementing the real compiler engine, you would:
 * 
 * 1. Include CSPro headers:
 *    #include <ZBRIDGEO/PifFile.h>
 *    #include <zBatchO/RunaplB.h>
 *    #include <engine/BATIFAZ.h>
 *    #include <Wcompile/wcompile.h>
 *    #include <zEngineO/ApplicationBuilder.h>
 * 
 * 2. Create an implementation like:
 *    class CSProEngineImpl : public ICompilerEngine {
 *        CNPifFile* m_pifFile;
 *        CRunAplBatch* m_batchRunner;
 *        // ...
 *    };
 * 
 * 3. In compile() method:
 *    - Load PFF/ENT/BCH file using CNPifFile
 *    - Create CRunAplBatch instance
 *    - Call LoadCompile() to trigger compilation
 *    - Extract errors from CEngineCompFunc::GetParserMessages()
 *    - Convert Logic::ParserMessage to DiagnosticMessage
 * 
 * 4. Link against CSPro libraries:
 *    - zBatchO.lib (batch application support)
 *    - zEngineO.lib (compilation engine)
 *    - Wcompile.lib (compiler interface)
 *    - zBridgeO.lib (PFF file handling)
 *    - Plus all dependencies (zToolsO, zAppO, etc.)
 */

} // namespace CSProCompiler

#endif // CSPRO_COMPILER_INTERFACE_H
