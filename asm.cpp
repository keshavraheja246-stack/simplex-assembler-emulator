/*
TITLE: asm.cpp
AUTHOR: Keshav Raheja
Roll No: 2401CS10

Declaration of Authorship
This test file, is part of the miniproject of CS2206 at the
department of Computer Science and Engg, IIT Patna.
*/

#include <bits/stdc++.h>
using namespace std;

struct OpcodeEntry {
    string hexCode;   // 2-hex-digit opcode (empty for pseudo-ops like "data")
    int operandKind; // 0 = no operand, 1 = value operand, 2 = label/offset operand
};

struct TableEntry {
    string label;
    string mnemonic;
    string operand;
    int operandType;   // 0=none, 1=label-ref, 8=octal, 10=decimal, 16=hex, -1=invalid
    bool hasLabel;
};


map<string, OpcodeEntry> opcodeTable;
map<string, int> symbolTable;   // label → program counter value

string fileName;
string baseName;   // fileName without ".asm" extension
vector<string> source;      // cleaned source lines
vector<TableEntry> tableEntries;
vector<int> pcValues;    // PC for each source line
vector<pair<int, string>> errorLog;
vector<pair<int, string>> machineCode; // (source-index, hex-word)
bool haltPresent = false;

// ---------------------------------------------------------------------------
// Character / String Classification
// ---------------------------------------------------------------------------

bool isLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool isAlphanumeric(char c) {
    return isLetter(c) || isDigit(c);
}

bool isOctalDigit(char c) {
    return c >= '0' && c <= '7';
}

