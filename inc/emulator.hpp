#pragma once
#include <string>
#include <cstdint>
#include <map>
#include <atomic>
class Emulator{
public:
    Emulator();
    //Read the .hex file
    void readFile(const std::string& filename);
    //Start emulation
    void emulate();
private:
    int gpr[16];
    int csr[3];
    std::map<uint32_t, uint8_t> memory;
    std::atomic<bool> emulatorRunning = false;

    /*--- Interrupt signals ---*/ 
    ///
    bool softwareInterrupt = false;
    bool illegalInstruction = false;
    std::atomic<bool> terminalInterrupt = false;
    std::atomic<bool> timerInterrupt = false;

    /*--- Terminal's mapped registers (and a signaling line)---*/
    ///
    std::atomic<uint32_t> term_out;
    std::atomic<bool> terminalSignal = false;
    std::atomic<uint32_t> term_in;
    
    /*--- Timer's mapped register (and a signaling line)---*/
    ///
    std::atomic<uint32_t> tim_cfg;
    std::atomic<bool> timerStart = false;

    //Initial PC value
    static constexpr uint32_t START_ADDRESS = 0x40000000;

    /*--- Indexes of special GPRs and CSR---*/
    ///
    static constexpr int PC = 15;
    static constexpr int SP = 14;
    static constexpr int STATUS = 0;
    static constexpr int HANDLER = 1;
    static constexpr int CAUSE = 2;

    /*--- Locations of memory mapped registers---*/
    ///
    static constexpr uint32_t TERM_OUT_ADDR = 0xFFFFFF00;
    static constexpr uint32_t TERM_IN_ADDR = 0xFFFFFF04;
    static constexpr uint32_t TIM_CFG_ADDR = 0xFFFFFF10;

    //Read a single byte
    uint8_t readByteMem(uint32_t address) const;
    //Read 4 bytes
    uint32_t readWordMem(uint32_t address) const;

    //Write a single byte
    void writeWordMem(uint32_t address, uint32_t value);
    //Write 4 bytes
    void writeByteMem(uint32_t address, uint8_t byte);

    //Read instruction from the location of PC and increment PC by the instruction's length (always 4)
    uint32_t instructionFetch();
    //Decode and execute instruction
    void instructionDecodeAndExecute(uint32_t instruction);

    /*--- Handlers for various instructions---*/
    ///
    void executeHalt(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp);
    void executeInt(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp);
    void executeCall(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp);
    void executeJump(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp);
    void executeXchg(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp);
    void executeArithmetic(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp);
    void executeLogic(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp);
    void executeShift(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp);
    void executeStore(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp);
    void executeLoad(uint8_t mod, uint8_t a, uint8_t b, uint8_t c, uint16_t disp);
    
    //Call if the instruction has inappropriate modifier or operands
    void illegalInstructionInterrupt();

    //Call after executing an instruction
    void handleInterrupts();

    //Timer thread's function
    void timer();

    //Terminal thread's function
    void terminal();

    //Print out the states of all GPRs
    void printRegisters();

};