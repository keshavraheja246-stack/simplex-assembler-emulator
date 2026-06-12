/*
TITLE: emu.cpp
AUTHOR: Keshav Raheja
Roll No: 2401CS10

Declaration of Authorship
This test file, is part of the miniproject of CS2206 at the
department of Computer Science and Engg, IIT Patna.
*/

#include <bits/stdc++.h>
using namespace std;

const int MEM_SIZE = 16777216;  // 2^24 addressable words
const int MAX_EXECUTIONS = 30000000;  // guard against infinite loops

int  Memory[MEM_SIZE];         // main memory
vector<string> machineWords;   // hex words loaded from .o file

int A  = 0;   // accumulator
int B  = 0;   // secondary register
int PC = 0;   // program counter
int SP = 0;   // stack pointer

int execStep    = 0;   // 0=none, 1=read, 2=write
int instrCount  = 0;   // total instructions executed
int instrIndex  = 0;   // current instruction index

array<int, 2> memChange = {0, 0};  // {old address/value, new value} for read/write trace


string toHex8(unsigned int value) {
    ostringstream oss;
    oss << hex << setw(8) << setfill('0') << value;
    return oss.str();
}

/**
 * Returns true if the filename ends with ".o".
 */
bool isObjectFile(const string& filename) {
    return filename.size() >= 2 &&
           filename.compare(filename.size() - 2, 2, ".o") == 0;
}

/**
 * Reads a binary .o file into Memory[] and machineWords[].
 * Exits with an error message if the file cannot be opened.
 */
void loadObjectFile(const string& filename) {
    ifstream file(filename, ios::in | ios::binary);
    if (!file) {
        cout << "ERROR: Unable to open file: " << filename << endl;
        exit(1);
    }

    unsigned int word;
    int pos = 0;
    while (file.read(reinterpret_cast<char*>(&word), sizeof(unsigned int))) {
        Memory[pos++] = static_cast<int>(word);
        machineWords.push_back(toHex8(word));
    }
}

/**
 * Resets all global state to initial values.
 */
void resetAll() {
    machineWords.clear();
    fill(begin(Memory), end(Memory), 0);
    A = B = PC = SP = 0;
    instrIndex = instrCount = execStep = 0;
}

// instruction implementations

void ldc(int value)  { B = A; A = value; }

void adc(int value)  { A += value; }

void ldl(int offset) {
    execStep = 1;
    B = A;
    A = Memory[SP + offset];
    memChange = {SP + offset, 0};
}

void stl(int offset) {
    execStep = 2;
    memChange = {Memory[SP + offset], A};
    Memory[SP + offset] = A;
    A = B;
}

void ldnl(int offset) {
    execStep = 1;
    A = Memory[A + offset];
    memChange = {SP + offset, 0};
}

void stnl(int offset) {
    execStep = 2;
    memChange = {Memory[A + offset], B};
    Memory[A + offset] = B;
}

void add(int)  { A = B + A; }
void sub(int)  { A = B - A; }
void shl(int)  { A = B << A; }
void shr(int)  { A = B >> A; }
void adj(int value) { SP += value; }

void a2sp(int) { SP = A; A = B; }
void sp2a(int) { B = A; A = SP; }

void callInstr(int offset) { B = A; A = PC+1; PC += offset; }
void retInstr(int) { PC = A-1; A = B; }

void brz(int offset)  { if (A == 0) PC += offset; }
void brlz(int offset) { if (A < 0)  PC += offset; }
void br(int offset)   { PC += offset; }

// ---------------------------------------------------------------------------
// Function dispatch table (indexed by opcode 0–17)
// ---------------------------------------------------------------------------

void (*dispatchTable[])(int) = {
    ldc, adc, ldl, stl, ldnl, stnl,
    add, sub, shl, shr,
    adj, a2sp, sp2a,
    callInstr, retInstr,
    brz, brlz, br
};

// ---------------------------------------------------------------------------
// Memory trace helpers
// ---------------------------------------------------------------------------

void printMemoryRead() {
    cout << "Reading memory["
         << toHex8(static_cast<unsigned int>(PC)) << "], "
         << "value = " << toHex8(static_cast<unsigned int>(memChange[0]))
         << endl;
}

void resetExecution() {
    A = B = PC = SP = 0;
    instrIndex = instrCount = execStep = 0;
}

void printMemoryWrite() {
    cout << "Writing memory["
         << toHex8(static_cast<unsigned int>(PC)) << "], "
         << "from " << toHex8(static_cast<unsigned int>(memChange[0]))
         << " to "  << toHex8(static_cast<unsigned int>(memChange[1]))
         << endl;
}

/**
 * Runs up to `steps` instructions.
 * traceMode: 0 = silent, 1 = trace reads, 2 = trace writes.
 * Returns false on error (segfault / execution limit exceeded).
 */
