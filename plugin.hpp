#pragma once

#include "server.hpp"

#include <crypt/crypt.hpp>
#include <crypt/rijndael.hpp>
#include <crypt/sha.hpp>
#include <sqlite/sqlite.hpp>

#include <ctime>
#include <limits>
#include <climits>

#define hidden __attribute__((visibility ("hidden")))
#define export extern "C" __attribute__((visibility ("default")))

using namespace inet;
#if defined(IPV6)
	using address_t = inet::ipv6address;
#else
	using address_t = inet::ipv4address;
#endif
const size_t max_chunk_size = 50*1024*1024;	//50 Mb

hidden sqlite::database *db = nullptr;