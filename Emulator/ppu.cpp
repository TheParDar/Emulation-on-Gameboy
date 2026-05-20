#include "ppu.hpp"
#include "memory.hpp"

namespace gb {

namespace {
constexpr u32 dmg_palette[] = {
    0xFFE0F8D0u, 0xFF88C070u, 0xFF346856u, 0xFF081820u
};

u32 shade(u8 bgp, u8 color) {
    const u8 idx = (bgp >> (color * 2)) & 0x03;
    return dmg_palette[idx];
}
} // namespace

PPU::PPU(Memory& mem) : mem_(mem) {}

void PPU::reset() {
    lcdc_ = 0;
    stat_ = 0;
    scy_ = scx_ = ly_ = lyc_ = 0;
    bgp_ = 0xFC;
    obp0_ = obp1_ = 0xFF;
    wy_ = wx_ = 0;
    mode_ = 2;
    mode_clock_ = 0;
    for (auto& px : framebuffer_)
        px = dmg_palette[3];
}

u8 PPU::read_register(u16 address) const {
    switch (address) {
    case 0xFF40: return lcdc_;
    case 0xFF41: return static_cast<u8>(stat_ | 0x80 | (mode_ << 2));
    case 0xFF42: return scy_;
    case 0xFF43: return scx_;
    case 0xFF44: return ly_;
    case 0xFF45: return lyc_;
    case 0xFF47: return bgp_;
    case 0xFF48: return obp0_;
    case 0xFF49: return obp1_;
    case 0xFF4A: return wy_;
    case 0xFF4B: return wx_;
    default: return 0xFF;
    }
}

void PPU::write_register(u16 address, u8 value) {
    switch (address) {
    case 0xFF40: lcdc_ = value; break;
    case 0xFF41: stat_ = value & 0xF8; break;
    case 0xFF42: scy_ = value; break;
    case 0xFF43: scx_ = value; break;
    case 0xFF44: break; // LY read-only
    case 0xFF45: lyc_ = value; break;
    case 0xFF47: bgp_ = value; break;
    case 0xFF48: obp0_ = value; break;
    case 0xFF49: obp1_ = value; break;
    case 0xFF4A: wy_ = value; break;
    case 0xFF4B: wx_ = value; break;
    default: break;
    }
}

void PPU::set_mode(int mode) {
    mode_ = mode;
    stat_ = static_cast<u8>((stat_ & 0xFC) | mode);
}

void PPU::render_scanline() {
    if (!(lcdc_ & 0x80))
        return;

    const u16 map_base = (lcdc_ & 0x08) ? 0x9C00 : 0x9800;
    const u16 data_base = (lcdc_ & 0x10) ? 0x8000 : 0x8800;
    const int y = ly_;
    const int tile_y = (y + scy_) & 0xFF;
    const int row = tile_y % 8;

    for (int x = 0; x < SCREEN_W; ++x) {
        const int tile_x = (x + scx_) & 0xFF;
        const u16 tile_map_addr = map_base + (tile_y / 8) * 32 + (tile_x / 8);
        const u8 tile_id = mem_.read(tile_map_addr);

        u16 tile_addr;
        if (lcdc_ & 0x10)
            tile_addr = data_base + tile_id * 16u;
        else
            tile_addr = 0x9000 + static_cast<u16>(static_cast<s8>(tile_id)) * 16u;

        const u8 lo = mem_.read(tile_addr + row * 2);
        const u8 hi = mem_.read(tile_addr + row * 2 + 1);
        const u8 bit = 7 - (tile_x % 8);
        const u8 color = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
        framebuffer_[y * SCREEN_W + x] = shade(bgp_, color);
    }
}

void PPU::step(int machine_cycles) {
    if (!(lcdc_ & 0x80)) {
        mode_clock_ = 0;
        ly_ = 0;
        return;
    }

    mode_clock_ += machine_cycles;
    constexpr int mode_lengths[] = {204, 4560, 80, 172}; // hblank, vblank, oam, pixel

    while (mode_clock_ >= mode_lengths[mode_]) {
        mode_clock_ -= mode_lengths[mode_];

        if (mode_ == 0) {
            set_mode(2);
        } else if (mode_ == 2) {
            set_mode(3);
        } else if (mode_ == 3) {
            render_scanline();
            set_mode(0);
            ++ly_;
            if (ly_ == lyc_)
                stat_ |= 0x04;
            if (ly_ >= SCREEN_H) {
                ly_ = 0;
                set_mode(1);
                mem_.request_interrupt(1); // VBlank
            }
        } else if (mode_ == 1) {
            set_mode(2);
        }
    }
}

} // namespace gb
