#include "emulator.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <thread>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

Emulator::Emulator(){
    for(auto& r: gpr){
        r = 0;
    }
    for(auto& r: csr){
        r = 0;
    }
}
void Emulator::printRegisters(){
    for (int i = 0; i < 16; i++) {
        std::cout << std::right << std::setw(3) << ("r" + std::to_string(i))
                  << "=0x"
                  << std::hex << std::setw(8) << std::setfill('0') << gpr[i]
                  << std::dec << std::setfill(' ') << "  ";

        //4 registers per line
        if (i % 4 == 3)
            std::cout << '\n';
    }
    std::cout << std::endl;
}
void Emulator::readFile(const std::string& filename){
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    while (true) {
        uint32_t address;
        uint8_t value;

        // Read 4B address
        in.read(reinterpret_cast<char*>(&address), sizeof(address));
        if (in.eof()) break;

        // Read 1B value
        in.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (in.eof()) break;

        auto it = memory.find(address);
        if (it == memory.end()){
            memory[address] = value;
        }else{
            std::ostringstream oss;
            oss << "Input error: multiple values for address 0x"
                << std::hex << std::setw(8) << std::setfill('0') << address;
            throw std::runtime_error(oss.str());
        }
        
    }
}
void Emulator::emulate(){
    emulatorRunning = true;
    gpr[PC] = START_ADDRESS;
    std::thread timer_thread{&Emulator::timer, this};
    std::thread terminal_thread{&Emulator::terminal, this};
    try{
        //Main loop
        while(emulatorRunning){
            uint32_t instruction = instructionFetch();
            instructionDecodeAndExecute(instruction);
            handleInterrupts();
        }
        //Regularly exited - print out register states
        std::cout << "\n-----------------------------------------------------------------\n";
        std::cout << "Emulated processor executed halt instruction\n";
        std::cout << "Emulated processor state:\n";
        printRegisters();
    }catch(std::runtime_error& ex){
        //Fatal error curred during execution - print out register states
        std::cout << "\n-----------------------------------------------------------------\n";
        std::cout << "Emulated processor encountered a fatal error:" << ex.what() << "\n";
        std::cout << "Emulated processor state:\n";
        printRegisters();

    }
    //Join with timer and terminal threads
    if (timer_thread.joinable()) timer_thread.join();
    if (terminal_thread.joinable()) terminal_thread.join();
}

