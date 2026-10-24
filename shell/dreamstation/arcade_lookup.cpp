// DreamStation host support: look up arcade ROM sets in the engine's table.
#include "hw/naomi/naomi_roms.h"

#include <cstring>

extern "C" const char* dreamstation_arcade_title(const char* set_name)
{
	for (int i = 0; Games[i].name != nullptr; i++)
		if (std::strcmp(Games[i].name, set_name) == 0)
			return Games[i].description != nullptr ? Games[i].description : Games[i].name;
	return nullptr;
}
