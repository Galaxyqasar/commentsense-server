#pragma once

#include "server.hpp"

#include <crypt/rijndael.hpp>
#include <crypt/sha.hpp>
#include <sqlite/sqlite.hpp>

#include <ctime>
#include <limits>
#include <climits>

#define hidden __attribute__((visibility ("hidden")))
#define export extern "C" __attribute__((visibility ("default")))

const size_t max_chunk_size = 50*1024*1024;	//50 Mb

hidden sqlite::database *db = nullptr;