uint32_t Emulator::instructionFetch(){
    uint32_t instruction = readWordMem(gpr[PC]);
    gpr[PC] += 4;
    return instruction;
}
void Emulator::instructionDecodeAndExecute(uint32_t instruction){
    uint8_t byte0 = static_cast<uint8_t>((instruction >> 0) & 0xFF);
    uint8_t byte1 = static_cast<uint8_t>((instruction >> 8) & 0xFF);
    uint8_t byte2 = static_cast<uint8_t>((instruction >> 16) & 0xFF);
    uint8_t byte3 = static_cast<uint8_t>((instruction >> 24) & 0xFF);

    //byte0: oc | mod
    uint8_t oc = byte0 >> 4;
    uint8_t mod = byte0 & 0x0F;
    //byte1: a | b
    uint8_t a = byte1 >> 4;
    uint8_t b = byte1 & 0x0F;
    //byte2: c | disp11..8
    uint8_t c = byte2 >> 4;
    //byte3: disp7..0
    uint16_t disp = (static_cast<uint16_t>(byte2 & 0x0F)<<8) | byte3; 
    if (disp & 0x800) disp |= 0xF000; //extend sign if disp was negative

    if(oc == 0x0){
        //Halt execution
        executeHalt(mod,a,b,c,disp);
    }else if(oc == 0x1){
        //Software interrupt
        executeInt(mod,a,b,c,disp);
    }else if(oc == 0x2){
        //Call
        executeCall(mod,a,b,c,disp);
    }else if(oc == 0x3){
        //Jump
        executeJump(mod,a,b,c,disp);
    }else if(oc == 0x4){
        //Xchg
        executeXchg(mod,a,b,c,disp);
    }else if(oc == 0x5){
        //Arithmetic
        executeArithmetic(mod,a,b,c,disp);
    }else if(oc == 0x6){
        //Logic
        executeLogic(mod,a,b,c,disp);
    }else if(oc == 0x7){
        //Shift
        executeShift(mod,a,b,c,disp);
    }else if(oc == 0x8){
        //Store
        executeStore(mod,a,b,c,disp);
    }else if(oc == 0x9){
        //Load
        executeLoad(mod,a,b,c,disp);
    }else{
        illegalInstructionInterrupt();
    }
}
void Emulator::handleInterrupts(){
    if(illegalInstruction){
        //push status
        gpr[SP] -= 4;
        writeWordMem(gpr[SP], csr[STATUS]);

        //push pc
        gpr[SP] -= 4;
        writeWordMem(gpr[SP], gpr[PC]);

        //cause <= 1
        csr[CAUSE] = 1;

        //status <= status|(0x4) mask all interrupts
        csr[STATUS] = csr[STATUS] | (0x4);

        //pc <= handler
        gpr[PC] = csr[HANDLER];

        illegalInstruction = false;
    }
    else if(softwareInterrupt){
        //push status
        gpr[SP] -= 4;
        writeWordMem(gpr[SP], csr[STATUS]);

        //push pc
        gpr[SP] -= 4;
        writeWordMem(gpr[SP], gpr[PC]);

        //cause <= 4
        csr[CAUSE] = 4;

        //status <= status&(~0x1)
        csr[STATUS] = csr[STATUS] & (~0x1);

        //pc <= handler
        gpr[PC] = csr[HANDLER];

        softwareInterrupt = false;
    }
    else if(!(csr[STATUS] & 0x4)){//if not globally masked

        if(timerInterrupt && !(csr[STATUS] & 0x1)){
            //push status
            gpr[SP] -= 4;
            writeWordMem(gpr[SP], csr[STATUS]);

            //push pc
            gpr[SP] -= 4;
            writeWordMem(gpr[SP], gpr[PC]);

            //cause <= 2
            csr[CAUSE] = 2;

            //status <= status|(0x4) mask all interrupts
            csr[STATUS] = csr[STATUS] | (0x4);

            //pc <= handler
            gpr[PC] = csr[HANDLER];
            timerInterrupt = false;
        }else if(terminalInterrupt && !(csr[STATUS] & 0x2)){
            //push status
            gpr[SP] -= 4;
            writeWordMem(gpr[SP], csr[STATUS]);

            //push pc
            gpr[SP] -= 4;
            writeWordMem(gpr[SP], gpr[PC]);

            //cause <= 3
            csr[CAUSE] = 3;

            //status <= status|(0x4) mask all interrupts
            csr[STATUS] = csr[STATUS] | (0x4);

            //pc <= handler
            gpr[PC] = csr[HANDLER];
            terminalInterrupt = false;
        }
    }
}

