#pragma once
#include <vector>
#include <cstdint>

class InstructionEncoder {
public:
    /* --- Offsets from beginning of instruction to the start of the field that needs to be patched --- */
    ///
    static constexpr int CALL_OP_OFFSET = 8;
    static constexpr int JMP_OP_OFFSET = 4;
    static constexpr int CONDJMP_OP_OFFSET = 8;
    static constexpr int LD_IMM_OP_OFFSET = 4;
    static constexpr int LD_MEM_OP_OFFSET = 4;
    static constexpr int LD_IND_DISP_OP_OFFSET = 2;
    static constexpr int ST_DIR_OP_OFFSET = 8;
    static constexpr int ST_IND_DISP_OP_OFFSET = 2;

    //Min value that a signed 12-bit integer can have
    static constexpr int MIN_DISP = -2048;

    //Max value that a signed 12-bit integer can have
    static constexpr int MAX_DISP = 2047;

    //Encode an integer into a vector of bytes, in little endian
    static std::vector<uint8_t> word(int value);

    /* --- Instruction encoding --- */
    ///
    static std::vector<uint8_t> halt();
    static std::vector<uint8_t> intr();
    static std::vector<uint8_t> iret();
    static std::vector<uint8_t> call(int address);
    static std::vector<uint8_t> ret();

    static std::vector<uint8_t> jmp(int offset);
    static std::vector<uint8_t> beq(int gpr1, int gpr2, int address);
    static std::vector<uint8_t> bne(int gpr1, int gpr2, int address);
    static std::vector<uint8_t> bgt(int gpr1, int gpr2, int address);

    static std::vector<uint8_t> push(int gpr);
    static std::vector<uint8_t> pop(int gpr);

    static std::vector<uint8_t> xchg(int gprS, int gprD);
    static std::vector<uint8_t> add(int gprS, int gprD);
    static std::vector<uint8_t> sub(int gprS, int gprD);
    static std::vector<uint8_t> mul(int gprS, int gprD);
    static std::vector<uint8_t> div(int gprS, int gprD);

    static std::vector<uint8_t> bit_not(int gpr);
    static std::vector<uint8_t> bit_and(int gprS, int gprD);
    static std::vector<uint8_t> bit_or (int gprS, int gprD);
    static std::vector<uint8_t> bit_xor(int gprS, int gprD);

    static std::vector<uint8_t> shl(int gprS, int gprD);
    static std::vector<uint8_t> shr(int gprS, int gprD);

    static std::vector<uint8_t> ld_immediate(int gpr, int imm);
    static std::vector<uint8_t> ld_memory(int gpr, int address);
    static std::vector<uint8_t> ld_register(int gpr, int reg);
    static std::vector<uint8_t> ld_register_indirect(int gpr, int reg);
    static std::vector<uint8_t> ld_register_indirect_disp(int gpr, int reg, int disp);

    static std::vector<uint8_t> st_direct(int gpr, int address);
    static std::vector<uint8_t> st_register_indirect(int gpr, int reg);
    static std::vector<uint8_t> st_register_indirect_disp(int gpr, int reg, int disp);

    static std::vector<uint8_t> csrrd(int csr, int gpr);
    static std::vector<uint8_t> csrwr(int gpr, int csr);
private:
    /* --- Special register indexes --- */
    ///
    static constexpr uint8_t PC = 15;
    static constexpr uint8_t SP = 14;
    static constexpr uint8_t STATUS = 0;

    //Encode an instruction as a vector of 4 bytes
    static std::vector<uint8_t> makeInstruction(
        uint8_t oc,
        uint8_t mod,
        uint8_t a,
        uint8_t b,
        uint8_t c,
        int d
    );
};
