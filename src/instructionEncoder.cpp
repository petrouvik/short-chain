#include "instructionEncoder.hpp"
#include <stdexcept>

std::vector<uint8_t> InstructionEncoder::makeInstruction(
    uint8_t oc,
    uint8_t mod,
    uint8_t a,
    uint8_t b,
    uint8_t c,
    int d
)
{ 
    if (d < MIN_DISP || d > MAX_DISP) {
        throw std::out_of_range("Error: Literal or symbol cannot be represented as 12-bit signed value.");
    }
    if (a < 0 || a > 15 || b < 0 || b > 15 || c < 0 || c > 15){
        throw std::out_of_range("Internal error: register index does not fit 4 bits.");
    }
    uint8_t byte0 = static_cast<uint8_t>((oc << 4) | (mod & 0x0F));
    uint8_t byte1 = static_cast<uint8_t>((a << 4) | (b & 0x0F));
    uint8_t byte2 = static_cast<uint8_t>((c << 4) | ((d >> 8) & 0x0F));
    uint8_t byte3 = static_cast<uint8_t>(d & 0xFF);

    return { byte0, byte1, byte2, byte3 };
}

std::vector<uint8_t> InstructionEncoder::word(int value) {
    std::vector<uint8_t> bytes(4);
    bytes[0] = static_cast<uint8_t>(value & 0xFF);
    bytes[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    bytes[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    bytes[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    return bytes;
}

std::vector<uint8_t> InstructionEncoder::halt(){
    //00 00 00 00
    return makeInstruction(0x0,0x0,0x0,0x0,0x0,0x000);
}
std::vector<uint8_t> InstructionEncoder::intr(){
    //10 00 00 00
    return makeInstruction(0x1,0x0,0x0,0x0,0x0,0x000);
}
std::vector<uint8_t> InstructionEncoder::iret(){
    //96 <status><sp> 00 04
    //93 <pc><sp> 00 08
    std::vector<uint8_t> result;
    auto first = makeInstruction(0x9,0x6,STATUS,SP,0x0,0x004);
    auto second = makeInstruction(0x9,0x3,PC,SP,0x0,0x008);

    result.reserve(first.size() + second.size());
    result.insert(result.end(), first.begin(), first.end());
    result.insert(result.end(), second.begin(), second.end());

    return result;
}
std::vector<uint8_t> InstructionEncoder::call(int address){
    //21 <PC>0 00 04
    //30 <PC>0 00 04
    //<address>

    std::vector<uint8_t> result;
    auto instr1 = makeInstruction(0x2, 0x1, PC, 0x0, 0x0, 0x004);
    auto instr2 = makeInstruction(0x3, 0x0, PC, 0x0, 0x0, 0x004);
    auto addrBytes = word(address);

    result.insert(result.end(), instr1.begin(), instr1.end());
    result.insert(result.end(), instr2.begin(), instr2.end());
    result.insert(result.end(), addrBytes.begin(), addrBytes.end());

    return result;
}
std::vector<uint8_t> InstructionEncoder::ret(){
    //93 <pc><sp> 00 04
    return makeInstruction(0x9, 0x3, PC, SP, 0x0, 0x004);
}
std::vector<uint8_t> InstructionEncoder::jmp(int address) {
    //38 <PC>0 00 00
    //<address>

    std::vector<uint8_t> result;
    auto instr1 = makeInstruction(0x3, 0x8, PC, 0x0, 0x0, 0x000);
    auto addrBytes = word(address);

    result.insert(result.end(), instr1.begin(), instr1.end());
    result.insert(result.end(), addrBytes.begin(), addrBytes.end());

    return result;
}
std::vector<uint8_t> InstructionEncoder::beq(int gpr1, int gpr2, int address) {
    //39 <PC><gpr1> <gpr2>0 04
    //30 <PC>0 00 04
    //<address>

    std::vector<uint8_t> result;
    auto instr1 = makeInstruction(0x3, 0x9, PC, gpr1, gpr2, 0x004);
    auto instr2 = makeInstruction(0x3, 0x0, PC, 0x0, 0x0, 0x004);
    auto addrBytes = word(address);

    result.insert(result.end(), instr1.begin(), instr1.end());
    result.insert(result.end(), instr2.begin(), instr2.end());
    result.insert(result.end(), addrBytes.begin(), addrBytes.end());

    return result;
}
std::vector<uint8_t> InstructionEncoder::bne(int gpr1, int gpr2, int address) {
    //3A <PC><gpr1> <gpr2>0 04
    //30 <PC>0 00 04
    //<address>

    std::vector<uint8_t> result;
    auto instr1 = makeInstruction(0x3, 0xA, PC, gpr1, gpr2, 0x004);
    auto instr2 = makeInstruction(0x3, 0x0, PC, 0x0, 0x0, 0x004);
    auto addrBytes = word(address);

    result.insert(result.end(), instr1.begin(), instr1.end());
    result.insert(result.end(), instr2.begin(), instr2.end());
    result.insert(result.end(), addrBytes.begin(), addrBytes.end());

    return result;
}
std::vector<uint8_t> InstructionEncoder::bgt(int gpr1, int gpr2, int address) {
    //3B <PC><gpr1> <gpr2>0 04
    //30 <PC>0 00 04
    //<address>

    std::vector<uint8_t> result;
    auto instr1 = makeInstruction(0x3, 0xB, PC, gpr1, gpr2, 0x004);
    auto instr2 = makeInstruction(0x3, 0x0, PC, 0x0, 0x0, 0x004);
    auto addrBytes = word(address);

    result.insert(result.end(), instr1.begin(), instr1.end());
    result.insert(result.end(), instr2.begin(), instr2.end());
    result.insert(result.end(), addrBytes.begin(), addrBytes.end());

    return result;
}
std::vector<uint8_t> InstructionEncoder::push(int gpr){
    //81 <sp>0 <gpr>F FC = -4
    return makeInstruction(0x8, 0x1, SP, 0x0, gpr,-4);
}
std::vector<uint8_t> InstructionEncoder::pop(int gpr){
    //93 <gpr><sp> 00 04
    return makeInstruction(0x9, 0x3, gpr, SP, 0x0, 4);
}
std::vector<uint8_t> InstructionEncoder::xchg(int gprS, int gprD){
    //40 0<gprS> <gprD>0 00
    return makeInstruction(0x4, 0x0, 0x0, gprS, gprD, 0x000);
}
std::vector<uint8_t> InstructionEncoder::add(int gprS, int gprD){
    //50 <gprD><gprD> <gprS>0 00
    return makeInstruction(0x5, 0x0, gprD, gprD, gprS, 0x000);
}
std::vector<uint8_t> InstructionEncoder::sub(int gprS, int gprD){
    //51 <gprD><gprD> <gprS>0 00
    return makeInstruction(0x5, 0x1, gprD, gprD, gprS, 0x000);
}
std::vector<uint8_t> InstructionEncoder::mul(int gprS, int gprD){
    //52 <gprD><gprD> <gprS>0 00
    return makeInstruction(0x5, 0x2, gprD, gprD, gprS, 0x000);
}
std::vector<uint8_t> InstructionEncoder::div(int gprS, int gprD){
    //53 <gprD><gprD> <gprS>0 00
    return makeInstruction(0x5, 0x3, gprD, gprD, gprS, 0x000);
}
std::vector<uint8_t> InstructionEncoder::bit_not(int gpr){
    //60 <gpr><gpr> 00 00
    return makeInstruction(0x6, 0x0, gpr, gpr, 0x0, 0x000);
}
std::vector<uint8_t> InstructionEncoder::bit_and(int gprS, int gprD){
    //61 <gprD><gprD> <gprS>0 00
    return makeInstruction(0x6, 0x1, gprD, gprD, gprS, 0x000);
}
std::vector<uint8_t> InstructionEncoder::bit_or(int gprS, int gprD){
    //62 <gprD><gprD> <gprS>0 00
    return makeInstruction(0x6, 0x2, gprD, gprD, gprS, 0x000);
}
std::vector<uint8_t> InstructionEncoder::bit_xor(int gprS, int gprD){
    //63 <gprD><gprD> <gprS>0 00
    return makeInstruction(0x6, 0x3, gprD, gprD, gprS, 0x000);
}
std::vector<uint8_t> InstructionEncoder::shl(int gprS, int gprD){
    //70 <gprD><gprD> <gprS>0 00
    return makeInstruction(0x7, 0x0, gprD, gprD, gprS, 0x000);
}
std::vector<uint8_t> InstructionEncoder::shr(int gprS, int gprD){
    //71 <gprD><gprD> <gprS>0 00
    return makeInstruction(0x7, 0x1, gprD, gprD, gprS, 0x000);
}
std::vector<uint8_t> InstructionEncoder::ld_immediate(int gpr, int imm) {
    //93 <gpr><PC> 00 04
    //<imm>

    std::vector<uint8_t> result;
    auto instr1 = makeInstruction(0x9, 0x3, gpr, PC, 0x0, 0x004);
    auto immBytes = word(imm);

    result.insert(result.end(), instr1.begin(), instr1.end());
    result.insert(result.end(), immBytes.begin(), immBytes.end());

    return result;
}
std::vector<uint8_t> InstructionEncoder::ld_memory(int gpr, int address) {
    //93 <gpr><PC> 00 04
    //<address>
    //92 <gpr><gpr> 00 00

    std::vector<uint8_t> result;
    auto instr1 = makeInstruction(0x9, 0x3, gpr, PC, 0x0, 0x004);
    auto addrBytes = word(address);
    auto instr2 = makeInstruction(0x9, 0x2, gpr, gpr, 0x0, 0x000);

    result.insert(result.end(), instr1.begin(), instr1.end());
    result.insert(result.end(), addrBytes.begin(), addrBytes.end());
    result.insert(result.end(), instr2.begin(), instr2.end());

    return result;
}
std::vector<uint8_t> InstructionEncoder::ld_register(int gpr, int reg){
    // 91 <gpr><reg> 00 00
    return makeInstruction(0x9, 0x1, gpr, reg, 0x0, 0x000);
}
std::vector<uint8_t> InstructionEncoder::ld_register_indirect(int gpr, int reg){
    // 92 <gpr><reg> 00 00
    return makeInstruction(0x9, 0x2, gpr, reg, 0x0, 0x000);
}
std::vector<uint8_t> InstructionEncoder::ld_register_indirect_disp(int gpr, int reg, int disp){
    // 92 <gpr><reg> 0<op2> <op1><op0>
    return makeInstruction(0x9, 0x2, gpr, reg, 0x0, disp);
}
std::vector<uint8_t> InstructionEncoder::st_direct(int gpr, int address) {
    //82 <PC>0 <gpr>0 04
    //30 <PC>0 00 04
    //<address>
    std::vector<uint8_t> result;
    auto instr1 = makeInstruction(0x8, 0x2, PC, 0x0, gpr, 0x004);
    auto instr2 = makeInstruction(0x3, 0x0, PC, 0x0, 0x0, 0x004);
    auto addrBytes = word(address);

    result.insert(result.end(), instr1.begin(), instr1.end());
    result.insert(result.end(), instr2.begin(), instr2.end());
    result.insert(result.end(), addrBytes.begin(), addrBytes.end());

    return result;
}
std::vector<uint8_t> InstructionEncoder::st_register_indirect(int gpr, int reg){
    // 80 <reg>0 <gpr>0 00
    return makeInstruction(0x8, 0x0, reg, 0x0, gpr, 0x000);
}
std::vector<uint8_t> InstructionEncoder::st_register_indirect_disp(int gpr, int reg, int disp){
    // 80 <reg>0 <gpr><op2> <op1><op0>
    return makeInstruction(0x8, 0x0, reg, 0x0, gpr, disp);
}
std::vector<uint8_t> InstructionEncoder::csrrd(int csr, int gpr) {
    // 90 <gpr><csr> 00 00
    return makeInstruction(0x9, 0x0, gpr, csr, 0x0, 0x000);
}
std::vector<uint8_t> InstructionEncoder::csrwr(int gpr, int csr) {
    // 94 <csr><gpr> 00 00
    return makeInstruction(0x9, 0x4, csr, gpr, 0x0, 0x000);
}