bool runCode(int traceMode, int steps = (1 << 25)) {
    const int codeSize = static_cast<int>(machineWords.size());

    while (steps-- > 0 && PC >= 0 && PC < codeSize) {
        instrCount++;

        if (instrCount > MAX_EXECUTIONS) {
            cout << "ERROR: Execution limit reached (possible infinite loop)." << endl;
            return false;
        }

        const string& word = machineWords[PC];
        int opcode  = stoi(word.substr(6, 2), nullptr, 16);
        int operand = stoi(word.substr(0, 6), nullptr, 16);

        // Sign-extend 24-bit operand
        if (operand >= (1 << 23))
            operand -= (1 << 24);

        // HALT
        if (opcode == 18) {
            cout << "HALT reached." << endl;
            cout << instrCount << " instruction(s) executed in total." << endl;
            return true;
        }

        // FIX 1: bounds-check opcode before dispatch — without this,
        // hitting a data word (e.g. 0x4b) calls dispatchTable[75]
        // which is out of bounds and segfaults.
        if (opcode < 0 || opcode > 17) {
            return true;
        }

        execStep = 0;
        dispatchTable[opcode](operand);

        if (traceMode == 1 && execStep == 1) printMemoryRead();
        if (traceMode == 2 && execStep == 2) printMemoryWrite();

        PC++;
        instrIndex++;
    }

    return true;
}
/**
 * Decodes and prints all loaded instructions as assembly.
 */
void traceCode() {
    const string mnemonics[] = {
        "ldc","adc","ldl","stl","ldnl","stnl",
        "add","sub","shl","shr","adj","a2sp","sp2a",
        "call","return","brz","brlz","br","HALT"
    };
    const bool hasOperand[] = {
        true, true, true, true, true, true,   // ldc..stnl
        false,false,false,false,               // add..shr
        true, false,false,                     // adj, a2sp, sp2a
        true, false,true, true, true, false    // call..HALT
    };

    cout << "\n=== TRACE: Machine Code -> Assembly ===\n";
    cout << left << setw(10) << "Address"
                 << setw(12) << "HexCode"
                 << setw(10) << "Mnemonic"
                 << "Operand\n";
    cout << string(44, '-') << "\n";

    for (int i = 0; i < static_cast<int>(machineWords.size()); ++i) {
        unsigned int raw = static_cast<unsigned int>(stoul(machineWords[i], nullptr, 16));
        int opcode  = static_cast<int>(raw & 0xFF);
        int operand = static_cast<int>(raw >> 8);
        if (operand & 0x800000) operand |= static_cast<int>(0xFF000000);

        string mnem = (opcode >= 0 && opcode <= 18) ? mnemonics[opcode] : "???";
        bool   showOpr = (opcode >= 0 && opcode <= 18) && hasOperand[opcode];

        cout << left << setw(10) << i
                     << setw(12) << toHex8(raw)
                     << setw(10) << mnem;
        if (showOpr) cout << dec << operand;
        cout << "\n";
    }
    cout << string(44, '-') << "\n";
}

/**
 * Prints register values before execution starts.
 */
void showBefore() {
    cout << "\n=== REGISTER VALUES BEFORE EXECUTION ===\n";
    cout << left << setw(5) << "A"  << " = " << A  << "\n";
    cout << left << setw(5) << "B"  << " = " << B  << "\n";
    cout << left << setw(5) << "PC" << " = " << PC << "\n";
    cout << left << setw(5) << "SP" << " = " << SP << "\n";
    cout << string(41, '=') << "\n";
}

/**
 * Runs the full program then prints register values.
 */

void showAfter() {
    resetExecution();
    runCode(0);
    cout << "\n=== REGISTER VALUES AFTER EXECUTION ===\n";
    cout << left << setw(5) << "A"  << " = " << A  << "\n";
    cout << left << setw(5) << "B"  << " = " << B  << "\n";
    cout << left << setw(5) << "PC" << " = " << PC << "\n";
    cout << left << setw(5) << "SP" << " = " << SP << "\n";
    cout << string(40, '=') << "\n";
}

/**
 * Runs the full program then prints all non-zero memory locations.
 */
void dumpMemory() {
    runCode(0);
    cout << "\n=== MEMORY DUMP AFTER EXECUTION ===\n";
    cout << left << setw(12) << "Address"
                 << setw(12) << "Hex"
                 << "Decimal\n";
    cout << string(36, '-') << "\n";

    for (int i = 0; i < MEM_SIZE; ++i) {
        if (Memory[i] != 0) {
            cout << left
                 << setw(12) << dec << i
                 << setw(12) << toHex8(static_cast<unsigned int>(Memory[i]))
                 << dec << Memory[i] << "\n";
        }
    }
    cout << string(36, '-') << "\n";
}


/**
 * Reads one command from stdin and executes it.
 * Returns false if the emulator should stop.
 */
bool runREPL() {
    cout << "\nEnter a command (or 0 to exit):\n";
    string cmd;
    cin >> cmd;

    if      (cmd == "-trace")  { traceCode();  }
    else if (cmd == "-before") { showBefore(); }
    else if (cmd == "-after")  { showAfter();  }
    else if (cmd == "-dump")   { dumpMemory(); }
    else if (cmd == "0")       { resetAll(); exit(0); }
    else {
        cout << "Unknown command. Valid commands: -trace, -before, -after, -dump, 0\n";
    }

    return true;
}

//Main -----------------------------------

int main() {
    cout << "Welcome to the Emulator\n";
    cout << "Please enter the .o file name: ";

    string filename;
    cin >> filename;

    if (!isObjectFile(filename)) {
        cout << "ERROR: Invalid file format. Please provide a .o file.\n";
        return 1;
    }

    loadObjectFile(filename);

    cout << "\nAvailable commands:\n"
         << "  -trace   Decode and display all instructions\n"
         << "  -before  Show register values before execution\n"
         << "  -after   Run program and show register values after\n"
         << "  -dump    Run program and dump non-zero memory\n"
         << "  0        Exit\n";

    while (true) {
        if (!runREPL()) {
            cout << "Emulator stopped due to an error.\n";
            break;
        }
    }

    return 0;
}