void Emulator::executeHalt(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp){
    if(mod != 0 || a != 0 || b != 0 || c != 0 || disp != 0){
        illegalInstructionInterrupt();
    }
    emulatorRunning = false;
}
void Emulator::executeInt(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp){
    if(mod != 0 || a != 0 || b != 0 || c != 0 || disp != 0){
        illegalInstructionInterrupt();
    }
    softwareInterrupt = true;
    
}
void Emulator::executeCall(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp){
    if(c != 0){
        illegalInstructionInterrupt();
    }

    if(mod == 0){
        //push pc
        gpr[SP] -= 4;
        writeWordMem(gpr[SP], gpr[PC]);
        //pc<=gpr[A]+gpr[B]+D
        gpr[PC] = gpr[a] + gpr[b] + static_cast<int16_t>(disp);
    }else if(mod == 1){
        //push pc
        gpr[SP] -= 4;
        writeWordMem(gpr[SP], gpr[PC]);
        //pc<=mem32[gpr[A]+gpr[B]+D]
        gpr[PC] = readWordMem(gpr[a] + gpr[b] + static_cast<int16_t>(disp));
    }else{
        illegalInstructionInterrupt();
    }
}
void Emulator::executeJump(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp){
    if(mod == 0){
        //pc<=gpr[A]+D;
        gpr[PC] = gpr[a] + static_cast<int16_t>(disp);
    }else if(mod == 1){
        //if (gpr[B] == gpr[C]) pc<=gpr[A]+D;
        if(gpr[b] == gpr[c]) gpr[PC] = gpr[a] + static_cast<int16_t>(disp);
    }else if(mod == 2){
        //if (gpr[B] != gpr[C]) pc<=gpr[A]+D;
        if(gpr[b] != gpr[c]) gpr[PC] = gpr[a] + static_cast<int16_t>(disp);
    }else if(mod == 3){
        //if (gpr[B] signed> gpr[C]) pc<=gpr[A]+D;
        if(static_cast<int32_t>(gpr[b]) > static_cast<int32_t>(gpr[c]))
            gpr[PC] = gpr[a] + static_cast<int16_t>(disp);
    }else if(mod == 8){
        //pc<=mem32[gpr[A]+D];
        gpr[PC] = readWordMem(gpr[a] + static_cast<int16_t>(disp));
    }else if(mod == 9){
        //if (gpr[B] == gpr[C]) pc<=mem32[gpr[A]+D];
        if(gpr[b] == gpr[c])
            gpr[PC] = readWordMem(gpr[a] + static_cast<int16_t>(disp));
    }else if(mod == 10){
        //if (gpr[B] != gpr[C]) pc<=mem32[gpr[A]+D];
        if(gpr[b] != gpr[c])
            gpr[PC] = readWordMem(gpr[a] + static_cast<int16_t>(disp));
    }else if(mod == 11){
        //if (gpr[B] signed> gpr[C]) pc<=mem32[gpr[A]+D];
        if(static_cast<int32_t>(gpr[b]) > static_cast<int32_t>(gpr[c]))
            gpr[PC] = readWordMem(gpr[a] + static_cast<int16_t>(disp));
    }else{
        illegalInstructionInterrupt();
    }

    
}
void Emulator::executeXchg(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp){
    if(mod != 0 || a != 0 || disp != 0){
        illegalInstructionInterrupt();
    }
    //temp<=gpr[B]; gpr[B]<=gpr[C]; gpr[C]<=temp;
    int temp = gpr[b];
    gpr[b] = (b == 0) ? 0 :gpr[c];
    gpr[c] = (c == 0) ? 0 :temp;

}
void Emulator::executeArithmetic(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp){
    if(disp != 0){
        illegalInstructionInterrupt();
    }
    if(mod == 0){
        //gpr[A]<=gpr[B] + gpr[C];
        gpr[a] = (a == 0) ? 0 :gpr[b] + gpr[c];
    }else if(mod == 1){
        //gpr[A]<=gpr[B] - gpr[C];
        gpr[a] = (a == 0) ? 0 :gpr[b] - gpr[c];
    }else if(mod == 2){
        //gpr[A]<=gpr[B] * gpr[C];
        gpr[a] = (a == 0) ? 0 :gpr[b] * gpr[c];
    }else if(mod == 3){
        //gpr[A]<=gpr[B] / gpr[C];
        if(gpr[c] != 0){
            gpr[a] = (a == 0) ? 0 :gpr[b] / gpr[c];
        }else{
            illegalInstructionInterrupt();
        }
        
    }else{
        illegalInstructionInterrupt();
    }
}
void Emulator::executeLogic(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp){
    if(disp != 0){
        illegalInstructionInterrupt();
    }
    if(mod == 0){
        //gpr[A]<=~gpr[B];
        gpr[a] = (a == 0) ? 0 :~gpr[b];
    }else if(mod == 1){
        //gpr[A]<=gpr[B] & gpr[C];
        gpr[a] = (a == 0) ? 0 :gpr[b] & gpr[c];
    }else if(mod == 2){
        //gpr[A]<=gpr[B] | gpr[C];
        gpr[a] = (a == 0) ? 0 :gpr[b] | gpr[c];
    }else if(mod == 3){
        //gpr[A]<=gpr[B] ^ gpr[C];
        gpr[a] = (a == 0) ? 0 :gpr[b] ^ gpr[c];
    }else{
        illegalInstructionInterrupt();
    }
}
void Emulator::executeShift(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp){
    if(disp != 0){
        illegalInstructionInterrupt();
    }
    if(mod == 0){
        //gpr[A]<=gpr[B] << gpr[C];
        gpr[a] = (a == 0) ? 0 :gpr[b] << gpr[c];
    }else if(mod == 1){
        //gpr[A]<=gpr[B] >> gpr[C];
        gpr[a] = (a == 0) ? 0 :gpr[b] >> gpr[c];
    }else{
        illegalInstructionInterrupt();
    }
}
void Emulator::executeStore(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp){
    if(mod == 0){
        //mem32[gpr[A]+gpr[B]+D]<=gpr[C];
        writeWordMem(gpr[a] + gpr[b] + static_cast<int16_t>(disp), gpr[c]);
    }else if(mod == 2){
        //mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C];
        writeWordMem(readWordMem(gpr[a] + gpr[b] + static_cast<int16_t>(disp)), gpr[c]);
    }else if(mod == 1){
        //gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];
        gpr[a] = (a == 0) ? 0 :gpr[a] + static_cast<int16_t>(disp);
        writeWordMem(gpr[a], gpr[c]);
    }else{
        illegalInstructionInterrupt();
    }
}
void Emulator::executeLoad(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp){
    if(mod == 0){
        //gpr[A]<=csr[B];
        gpr[a] = (a == 0) ? 0 :csr[b];
    }else if(mod == 1){
        //gpr[A]<=gpr[B]+D;
        gpr[a] = (a == 0) ? 0 :gpr[b] + static_cast<int16_t>(disp);
    }else if(mod == 2){
        //gpr[A]<=mem32[gpr[B]+gpr[C]+D];
        gpr[a] = (a == 0) ? 0 :readWordMem(gpr[b] + gpr[c] + static_cast<int16_t>(disp));
    }else if(mod == 3){
        //gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
        gpr[a] = (a == 0) ? 0 :readWordMem(gpr[b]);
        gpr[b] = (b == 0) ? 0 :gpr[b] + static_cast<int16_t>(disp);
    }else if(mod == 4){
        //csr[A]<=gpr[B];
        csr[a] = gpr[b];
    }else if(mod == 5){
        //csr[A]<=csr[B]|D;
        csr[a] = csr[b] | disp;
    }else if(mod == 6){
        //csr[A]<=mem32[gpr[B]+gpr[C]+D];
        csr[a] = readWordMem(gpr[b] + gpr[c] + static_cast<int16_t>(disp));
    }else if(mod == 7){
        //csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
        csr[a] = readWordMem(gpr[b]);
        gpr[b] = (b == 0) ? 0 :gpr[b] + static_cast<int16_t>(disp);
    }else{
        illegalInstructionInterrupt();
    }
}

