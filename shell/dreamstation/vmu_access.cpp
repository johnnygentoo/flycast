// DreamStation host support: the VMUs of a running game, for a host that
// shows their screens itself and can take a VMU out to play its mini-game.
// Called between frames, on the thread that runs them.
#include "hw/maple/maple_cfg.h"
#include "hw/maple/maple_devs.h"
#include "hw/maple/maple_if.h"
#include "oslib/oslib.h"
#include "rend/osd.h"

#include <cstring>
#include <string>

extern u8 dreamstation_vmu_held[MAPLE_PORTS];

namespace {

bool valid(int bus, int slot)
{
	return bus >= 0 && bus < MAPLE_PORTS && slot >= 0 && slot < 2;
}

} // namespace

// The screen of the VMU in `slot` (0, 1) of controller `bus` (0..3): 48x32
// dots, non-zero = lit. Returns false if it has not shown anything yet;
// `serial` changes with every new image.
extern "C" bool dreamstation_vmu_screen(int bus, int slot, unsigned char* dots,
                                        unsigned long long* serial)
{
	if (!valid(bus, slot))
		return false;
	const int id = bus * 2 + slot;
	if (vmu_lcd_serial[id] == 0)
		return false;
	// The engine keeps them as a picture, black (0) where a dot is lit, with
	// the rows as the VMU holds them: it sits upside down in the controller,
	// so games draw them bottom row first.
	for (int y = 0; y < 32; y++)
		for (int x = 0; x < 48; x++)
			dots[y * 48 + x] = vmu_lcd_raw[id][(31 - y) * 48 + x] == 0 ? 1 : 0;
	if (serial != nullptr)
		*serial = vmu_lcd_serial[id];
	return true;
}

// A VMU is plugged into that slot now.
extern "C" bool dreamstation_vmu_present(int bus, int slot)
{
	if (!valid(bus, slot))
		return false;
	const auto& dev = MapleDevices[bus][slot];
	return dev != nullptr && dev->get_device_type() == MDT_SegaVMU;
}

// The card file the VMU in that slot reads and writes.
extern "C" const char* dreamstation_vmu_path(int bus, int slot)
{
	static std::string path;
	if (!valid(bus, slot))
		return nullptr;
	const char port[3] = { char('A' + bus), char('1' + slot), 0 };
	path = hostfs::getVmuPath(port, true);
	return path.c_str();
}

// Takes the VMU out of that slot (held) or puts it back, as a player pulling
// it from the controller would: the game sees it go and, plugged back, reads
// the card file afresh.
extern "C" void dreamstation_vmu_hold(int bus, int slot, bool held)
{
	if (!valid(bus, slot))
		return;
	const u8 bit = u8(1 << slot);
	if (held == ((dreamstation_vmu_held[bus] & bit) != 0))
		return;
	if (held)
		dreamstation_vmu_held[bus] |= bit;
	else
		dreamstation_vmu_held[bus] &= u8(~bit);
	maple_ReconnectDevice(bus, slot);
}

// For a new game: forgets which VMUs were taken out (without touching
// devices) and the screens the last game drew.
extern "C" void dreamstation_vmu_release_all()
{
	std::memset(dreamstation_vmu_held, 0, sizeof(dreamstation_vmu_held));
	std::memset(vmu_lcd_serial, 0, sizeof(vmu_lcd_serial));
}
