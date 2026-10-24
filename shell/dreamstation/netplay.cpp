// DreamStation host support: GGPO rollback netplay and Dreamcast Internet
// settings that the libretro shell leaves to its own GUI.
//
// A netplay game is configured before it loads; once loaded the host starts
// the session (connecting to the peer and synchronising, in the background)
// and runs no frame until it has started. Called on the thread that runs the
// frames.
#include "cfg/option.h"
#include "network/net_handshake.h"
#include "stdclass.h"

#include <chrono>
#include <cstring>
#include <future>
#include <string>

namespace {

std::future<bool> g_status;
int g_state = 0;            // 0 off, 1 connecting, 2 running, -1 failed
std::string g_error;

} // namespace

// Before a game loads: netplay on or off, as host (player 1) or joining
// `peer` ("address" or "address:port"), with `delay` frames of input delay.
extern "C" void dreamstation_netplay_configure(bool enable, bool host, const char* peer, int delay)
{
	config::GGPOEnable.set(enable);
	config::ActAsServer.set(host);
	config::NetworkServer.set(peer != nullptr ? peer : "");
	config::GGPODelay.set(delay);
	g_state = 0;
	g_error.clear();
}

// After the game has loaded: starts the session. False if netplay is off or
// the game cannot be played over the network.
extern "C" bool dreamstation_netplay_start()
{
	if (!config::GGPOEnable || NetworkHandshake::instance == nullptr) {
		g_state = -1;
		g_error = "This game cannot be played online";
		return false;
	}
	g_status = NetworkHandshake::instance->start();
	g_state = 1;
	return true;
}

// 0 off, 1 still connecting, 2 running, -1 failed (`message` says why).
extern "C" int dreamstation_netplay_poll(char* message, int size)
{
	if (g_state == 1 && g_status.valid()
			&& g_status.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
	{
		try {
			if (g_status.get())
				g_state = 2;
			else {
				g_state = -1;
				g_error = "The connection was closed";
			}
		} catch (const FlycastException& e) {
			g_state = -1;
			g_error = e.what();
		} catch (const std::exception& e) {
			g_state = -1;
			g_error = e.what();
		}
	}
	if (message != nullptr && size > 0) {
		std::strncpy(message, g_error.c_str(), size - 1);
		message[size - 1] = 0;
	}
	return g_state;
}

// Gives up connecting (or ends the session).
extern "C" void dreamstation_netplay_cancel()
{
	if (NetworkHandshake::instance != nullptr)
		NetworkHandshake::instance->stop();
	if (g_status.valid()) {
		try {
			g_status.get();
		} catch (...) {
		}
	}
	g_state = 0;
}

// The DNS server Dreamcast games use to find their servers online.
extern "C" void dreamstation_set_dns(const char* server)
{
	config::DNS.set(server != nullptr && *server ? server : "dns.flyca.st");
}
