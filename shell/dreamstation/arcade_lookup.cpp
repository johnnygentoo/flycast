// DreamStation host support: look up arcade ROM sets in the engine's table.
#include "hw/naomi/naomi_roms.h"

#include <cstring>

namespace {

const Game *findSet(const char *set_name)
{
	for (int i = 0; Games[i].name != nullptr; i++)
		if (std::strcmp(Games[i].name, set_name) == 0)
			return &Games[i];
	return nullptr;
}

} // namespace

extern "C" const char* dreamstation_arcade_title(const char* set_name)
{
	const Game *game = findSet(set_name);
	if (game == nullptr)
		return nullptr;
	return game->description != nullptr ? game->description : game->name;
}

// The board a set runs on, decided the same way as naomi_cart_GetPlatform().
extern "C" const char* dreamstation_arcade_board(const char* set_name)
{
	const Game *game = findSet(set_name);
	if (game == nullptr)
		return nullptr;
	if (game->cart_type == AW)
		return "Atomiswave";
	if (game->bios != nullptr && std::strcmp(game->bios, "naomi2") == 0)
		return "Naomi 2";
	if (game->bios != nullptr && std::strcmp(game->bios, "segasp") == 0)
		return "System SP";
	return "Naomi";
}

extern "C" const char* dreamstation_arcade_parent(const char* set_name)
{
	const Game *game = findSet(set_name);
	return game != nullptr ? game->parent_name : nullptr;
}

// Name of the GD-ROM image (<name>.chd) a set boots from, if any.
extern "C" const char* dreamstation_arcade_gdrom(const char* set_name)
{
	const Game *game = findSet(set_name);
	return game != nullptr ? game->gdrom_name : nullptr;
}

// The index-th ROM file the set needs. Returns false past the last one.
// Optional files (a default EEPROM) are flagged; copies of another ROM are
// not files and are skipped.
extern "C" bool dreamstation_arcade_rom(const char* set_name, int index, const char** file_name,
                                        unsigned* length, unsigned* crc, bool* optional)
{
	const Game *game = findSet(set_name);
	if (game == nullptr)
		return false;
	int n = 0;
	for (int i = 0; i < MAX_GAME_FILES && game->blobs[i].filename != nullptr; i++)
	{
		if (game->blobs[i].blob_type == Copy)
			continue;
		if (n++ == index)
		{
			*file_name = game->blobs[i].filename;
			*length = game->blobs[i].length;
			*crc = game->blobs[i].crc;
			*optional = game->blobs[i].blob_type == Eeprom;
			return true;
		}
	}
	return false;
}
