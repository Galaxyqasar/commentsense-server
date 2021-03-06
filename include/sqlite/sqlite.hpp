#pragma once

#include <vector>
#include <string>
#include <array>
#include <variant>
#include <cassert>
#include <cstring>
#include <stdexcept>

#include <sqlite/sqlite3.h>
#include <spdlog/spdlog.h>

namespace sqlite{
	typedef std::variant<std::monostate, int, double, std::string, std::vector<unsigned char>> variant;

	class database;

	class stmt{
	public:
		stmt(std::string querry, database *db, std::vector<variant> values);
		~stmt();

		void setArgs(std::vector<variant> values);
		inline void reset(){
			sqlite3_reset(handle);
			m_status = SQLITE_ROW;
		}

		template<int n = 0>
		std::array<variant, n> next(){
			m_status = sqlite3_step(handle);
			if(sqlite3_errcode(this->db) == SQLITE_ERROR) {
				spdlog::error("sqlite3 error: {}", sqlite3_errmsg(this->db));
				throw std::runtime_error("sqlite3 error!");
			}
			else if(sqlite3_errcode(this->db) == SQLITE_WARNING) {
				spdlog::warn("sqlite3 warning: {}", sqlite3_errmsg(this->db));
			}
			std::array<variant, n> row;
			if(sqlite3_column_count(handle) == n){
				for(int i = 0; i < n; i++){
					int type = sqlite3_column_type(handle, i);
					switch(type){
						case SQLITE_INTEGER:{
							row[i] = sqlite3_column_int(handle, i);
						} break;
						case SQLITE_FLOAT:{
							row[i] = sqlite3_column_double(handle, i);
						} break;
						case SQLITE_TEXT:{
							row[i] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(handle, i)));
						} break;
						case SQLITE_BLOB:{
							const void *raw = sqlite3_column_blob(handle, i);
							int size = sqlite3_column_bytes(handle, i);
							std::vector<unsigned char> data(size, '\0');
							memcpy(&data[0], raw, size);
							row[i] = data;
						} break;
						case SQLITE_NULL:{
							row[i] = std::monostate();
						} break;
						default: {
						}
					}
				}
			}
			return row;
		}

		inline int status(){
			return m_status;
		}
	private:
		sqlite3_stmt *handle = nullptr;
		sqlite3* db;
		int m_status = SQLITE_ROW;
	};

	template<>
	std::array<variant, 0> stmt::next<0>(){
		m_status = sqlite3_step(handle);
		if(sqlite3_errcode(this->db) == SQLITE_ERROR) {
			spdlog::error("sqlite3 error: {}", sqlite3_errmsg(this->db));
			throw std::runtime_error("sqlite3 error!");
		}
		else if(sqlite3_errcode(this->db) == SQLITE_WARNING) {
			spdlog::warn("sqlite3 warning: {}", sqlite3_errmsg(this->db));
		}
		return {};
	}

	class database{
	public:
		database(std::string fileName);
		~database();

		template<int n = 0>
		std::vector<std::array<variant, n>> exec(std::string querry, std::vector<variant> values = {}, int *changes = nullptr){
			stmt s(querry, this, values);
			std::vector<std::array<variant, n>> result;
			std::array<variant, n> row = s.next<n>();
			while(s.status() != SQLITE_DONE){
				result.push_back(row);
				row = s.next<n>();
			}
			if(changes)
				*changes = sqlite3_changes(handle);
			return result;
		}

	private:
		sqlite3 *handle = nullptr;
		friend class stmt;
	};

	template<>
	std::vector<std::array<variant, 0>> database::exec<0>(std::string querry, std::vector<variant> values, int *changes){
		stmt s(querry, this, values);
		do {
			s.next<0>();
		} while(s.status() != SQLITE_DONE);
		if(changes)
			*changes = sqlite3_changes(handle);
		return {};
	}
}