void Emulator::illegalInstructionInterrupt(){
    illegalInstruction = true;
}



uint8_t Emulator::readByteMem(uint32_t address) const{
    if(address >= 0xFFFFFF00U){
        std::ostringstream oss;
        oss << "Read error: can't read a singular byte from mapped address space address at 0x"
            << std::hex << std::setw(8) << std::setfill('0') << address;
        throw std::runtime_error(oss.str());
    }
    auto it = memory.find(address);
    if (it == memory.end()) {
        //Allow the program to read from wherever it wants
        // std::ostringstream oss;
        // oss << "Segmentation fault: invalid memory access at address 0x"
        //     << std::hex << std::setw(8) << std::setfill('0') << address;
        // throw std::runtime_error(oss.str());
        return 0;
    }
    return it->second;
}
uint32_t Emulator::readWordMem(uint32_t address) const {
    if (address > 0xFFFFFFFFu - 3) {
        std::ostringstream oss;
        oss << "Read error: 4-byte read crosses memory boundary at address 0x"
            << std::hex << std::setw(8) << std::setfill('0') << address;
        throw std::runtime_error(oss.str());
    }
    if(address >= 0xFFFFFF00U){
        if(address == TERM_IN_ADDR){
            return term_in;
        }else if(address == TIM_CFG_ADDR){
            return tim_cfg;
        }else{
             std::ostringstream oss;
            oss << "Read error: no matching mapped register for reading at address 0x"
                << std::hex << std::setw(8) << std::setfill('0') << address;
            throw std::runtime_error(oss.str());
        }
    }

    uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        uint8_t byte = readByteMem(address + i);
        value |= static_cast<uint32_t>(byte) << (8 * i);
    }

    return value;
}
void Emulator::writeByteMem(uint32_t address, uint8_t value) {
    if(address >= 0xFFFFFF00U){
        std::ostringstream oss;
        oss << "Write error: can't write a singular byte into mapped address space at address 0x"
            << std::hex << std::setw(8) << std::setfill('0') << address;
        throw std::runtime_error(oss.str());
    }
    memory[address] = value;

    
}
void Emulator::writeWordMem(uint32_t address, uint32_t value) {
    // Ensure that writing 4 bytes doesn’t overflow the 32-bit address space
    if (address > 0xFFFFFFFFu - 3) {
        std::ostringstream oss;
        oss << "Write error: 4-byte write crosses memory boundary at address 0x"
            << std::hex << std::setw(8) << std::setfill('0') << address;
        throw std::runtime_error(oss.str());
    }
    if(address >= 0xFFFFFF00U){
        if(address == TERM_OUT_ADDR){
            while(terminalSignal) std::this_thread::yield();
            term_out = value;
            terminalSignal = true;
            while(terminalSignal) std::this_thread::yield();
            return;
        }else if(address == TIM_CFG_ADDR){
            tim_cfg = value;
            timerStart = true;
            return;
        }else{
             std::ostringstream oss;
            oss << "Write error: no matching mapped register for writing at address 0x"
                << std::hex << std::setw(8) << std::setfill('0') << address;
            throw std::runtime_error(oss.str());
        }
    }

    // Write in little endian order
    for (int i = 0; i < 4; ++i) {
        uint8_t byte = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
        writeByteMem(address + i, byte);
    }
}

