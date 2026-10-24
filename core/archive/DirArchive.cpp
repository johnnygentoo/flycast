/*
	A folder of loose ROM files, read like an unzipped MAME set.

	This file is part of Flycast (DreamStation fork).

    Flycast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
 */
#include "DirArchive.h"

#include <zlib.h>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>

namespace {

class DirArchiveFile : public ArchiveFile
{
public:
	DirArchiveFile(hostfs::File *file, const std::string& name, size_t size)
		: file(file), name(name), size(size) {}

	u32 Read(void *buffer, u32 length) override {
		return (u32)file->read(buffer, 1, length);
	}
	size_t length() override {
		return size;
	}
	const char *getName() override {
		return name.c_str();
	}

private:
	std::unique_ptr<hostfs::File> file;
	std::string name;
	size_t size;
};

// Loading a set looks every ROM up by CRC first, and the BIOS and the game
// each open the folder separately. Remember the sums so each file is read
// once; size and modification time tell when a file has been replaced.
std::mutex crcLock;
std::map<std::string, u32> crcCache;

u32 fileCrc(const std::string& path, size_t size, u64 updateTime)
{
	const std::string key = path + '|' + std::to_string(size) + '|' + std::to_string(updateTime);
	{
		std::lock_guard<std::mutex> _(crcLock);
		auto it = crcCache.find(key);
		if (it != crcCache.end())
			return it->second;
	}
	std::unique_ptr<hostfs::File> file(hostfs::storage().openFile(path, "rb"));
	if (!file)
		return 0;
	uLong crc = crc32(0, Z_NULL, 0);
	std::vector<u8> buf(1 << 20);
	for (;;)
	{
		size_t n = file->read(buf.data(), 1, buf.size());
		if (n == 0)
			break;
		crc = crc32(crc, buf.data(), (uInt)n);
	}
	std::lock_guard<std::mutex> _(crcLock);
	crcCache[key] = (u32)crc;
	return (u32)crc;
}

} // namespace

DirArchive *DirArchive::open(const std::string& path)
{
	// Listed directly: the libretro storage can only list folders through a
	// frontend VFS interface, which a statically linked host does not provide.
	namespace fs = std::filesystem;
	std::error_code ec;
	const fs::path dir(path);
	if (!fs::is_directory(dir, ec))
		return nullptr;
	auto archive = std::make_unique<DirArchive>();
	for (const auto& e : fs::directory_iterator(dir, ec))
	{
		if (!e.is_regular_file(ec))
			continue;
		const u64 updateTime = (u64)e.last_write_time(ec).time_since_epoch().count();
		archive->entries.push_back({ e.path().filename().string(), e.path().string(),
				(size_t)e.file_size(ec), updateTime });
	}
	if (archive->entries.empty())
		return nullptr;
	return archive.release();
}

ArchiveFile *DirArchive::OpenFile(const char *name)
{
	for (const auto& e : entries)
		if (stricmp(e.name.c_str(), name) == 0)
		{
			hostfs::File *file = hostfs::storage().openFile(e.path, "rb");
			return file != nullptr ? new DirArchiveFile(file, e.name, e.size) : nullptr;
		}
	return nullptr;
}

ArchiveFile *DirArchive::OpenFileByCrc(u32 crc)
{
	if (crc == 0)
		return nullptr;
	for (const auto& e : entries)
		if (fileCrc(e.path, e.size, e.updateTime) == crc)
		{
			hostfs::File *file = hostfs::storage().openFile(e.path, "rb");
			return file != nullptr ? new DirArchiveFile(file, e.name, e.size) : nullptr;
		}
	return nullptr;
}
