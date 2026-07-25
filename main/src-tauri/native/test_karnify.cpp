#include "karnotation.h"
#include <iostream>
#include <sstream>
#include <regex>

// Parse solver output format like "13|F' d' u D M u U' U2D' 31  [9]"
// Extract just the algorithm part
struct SolverSolution {
    std::string rawLine;
    std::string rawAlg;
    
    static SolverSolution parse(const std::string &line) {
        SolverSolution sol;
        sol.rawLine = line;
        
        // Find first [ which marks the start of counts
        size_t bracketPos = line.find('[');
        if (bracketPos == std::string::npos) {
            sol.rawAlg = "";
            return sol;
        }
        
        // Get substring before [, trim whitespace
        std::string beforeBracket = line.substr(0, bracketPos);
        
        // Skip the first "13|" or "13\" style prefix
        size_t pipeOrBackslash = beforeBracket.find_first_of("|\\");
        if (pipeOrBackslash != std::string::npos) {
            std::string algPart = beforeBracket.substr(pipeOrBackslash + 1);
            // Trim trailing whitespace
            size_t endPos = algPart.find_last_not_of(" \t");
            if (endPos != std::string::npos) {
                sol.rawAlg = algPart.substr(0, endPos + 1);
            }
        }
        
        return sol;
    }
};

int main() {
    std::string line;
    std::vector<std::string> testLines;
    
    // Read from stdin until "Stop requested" or EOF
    std::cerr << "Paste solver output below (ends with 'Stop requested.' or EOF):" << std::endl;
    std::cerr << "Format: 13|F' d' u D M u U' U2D' 31  [9]" << std::endl << std::endl;
    
    while (std::getline(std::cin, line)) {
        if (line.find("Stop requested") != std::string::npos) {
            break;
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }
        testLines.push_back(line);
    }
    
    if (testLines.empty()) {
        std::cerr << "No valid lines read" << std::endl;
        return 1;
    }
    
    std::cout << "Testing karnify on " << testLines.size() << " solutions...\n" << std::endl;
    
    int passed = 0;
    int failed = 0;
    int errors = 0;
    
    for (size_t i = 0; i < testLines.size(); i++) {
        auto sol = SolverSolution::parse(testLines[i]);
        
        if (sol.rawAlg.empty()) {
            std::cerr << "[SKIP " << (i+1) << "] Could not parse: " << testLines[i] << std::endl;
            continue;
        }
        
        try {
            std::string result = karnify(sol.rawAlg);
            std::cout << "[" << (i+1) << "] " << sol.rawAlg << " → " << result << std::endl;
            passed++;
        } catch (const std::exception &e) {
            std::cerr << "[ERROR " << (i+1) << "] " << sol.rawAlg << ": " << e.what() << std::endl;
            errors++;
        }
    }
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Total: " << testLines.size() << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "Errors: " << errors << std::endl;
    
    if (errors > 0) {
        return 1;
    }
    
    return 0;
}