bool isHexDigit(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool isOctalLiteral(const string& s) {
    if (s.size() <= 1 || s[0] != '0') return false;
    for (size_t i = 1; i < s.size(); ++i)
        if (!isOctalDigit(s[i])) return false;
    return true;
}

bool isHexLiteral(const string& s) {
    if (s.size() <= 2 || s[0] != '0' || (s[1] != 'x' && s[1] != 'X')) return false;
    for (size_t i = 2; i < s.size(); ++i)
        if (!isHexDigit(s[i])) return false;
    return true;
}

bool isDecimalLiteral(const string& s) {
    if (s.empty()) return false;
    for (char c : s)
        if (!isDigit(c)) return false;
    return true;
}

// Returns true if `name` is a valid label/identifier
bool isValidIdentifier(const string& name) {
    if (name.empty() || (!isLetter(name[0]) && name[0] != '_')) return false;
    for (char c : name)
        if (!isAlphanumeric(c) && c != '_') return false;
    return true;
}

// Error handling

void logError(int lineNo, const string& type) {
    errorLog.push_back({lineNo,
        "Issue detected at line: " + to_string(lineNo) + ", Error Type: " + type});
}

/**
 * Normalises a raw source line:
 *   - Collapses all runs of spaces/tabs to a single space
 *   - Ensures exactly ": " after every ':'
 *   - Strips comments (';' and everything after)
 */

string cleanLine(const string& raw) {
    string s = raw;

    // Strip trailing whitespace 
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r'))
        s.pop_back();

    string result;
    result.reserve(s.size());

    for (size_t i = 0; i < s.size(); ) {
        char c = s[i];

        if (c == ';') break; // comment — discard rest

        if (c == ':') {
            result += ": ";
            ++i;
            if (i < s.size() && s[i] == ' ') ++i;   // skip one space after ':'
            continue;
        }

        if (c == ' ' || c == '\t') {
            result += ' ';
            while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
            continue;
        }

        result += c;
        ++i;
    }

    // Strip trailing spaces again (may appear after colon normalisation)
    while (!result.empty() && (result.back() == ' ' || result.back() == '\t'))
        result.pop_back();

    return result;
}

/**
 * Classify an operand string.
 * Returns: 0=no operand, 1=label, 8=octal, 10=decimal, 16=hex, -1=invalid
 */
int classifyOperand(string s) {
    if (s.empty()) return 0;
    if (s[0] == '-' || s[0] == '+') s.erase(s.begin());
    if (s.empty()) return -1;

    if (isHexLiteral(s)) return 16;
    if (isOctalLiteral(s)) return 8;
    if (isDecimalLiteral(s)) return 10;
    if (isValidIdentifier(s)) return 1;
    return -1;
}

/**
 * Convert a signed integer to hex string.
 * Uses 2's complement representation for negative values within `bitWidth` bits.
 */
string toHexString(int32_t value, int bitWidth = 24) {
    uint32_t bits = (bitWidth == 32)
        ? static_cast<uint32_t>(value)
        : (value < 0 ? value + (1 << bitWidth) : static_cast<uint32_t>(value));
    ostringstream oss;
    oss << hex << bits;
    return oss.str();
}

/**
 * Left-pads `s` with '0' to reach `width` characters.
 */
string zeroPad(const string& s, int width = 6) {
    return string(max(0, width - (int)s.size()), '0') + s;
}

void initOpcodeTable() {
    // pseudo-ops (no hex code)
    opcodeTable["data"] = {"",   1};
    opcodeTable["SET"]  = {"",   1};

    // real instructions
    opcodeTable["ldc"]    = {"00", 1};
    opcodeTable["adc"]    = {"01", 1};
    opcodeTable["ldl"]    = {"02", 2};
    opcodeTable["stl"]    = {"03", 2};
    opcodeTable["ldnl"]   = {"04", 2};
    opcodeTable["stnl"]   = {"05", 2};
    opcodeTable["add"]    = {"06", 0};
    opcodeTable["sub"]    = {"07", 0};
    opcodeTable["shl"]    = {"08", 0};
    opcodeTable["shr"]    = {"09", 0};
    opcodeTable["adj"]    = {"0A", 1};
    opcodeTable["a2sp"]   = {"0B", 0};
    opcodeTable["sp2a"]   = {"0C", 0};
    opcodeTable["call"]   = {"0D", 2};
    opcodeTable["return"] = {"0E", 0};
    opcodeTable["brz"]    = {"0F", 2};
    opcodeTable["brlz"]   = {"10", 2};
    opcodeTable["br"]     = {"11", 2};
    opcodeTable["HALT"]   = {"12", 0};
}

void collectLabels() {
    int lineCount = static_cast<int>(source.size());

    for (int k = 0; k < lineCount; ++k) {
        const string& line = source[k];
        int lineNo = k + 1;
        string token;

        for (size_t p = 0; p < line.size(); ++p) {
            if (line[p] != ':') {
                token += line[p];
                continue;
            }

            // Found a colon — `token` is the label candidate
            if (!isValidIdentifier(token)) {
                logError(lineNo, "Invalid label name");
                break;
            }

            bool isSET  = (line.size() >= p + 5 && line.substr(p + 2, 3) == "SET");
            bool isData = (line.size() >= p + 6 && line.substr(p + 2, 4) == "data");

            if (symbolTable.count(token)) {
                // Label already seen
                if (isSET)  { ++p; continue; }
                if (isData && symbolTable[token] < 0) { symbolTable[token] = k; ++p; continue; }
                logError(lineNo, "Multiple declaration of label: " + token);
            } else {
                symbolTable[token] = isSET ? -k : k;
            }
            break;
        }
    }
}

/**
 * Emits the instruction sequence that implements a SET pseudo-op at runtime.
 * This is only used when the label has already been defined (non-defining occurrence).
 */
void emitSETSequence(vector<string>& out, const string& labelName,
                             const string& line, size_t colonPos) {
    if (line.size() < colonPos + 6) return;
    string value = line.substr(colonPos + 6);
    out.push_back("adj 10000");
    out.push_back("stl -1");
    out.push_back("stl 0");
    out.push_back("ldc " + value);
    out.push_back("ldc " + labelName.substr(0, colonPos));
    out.push_back("stnl 0");
    out.push_back("ldl 0");
    out.push_back("ldl -1");
    out.push_back("adj -10000");
}

/**
 * Expands all SET directives in `source`.
 * Defining occurrences (where the label's canonical PC is this line) are
 * rewritten as "label: data <value>". Non-defining occurrences become the
 * runtime store sequence.
 */
void expandSETDirectives() {
    vector<string> expanded;
    expanded.reserve(source.size());

    for (size_t k = 0; k < source.size(); ++k) {
        const string& line = source[k];
        string token;
        bool handled = false;

        for (size_t p = 0; p < line.size(); ++p) {
            if (line[p] != ':') { token += line[p]; continue; }

            if (line.size() >= p + 5 && line.substr(p + 2, 3) == "SET") {
                handled = true;
                if (abs(symbolTable[token]) != static_cast<int>(k)) {
                    emitSETSequence(expanded, token, line, p);
                } else {
                    symbolTable[token] = static_cast<int>(expanded.size());
                    expanded.push_back(
                        line.substr(0, p + 1) + " data " +
                        line.substr(p + 6));
                }
                break;
            }
        }

        if (!handled && !line.empty())
            expanded.push_back(line);
    }

    source = std::move(expanded);
}

// Pass 1 helpers — data segment reordering
// ---------------------------------------------------------------------------

/**
 * Moves all 'data' lines to the end of the source so that executable
 * instructions precede data declarations.
 */
void reorderDataSegment() {
    vector<string> instructions, dataLines;

    for (size_t k = 0; k < source.size(); ++k) {
        bool isDataLine = false;

        // A line is considered data if it contains "data " or the *next* line
        // starts with "data" and this line ends with ':'
        if ((!source[k].empty() && source[k].back() == ':' && k + 1 < source.size() &&
             source[k + 1].substr(0, 4) == "data")) {
            isDataLine = true;
        } else {
            for (size_t p = 0; p + 4 <= source[k].size(); ++p) {
                if (source[k].substr(p, 4) == "data") { isDataLine = true; break; }
            }
        }

        (isDataLine ? dataLines : instructions).push_back(source[k]);
    }

    instructions.insert(instructions.end(), dataLines.begin(), dataLines.end());
    source = std::move(instructions);
}

/**
 * Splits each cleaned source line into label / mnemonic / operand components,
 * assigns PC values, and validates operand types.
 */
void buildSymbolAndEntryTable() {
    int pc = 0;
    int lineCount = static_cast<int>(source.size());

    for (int k = 0; k < lineCount; ++k) {
        const string& line = source[k];
        int lineNo = k + 1;

        // Parse: split on ':' then on spaces
        string parts[4];
        int    partIdx = 1;
        string token;

        for (size_t p = 0; p < line.size(); ++p) {
            char c = line[p];

            if (c == ':') {
                parts[0] = token; token.clear();
                if (p + 1 < line.size() && line[p + 1] == ' ') ++p;
                continue;
            }
            if (c == ' ') {
                if (!token.empty() && partIdx < 4) { parts[partIdx++] = token; token.clear(); }
                continue;
            }
            token += c;
            if (p == line.size() - 1 && !token.empty() && partIdx < 4)
                parts[partIdx++] = token;
        }

        // Update symbol table with resolved PC
        if (!parts[0].empty()) symbolTable[parts[0]] = pc;

        pcValues[k]           = pc;
        tableEntries[k].hasLabel = !parts[1].empty();

        if (parts[1] == "HALT") haltPresent = true;

        // Empty / label-only line
        if (partIdx == 1) {
            tableEntries[k] = {"", "", "", 0, false};
            continue;
        }

        ++pc;

        // Validate mnemonic
        if (!opcodeTable.count(parts[1])) {
            logError(lineNo, "Invalid Mnemonic");
            continue;
        }

        // Validate operand count — too many tokens
        if (partIdx > 3) {
            logError(lineNo, "Unexpected operand / extra tokens on line");
            continue;
        }

        // Validate operand presence
        int expectedOperand = min(opcodeTable[parts[1]].operandKind, 1);
        if (expectedOperand != partIdx - 2) {
            logError(lineNo, "Invalid OPCode-Syntax combination");
            continue;
        }

        int oprType = classifyOperand(parts[2]);
        tableEntries[k] = {parts[0], parts[1], parts[2], oprType, !parts[0].empty()};

        if (oprType == 1 && !symbolTable.count(parts[2]))
            logError(lineNo, "No such label / data variable");
        else if (oprType == -1)
            logError(lineNo, "Invalid number");
    }
}

// Pass 1
// ---------------------------------------------------------------------------

bool firstPass() {
    cout << "Enter the ASM file name to assemble:\n";
    cin >> fileName;

    if (fileName.size() < 4 || fileName.substr(fileName.size() - 4) != ".asm") {
        cout << "ERROR: Please provide a .asm file.\n";
        return false;
    }

    baseName = fileName.substr(0, fileName.size() - 4);

    ifstream infile(fileName);
    if (!infile) {
        cout << "ERROR: File not found: " << fileName << "\n";
        return false;
    }

    string rawLine;
    while (getline(infile, rawLine))
        source.push_back(cleanLine(rawLine));

    collectLabels();

    if (errorLog.empty()) expandSETDirectives();

    reorderDataSegment();

    tableEntries.resize(source.size());
    pcValues.resize(source.size());

    buildSymbolAndEntryTable();
    return true;
}

// ---------------------------------------------------------------------------
// Error / log output
// ---------------------------------------------------------------------------

bool writeLogAndCheckErrors() {
    ofstream logFile(baseName + ".log");
    if (!logFile) {
        cout << "ERROR: Cannot open " << baseName << ".log\n";
        return false;
    }

    logFile << "Log written to: " << baseName << ".log\n";

    if (errorLog.empty()) {
        cout << "No errors found!\n";
        if (!haltPresent) {
            cout << "Warning: HALT not present!\n";
            logFile << "Warning: HALT not present!\n";
        }
        logFile << "Machine code: " << baseName << ".o\n";
        logFile << "Listing file: " << baseName << ".lst\n";
        return true;
    }

    sort(errorLog.begin(), errorLog.end());
    cout << errorLog.size() << " error(s) found. See " << baseName << ".log\n";
    for (size_t i = 0; i < errorLog.size(); ++i)
        logFile << errorLog[i].second << "\n";

    return false;
}

// ---------------------------------------------------------------------------
// Pass 2 — Machine code generation
// ---------------------------------------------------------------------------

void secondPass() {
    int lineCount = static_cast<int>(tableEntries.size());

    for (int k = 0; k < lineCount; ++k) {
        const auto& entry    = tableEntries[k];
        const string& mnem   = entry.mnemonic;
        const string& operand = entry.operand;
        int oprType           = entry.operandType;

        if (source[k].empty()) continue;

        int pc = pcValues[k];

        // Label-only / empty mnemonic → placeholder word
        if (mnem.empty()) {
            machineCode.push_back({pc, "        "});
            continue;
        }

        const string& opcode = opcodeTable.at(mnem).hexCode;

        if (oprType == 0) {
            // No operand
            machineCode.push_back({pc, "000000" + opcode});

        } else if (oprType == 1) {
            // Label reference — compute PC-relative or absolute offset
            int32_t offset = (opcodeTable.at(mnem).operandKind == 2)
                ? symbolTable.at(operand) - (pc + 1)   // PC-relative
                : symbolTable.at(operand);              // absolute
            machineCode.push_back({pc, zeroPad(toHexString(offset)) + opcode});

        } else {
            // Numeric literal — 32-bit for 'data', 24-bit otherwise
            bool isData   = (mnem == "data");
            int  bitWidth = isData ? 32 : 24;
            int  hexWidth = isData ? 8  : 6;
            int32_t value = static_cast<int32_t>(stoi(operand, nullptr, oprType));
            machineCode.push_back({pc, zeroPad(toHexString(value, bitWidth), hexWidth) + opcode});
        }
    }
}

// ---------------------------------------------------------------------------
// Output — binary (.o) and listing (.lst)
// ---------------------------------------------------------------------------

void writeOutputFiles() {
    // Binary machine code
    ofstream binOut(baseName + ".o", ios::binary);
    for (size_t i = 0; i < machineCode.size(); ++i) {
        const string& word = machineCode[i].second;
        if (word.empty() || word == "        ") continue;
        unsigned int x = 0;
        istringstream(word) >> hex >> x;
        binOut.write(reinterpret_cast<const char*>(&x), sizeof(x));
    }

    // Human-readable listing — iterate source lines in order, paired with machineCode
    ofstream lstOut(baseName + ".lst");
    for (size_t i = 0; i < machineCode.size(); ++i) {
        int pc   = machineCode[i].first;   // PC stored directly
        const string& word = machineCode[i].second;
        lstOut << zeroPad(toHexString(pc)) << " "
               << word << " "
               << source[i] << "\n";
    }

    cout << "Log file    : " << baseName << ".log\n"
         << "Machine code: " << baseName << ".o\n"
         << "Listing file: " << baseName << ".lst\n";
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main() {
    initOpcodeTable();
    bool check=firstPass();

    if (check && writeLogAndCheckErrors()) {
        secondPass();
        writeOutputFiles();
    }

    cout << "Press Enter to exit...";
    cin.ignore();
    cin.get();
    return 0;
}
