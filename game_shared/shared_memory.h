#pragma once

struct SharedMemory {
	struct {
		char login[128] = {};
		char oAuthPassword[128] = {};
	} twitchCredentials;
};