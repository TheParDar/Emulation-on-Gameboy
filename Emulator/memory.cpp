#include "memory.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "timer.hpp"

#include <cstring>

namespace gb {

Memory::Memory(Cartridge& cart) : cart_(cart) {}

void Memory::connect(CPU* cpu, PPU* ppu, Timer* timer) {
    cpu_ = cpu;
    ppu_ = ppu;
    timer_ = timer;
}

u8 Memory::read(u16 address) const {
    if (address < 0x8000)
        return cart_.read(address);

    if (address < 0xA000)
        return vram_[address - 0x8000];

    if (address < 0xC000)
        return 0xFF; // external RAM stub

    if (address < 0xE000)
        return wram_[address - 0xC000];

    if (address < 0xFE00)
        return wram_[address - 0xE000]; // echo

    if (address < 0xFE00 + sizeof(oam_))
        return oam_[address - 0xFE00];

    if (address >= 0xFF00)
        return read_io(address);

    return 0xFF;
}

void Memory::write(u16 address, u8 value) {
    if (address < 0x8000) {
        cart_.write(address, value);
        return;
    }

    if (address < 0xA000) {
        vram_[address - 0x8000] = value;
        return;
    }

    if (address < 0xC000)
        return; // external RAM stub

    if (address < 0xE000) {
        wram_[address - 0xC000] = value;
        return;
    }

    if (address < 0xFE00) {
        wram_[address - 0xE000] = value;
        return;
    }

    if (address < 0xFE00 + sizeof(oam_)) {
        oam_[address - 0xFE00] = value;
        return;
    }

    if (address >= 0xFF00)
        write_io(address, value);
}

u8 Memory::read_io(u16 address) const {
    switch (address) {
    case 0xFF00: return 0xCF; // joypad — no keys
    case 0xFF04: return timer_ ? timer_->read_div() : 0;
    case 0xFF05: return timer_ ? timer_->read_tima() : 0;
    case 0xFF06: return timer_ ? timer_->read_tma() : 0;
    case 0xFF07: return timer_ ? timer_->read_tac() : 0;
    case 0xFF0F: return if_;
    case 0xFF40:
    case 0xFF41:
    case 0xFF42:
    case 0xFF43:
    case 0xFF44:
    case 0xFF45:
    case 0xFF47:
    case 0xFF48:
    case 0xFF49:
    case 0xFF4A:
    case 0xFF4B:
        return ppu_ ? ppu_->read_register(address) : 0;
    case 0xFF80: case 0xFF81: case 0xFF82: case 0xFF83:
    case 0xFF84: case 0xFF85: case 0xFF86: case 0xFF87:
    case 0xFF88: case 0xFF89: case 0xFF8A: case 0xFF8B:
    case 0xFF8C: case 0xFF8D: case 0xFF8E:
        return hram_[address - 0xFF80];
    case 0xFFFF: return ie_;
    default:
        if (address >= 0xFF80 && address <= 0xFFFE)
            return hram_[address - 0xFF80];
        return 0xFF;
    }
}

void Memory::write_io(u16 address, u8 value) {
    switch (address) {
    case 0xFF00: break;
    case 0xFF04:
        if (timer_) timer_->write_div(value);
        break;
    case 0xFF05:
        if (timer_) timer_->write_tima(value);
        break;
    case 0xFF06:
        if (timer_) timer_->write_tma(value);
        break;
    case 0xFF07:
        if (timer_) timer_->write_tac(value);
        break;
    case 0xFF0F:
        if_ = value;
        break;
    case 0xFF40: case 0xFF41: case 0xFF42: case 0xFF43:
    case 0xFF44: case 0xFF45: case 0xFF47: case 0xFF48:
    case 0xFF49: case 0xFF4A: case 0xFF4B:
        if (ppu_) ppu_->write_register(address, value);
        break;
    case 0xFF80: case 0xFF81: case 0xFF82: case 0xFF83:
    case 0xFF84: case 0xFF85: case 0xFF86: case 0xFF87:
    case 0xFF88: case 0xFF89: case 0xFF8A: case 0xFF8B:
    case 0xFF8C: case 0xFF8D: case 0xFF8E:
        hram_[address - 0xFF80] = value;
        break;
    case 0xFFFF:
        ie_ = value;
        break;
    default:
        if (address >= 0xFF80 && address <= 0xFFFE)
            hram_[address - 0xFF80] = value;
        break;
    }
}

void Memory::request_interrupt(u8 bit) {
    if_ |= (1u << bit);
}

} // namespace gb
