#include "cartridge.hpp"

#include <fstream>
#include <iostream>

namespace gb {

bool Cartridge::load_from_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Cannot open ROM: " << path << '\n';
        return false;
    }

    const auto size = file.tellg();
    if (size < 0x8000) {
        std::cerr << "ROM too small (" << size << " bytes)\n";
        return false;
    }

    rom_.resize(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(rom_.data()), size);
    if (!file) {
        std::cerr << "ROM read failed\n";
        rom_.clear();
        return false;
    }

    title_.assign(reinterpret_cast<const char*>(&rom_[0x0134]), 16);
    auto end = title_.find('\0');
    if (end != std::string::npos)
        title_.resize(end);

    const u8 cart_type = rom_[0x0147];
    switch (cart_type) {
    case 0x00: mbc_type_ = 0; break;
    case 0x01:
    case 0x02:
    case 0x03: mbc_type_ = 1; break;
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13: mbc_type_ = 3; break;
    default:
        std::cerr << "Warning: unsupported cartridge type 0x"
                  << std::hex << static_cast<int>(cart_type)
                  << std::dec << " (using ROM-only)\n";
        mbc_type_ = 0;
        break;
    }

    rom_bank_ = 1;
    ram_bank_ = 0;
    ram_enabled_ = false;
    return true;
}

u8 Cartridge::read(u16 address) const {
    if (address < 0x4000)
        return rom_[address % rom_.size()];

    if (address < 0x8000) {
        const size_t offset = static_cast<size_t>(rom_bank_) * 0x4000
                              + (address - 0x4000);
        return rom_[offset % rom_.size()];
    }

    return 0xFF;
}

void Cartridge::write(u16 address, u8 value) {
    if (address < 0x2000) {
        if (mbc_type_ == 1)
            ram_enabled_ = (value & 0x0F) == 0x0A;
        return;
    }

    if (address < 0x4000) {
        if (mbc_type_ == 1)
            rom_bank_ = (value & 0x1F);
        if (rom_bank_ == 0)
            rom_bank_ = 1;
        return;
    }

    if (address < 0x6000 && mbc_type_ == 1) {
        ram_bank_ = value & 0x03;
    }
}

} // namespace gb
