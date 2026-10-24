/*
	A folder of loose ROM files, read like an unzipped MAME set
	(roms/pstone/ instead of roms/pstone.zip).

	This file is part of Flycast (DreamStation fork).

    Flycast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
 */
#pragma once

#include "archive.h"

#include <string>
#include <vector>

class DirArchive : public Archive
{
public:
	ArchiveFile *OpenFile(const char *name) override;
	ArchiveFile *OpenFileByCrc(u32 crc) override;

	// Opens the folder if it exists and contains at least one file.
	static DirArchive *open(const std::string& path);

protected:
	bool Open(hostfs::File *file) override { return false; }

private:
	struct Entry
	{
		std::string name;
		std::string path;
		size_t size;
		u64 updateTime;
	};
	std::vector<Entry> entries;
};