void Emulator::timer(){
    //Busy wait until the timer is started
    while(!timerStart && emulatorRunning) std::this_thread::yield();

    while(emulatorRunning){
        uint32_t cfg = tim_cfg;
        uint32_t delay;
        switch(cfg){
            case 0x0: delay =   500; break;
            case 0x1: delay =  1000; break;
            case 0x2: delay =  1500; break;
            case 0x3: delay =  2000; break;
            case 0x4: delay =  5000; break;
            case 0x5: delay = 10000; break;
            case 0x6: delay = 30000; break;
            case 0x7: delay = 60000; break;
            default:
                //If setting is unrecognized, set delay to 500ms
                delay = 500; break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        timerInterrupt = true;
    }
}

void Emulator::terminal(){
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    //Turn off canonical mode and echo
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    //Set stdin to non-blocking mode
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    //Main terminal loop
    while (emulatorRunning) {
        
        //Check for input
        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n > 0) {
            term_in = static_cast<uint8_t>(ch);
            terminalInterrupt = true;
        }

        //Check for output
        if (terminalSignal) {
            std::cout << static_cast<char>((term_out) & 0xFF) << std::flush;
            terminalSignal = false; //when terminalSignal is false the processor knows the terminal is done and not busy
        }
        std::this_thread::yield();
    } 
    //Restore terminal on exit
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
}
void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <file>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h   Show this help message and exit\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << progName << " program.hex\n\n";
}
int main(int argc, char **argv){
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string arg = argv[1];
    if (arg == "-h") {
        printUsage(argv[0]);
        return 0;
    }

    const std::string filename = arg;

    Emulator emulator;

    try {
        emulator.readFile(filename);
    } catch (const std::runtime_error& e) {
        std::cerr << "Error reading file " << filename << ": " << e.what() << "\n";
        return 1;
    }

    try {
        emulator.emulate();
    } catch (const std::runtime_error& e) {
        std::cerr << "Emulation error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
