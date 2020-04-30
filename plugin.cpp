#include "plugin.hpp"

hidden inline std::string read(std::string filename){
	std::ifstream f(filename);
	return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

hidden bool exists(std::string filename) {
	return std::ifstream(filename).is_open();
}

hidden std::string getTimeStr() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[128];

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
	return std::string(buffer);
}

hidden std::string getContentType(std::string filename) {
	static std::map<std::string, std::string> contentTypes = {
		{"aac", "audio/aac"},
		{"css", "text/css"},
		{"htm", "text/html"},
		{"html", "text/html"},
		{"jpeg", "image/jpeg"},
		{"jpg", "image/jpeg"},
		{"js", "text/javascript"},
		{"json", "application/json"},
		{"png", "image/png"},
		{"svg", "image/svg+xml"},
		{"txt", "text/plain"},
		{"xml", "text/xml"},
	};
	unsigned pos = filename.rfind('.');
	std::string ending(filename.begin() + (pos < filename.length() ? pos+1 : 0), filename.end());
	return contentTypes[ending];
}

hidden int getUserId(std::string username) {
	std::vector<std::array<sqlite::variant, 1>> result = db->exec<1>("select id from users where name like ?1;", {username});
	int userId = -1;
	if(result.size() && result[0][0].index()) {
		userId = std::get<int>(result[0][0]);
	}
	spdlog::info("getUserId(username: \'{}\') -> {}", username, userId);
	return userId;
}

hidden bool isUserValid(std::string username, std::string password) {
	std::vector<std::array<sqlite::variant, 1>> result = db->exec<1>("select password from users where name like ?1;", {username});
	bool valid = false;
	if(result.size() == 1 && result[0][0].index()){
		valid = (std::get<std::string>(result[0][0]) == password);
	}
	spdlog::info("isUserValid(username: \'{}\', password: \'{}\') -> {}", username, password, valid ? "true" : "false");
	return valid;
}

hidden bool isSessionValid(std::string sid, std::string username = "") {
	bool r = false;
	if(sid.length() == 64) {
		std::vector<std::array<sqlite::variant, 1>> result = username.length() ?
				db->exec<1>("select session from users where sid like ?1 and name == ?2;", {sid, username}) :
				db->exec<1>("select session from users where sid like ?1;", {sid});
		if(result.size() == 1 && result[0][0].index()){
			std::vector<std::string> session = split(std::get<std::string>(result[0][0]), '|');
			if(session.size() == 5) {
				int start = atoi(session[2].c_str());
				int timeout = atoi(session[3].c_str());
				if((start + timeout) < clock() && timeout > 0) {//session timed out
					db->exec("update users set sid = \'\', session = \'\' where name like ?1;", {session[0]});
				}
				else {
					r = true;
				}
			}
		}
	}
	spdlog::info("isSessionValid(sid: \'{}\', username: \'{}\') -> {}", sid, username, r ? "true" : "false");
	return r;
}

hidden std::string createSession(std::string username, std::string password, int timeout = 86400) {	//default timeout: 1 day
	std::string sid = "";
	if(isUserValid(username, password)) {
		sid = std::get<std::string>(db->exec<1>("select sid from users where name like ?1;", {username})[0][0]);
		if(!isSessionValid(sid)) {
			std::string key = join({username, getTimeStr(), std::to_string(clock()), std::to_string(timeout*CLOCKS_PER_SEC), std::to_string(rand())}, '|');
			sid = crypt::sha256::hash(key);
			int changes = 0;
			db->exec("update users set sid = ?1, session = ?2 where name like ?3;", {sid, key, username}, &changes);
			if(!changes) {
				sid = "";
			}
		}
	}
	spdlog::info("createSession(username: \'{}\', password: \'{}\') -> {}", username, password, sid);
	return sid;
}

///////////////////////////////////////////////
/////////////exported functions////////////////
///////////////////////////////////////////////

export json getStats(json request, Server *server, tcpsocket*) {
	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", HttpStatus_OK},
		{"header", json::object{
			{"Content-Type", "application/json"}
		}},
		{"data", crypt::vigenere(json(json::object{
			{"/proc/stat", read("/proc/stat")},
			{"/proc/uptime", read("/proc/uptime")},
			{"/proc/meminfo", read("/proc/meminfo")},
			{"/proc/cpuinfo", read("/proc/cpuinfo")},
			{"/proc/self/status", read("/proc/self/status")}
		}).print(), crypt::sha256::hash(server->getPassPhrase() + request["url"].toString()), crypt::encrypt)}
	};
}

export json getFile(json request, Server *server, tcpsocket *client) {
	std::string url = std::string("www") + request["url"].toString();
	if(url == "www/")
		url += "index.html";
	spdlog::info("getFile(\'{}\')", url);
	std::string encoding = "identity";
	if(request["header"]["accept-encoding"].isString()) {
		std::vector<std::string> acceptEncodings = split(removeAll(request["header"]["accept-encoding"].toString(), " "), ',');
		if(std::find(acceptEncodings.begin(), acceptEncodings.end(), "br") != acceptEncodings.end() && exists(url + ".br")) {
			url += ".br";
			encoding = "br";
		}
		else if(std::find(acceptEncodings.begin(), acceptEncodings.end(), "gzip") != acceptEncodings.end() && exists(url + ".gz")) {
			url += ".gz";
			encoding = "gzip";
		}
	}

	std::ifstream f(url, std::ios::binary | std::ios::ate);
	if(f.is_open()) {
		size_t size = f.tellg();
		f.seekg(std::ios::beg);
		if(size < max_chunk_size) {
			return json::object{
				{"request", request},
				{"version", "HTTP/1.1"},
				{"status", HttpStatus_OK},
				{"header", json::object{
					{"Content-Encoding", encoding},
					{"Content-Type", getContentType(url)}
				}},
				{"data", std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>())}
			};
		}
		else{	//the client wants a really big file so it is sent in chunks
			client->send(server->responseToString(json::object{
					{"request", request},
					{"version", "HTTP/1.1"},
					{"status", HttpStatus_OK},
					{"header", json::object{
						{"Content-Encoding", encoding},
						{"Content-Type", getContentType(url)},
						{"Content-Length", std::to_string(size)}
					}}
				}, false) + "\n");
			size_t sent = 0;
			std::string buffer(max_chunk_size, 0);
			while(sent < size && client->isConnected()){
				f.read(&buffer[0], max_chunk_size);
				client->send(buffer);
				sent += max_chunk_size;
			}
			return json();
		}
	}
	else {
		return json::object{
			{"request", request},
			{"version", "HTTP/1.1"},
			{"status", HttpStatus_NotFound},
			{"header", json::object{
				{"Content-Type", getContentType(url)}
			}},
			{"data", read("./www/index.html")}
		};
	}
}

export json getComments(json request, Server*, tcpsocket*){
	std::string url = request["url"].toString();
	std::string site = request["parameters"]["site"].isString() ? request["parameters"]["site"].toString() : "";
	std::string name = request["parameters"]["name"].isString() ? request["parameters"]["name"].toString() : "";
	int count = request["parameters"]["count"].isString() ? atoi(request["parameters"]["count"].toString().c_str()) : INT_MAX;
	int userId = getUserId(name);

	static sqlite::stmt stmt("select id,headline,content,author,votes,url,date,"
						"(length(votes)-length(replace(votes, \",\", \"\"))) + min(1, min(length(votes),1)) as count "
						"from comments where url like ?1 order by count desc limit ?2;", db, {"", 0});
	stmt.setArgs({site + '%', int(count)});	//resets the statement to be re-executed and updates the arguments

	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", 200},
		{"header", json::object{
			{"Content-Type", getContentType("json")}
		}},
		{"data", [&userId, &site, &name, &count]() -> json {
			json::array comments;
			while(stmt.status() == SQLITE_ROW){
				std::array<sqlite::variant, 8> line = stmt.next<8>();
				if(line[0].index() && line[1].index() && line[2].index() && line[3].index() && 
				   line[4].index() && line[5].index() && line[6].index() && line[7].index()) {
					std::vector<std::string> votes = split(std::get<std::string>(line[4]), ',');
					comments.push_back(json::object{
						{"id", std::get<int>(line[0])},
						{"headline", std::get<std::string>(line[1])},
						{"content", std::get<std::string>(line[2])},
						{"author", std::get<std::string>(line[3])},
						{"date", std::get<std::string>(line[6])},
						{"likes", std::get<int>(line[7])},
						{"voted", std::find(votes.begin(), votes.end(), std::to_string(userId)) != votes.end()},
						{"url", std::get<std::string>(line[5])}
					});
				}
			}
			spdlog::info("getComments(site: \'{}\', name: \'{}\', max: {}) -> {}", site, name, count, json(comments).print(json::minified));
			return comments;
		} ().print(json::minified)}
	};
}

export json getTopSites(json request, Server*, tcpsocket*){
	std::string url = request["url"].toString();
	std::string pattern = request["parameters"]["url"].isString() ? request["parameters"]["url"].toString() : "%";
	int count = request["parameters"]["count"].isString() ? atoi(request["parameters"]["count"].toString().c_str()) : 5;

	static sqlite::stmt stmt("select count(id) as n,url from comments where url like ?1 "
							 "group by url order by n desc limit ?2;", db, {"", 0});
	stmt.setArgs({pattern, count});	//resets the statement to be reexecuted and updates the arguments

	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", 200},
		{"header", json::object{
			{"Content-Type", getContentType("json")}
		}},
		{"data", [&pattern, &count]() -> json {
			json::array sites = {};
			while(stmt.status() == SQLITE_ROW){
				std::array<sqlite::variant, 2> line = stmt.next<2>();
				if(line[0].index() && line[1].index()){
					sites.push_back(json::object{
						{"url", std::get<std::string>(line[1])},
						{"comments", std::get<int>(line[0])}
					});
				}
			}
			spdlog::info("getSites(pattern: \'{}\', max: {}) -> {}", pattern, count, json(sites).print(json::minified));
			return sites;
		} ().print(json::minified)}
	};
}

export json postComment(json request, Server*, tcpsocket *client){
	std::string payload = client->recv(atoi(request["header"]["content-length"].toString().c_str()));
	json data = json::parse(payload);
	std::string username = data["username"].toString();
	std::string password = data["password"].toString();
	std::string sid = request["cookies"]["sid"].isString() ? request["cookies"]["sid"].toString() : data["sid"].toString();
	std::string url = data["url"].toString();
	std::string headline = data["headline"].toString();
	std::string content = data["content"].toString();

	spdlog::info("postComment(url: \'{}\', username: \'{}\', headline: \'{}\', content: \'{}\')", url, username, headline, content);

	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", [&username, &password, &sid, &headline, &content, &url]() -> int {
			if(isUserValid(username, password) || isSessionValid(sid, username)){
				int changes = 0;
				db->exec("insert into comments (headline,content,author,url,votes,date) values(?1, ?2, ?3, ?4, \'\', datetime(\'now\'));", 
								{headline, content, username, url}, &changes);
				if(changes > 0) {
					return HttpStatus_OK;
				}
				return HttpStatus_InternalServerError;
			}
			return HttpStatus_Forbidden;
		}()},
		{"header", json::array{
			{"Content-Type", getContentType("json")}
		}}
	};
}

export json voteComment(json request, Server*, tcpsocket *client){
	std::string payload = client->recv(atoi(request["header"]["content-length"].toString().c_str()));
	json data = json::parse(payload);
	std::string username = data["username"].toString();
	std::string password = data["password"].toString();
	std::string sid = request["cookies"]["sid"].isString() ? request["cookies"]["sid"].toString() : data["sid"].toString();

	int id = data["id"].toInt();
	bool vote = data["vote"].toBool();
	std::string uid = std::to_string(getUserId(username));

	spdlog::info("voteComment(id: {}, username: \'{}\', upvote: {})", id, username, vote ? "true" : "false");
	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", [&username, &password, &sid, &id, &vote, &uid]() -> int {
			if(isUserValid(username, password) || isSessionValid(sid, username)) {
				std::vector<std::array<sqlite::variant, 1>> result = db->exec<1>("select votes from comments where id like ?1;", {id});
				if(result.size()) {
					std::vector<std::string> votes = split(std::get<std::string>(result[0][0]), ',');
					std::vector<std::string>::iterator loc = std::find(votes.begin(), votes.end(), uid);
					if(vote) {
						if(loc == votes.end()) {
							votes.push_back(uid);
						}
						else {
							return HttpStatus_Conflict;
						}
					}
					else {
						if(loc != votes.end()) {
							votes.erase(loc);
						}
						else {
							return HttpStatus_BadRequest;
						}
					}
					int changes = 0;
					db->exec("update comments set votes = ?1 where id like ?2;", {join(votes, ','), id}, &changes);
					return changes ? HttpStatus_OK : HttpStatus_InternalServerError;
				}
				return HttpStatus_NotFound;
			}
			return HttpStatus_Unauthorized;
		}()},
		{"header", json::object{
			{"Content-Type", getContentType("json")}
		}}
	};
}

export json signup(json request, Server*, tcpsocket *client){
	std::string payload = client->recv(request["header"]["content-length"].toInt());
	json data = json::parse(payload);

	std::string username = data["username"].toString();
	std::string password = data["password"].toString();
	std::string email = data["email"].toString();

	spdlog::info("signup(username: \'{}\', password: \'{}\', email: \'{}\')", username, password, email);

	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", [&username, &password, &email]() -> int {
			if(db->exec<1>("select id from users where name like ?1;", {username}).size() == 0){
				int changes = 0;
				db->exec("insert into users (name, password, email, session, sid) values(?1, ?2, ?3, \'\', \'\');", 
						{username, password, email}, &changes);
				return changes ? HttpStatus_OK : HttpStatus_InternalServerError;
			}
			return HttpStatus_Forbidden;
		}()},
		{"header", json::object{
			{"Content-Type", getContentType("json")}
		}}
	};
}

export json signin(json request, Server*, tcpsocket*){
	std::string username = request["parameters"]["username"].isString() ? request["parameters"]["username"].toString() : "";
	std::string password = request["parameters"]["password"].isString() ? request["parameters"]["password"].toString() : "";
	int timeout = request["parameters"]["timeout"].isString() ? atoi(request["parameters"]["timeout"].toString().c_str()) : 86400;

	spdlog::info("signin(username: \'{}\', password: \'{}\', timeout: {}", username, password, timeout);

	std::string sid = createSession(username, password, timeout);
	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", (sid != "" ? HttpStatus_OK : HttpStatus_Forbidden)},
		{"header", json::object{
			{"Set-Cookie", "sid=" + sid + ";PATH=/"},
			{"Content-Type", getContentType("json")}
		}},
		{"data", json(json::object{{"sid", sid}}).print()}
	};
}

export json checksid(json request, Server*, tcpsocket*){
	std::string sid = request["parameters"]["sid"].isString() ? request["parameters"]["sid"].toString() :
						(request["cookies"]["sid"].isString() ? request["cookies"]["sid"].toString() : "");
	spdlog::info("checksid(sid: \'{}\')", sid);
	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", isSessionValid(sid) ? HttpStatus_OK : HttpStatus_Unauthorized}
	};
}

export json signout(json request, Server*, tcpsocket*){
	std::string sid = request["parameters"]["sid"].isString() ? request["parameters"]["sid"].toString() :
						(request["cookies"]["sid"].isString() ? request["cookies"]["sid"].toString() : "");
	db->exec("update users set sid = \'\', session = \'\' where sid like ?1;", {sid});
	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", 200},
		{"header", json::object{
			{"Set-Cookie", "sid=; PATH=/"},
			{"Content-Type", getContentType("json")}
		}}
	};
}

export json getUserData(json request, Server*, tcpsocket*){
	std::string username = request["parameters"]["username"].isString() ? request["parameters"]["username"].toString() : "";
	std::string password = request["parameters"]["password"].isString() ? request["parameters"]["password"].toString() : "";
	std::string sid = request["parameters"]["sid"].isString() ? request["parameters"]["sid"].toString() :
						(request["cookies"]["sid"].isString() ? request["cookies"]["sid"].toString() : "");
	json data;
	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", [&data, &username, &password, &sid]() -> int {
			if(isSessionValid(sid, username) || isUserValid(username, password)){
				std::vector<std::array<sqlite::variant, 3>> result = db->exec<3>(
					"select name, email, length(sid) = 1 as session from users where sid = ?1 or name = ?2", {sid, username});
				data = json::object{
					{"username", std::get<std::string>(result[0][0])},
					{"email", std::get<std::string>(result[0][1])},
					{"session", !bool(std::get<int>(result[0][2]))}
				};
				return HttpStatus_OK;
			}
			return HttpStatus_Unauthorized;
		}()},
		{"header", json::object{
			{"Content-Type", getContentType("json")}
		}},
		{"data", data.print(json::minified)}
	};
}

export json changeUserData(json request, Server*, tcpsocket *client) {
	std::string payload = client->recv(request["header"]["content-length"].toInt());
	json data = json::parse(payload);

	std::string username = data["username"].toString();
	std::string password = data["password"].toString();

	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", [&username, &password, &data]() -> int {
			if(isUserValid(username, password)) {
				int changes = 0;
				std::string querry = "";
				std::vector<sqlite::variant> args = {};
				if(data["email"].isString()) {
					if(data["new-password"].isString()) {
						querry = "update users set email = ?1, password = ?2 where name == ?3";
						args = {data["email"].toString(), data["new-password"].toString(), username};
					}
					else {
						querry = "update users set email = ?1 where name == ?2";
						args = {data["email"].toString(), username};
					}
				}
				else if(data["new-password"].isString()) {
					querry = "update users set password = ?1 where name == ?2";
					args = {data["new-password"].toString(), username};
				}
				db->exec(querry, args, &changes);
				return changes ? HttpStatus_OK : HttpStatus_InternalServerError;
			}
			return HttpStatus_Unauthorized;
		}()}
	};
}

export json optionsAlwaysOK(json request, Server*, tcpsocket*){
	return json::object{
		{"request", request},
		{"version", "HTTP/1.1"},
		{"status", 200}
	};
}

///////////////////////////////////////////////
///////////////constructor/////////////////////
///////////////////////////////////////////////

void __attribute__((constructor)) initPlugin(){
	db = new sqlite::database("main.db3");
// start db structure
	std::map<std::string, std::string> structure;
structure["comments"] = R"(CREATE TABLE "comments" (
	"id"		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	"headline"	TEXT NOT NULL,
	"content"	TEXT NOT NULL,
	"author"	TEXT NOT NULL,
	"votes"		TEXT,
	"date"		TEXT,
	"url"		TEXT
))";
structure["users"] = R"(CREATE TABLE "users" (
	"id"		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	"name"		TEXT NOT NULL UNIQUE,
	"password"	TEXT NOT NULL,
	"email"		TEXT,
	"sid"		TEXT NOT NULL,
	"session"	TEXT NOT NULL
))";
structure["resources"] = R"(CREATE TABLE "resources" (
	"id"		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	"url"		TEXT NOT NULL,
	"hash"		TEXT NOT NULL,
	"userId"	INTEGER NOT NULL
))";
structure["sqlite_sequence"] = R"(CREATE TABLE sqlite_sequence(name,seq))";
// end db structure
	std::vector<std::array<sqlite::variant, 3>> tables = db->exec<3>("select type,name,sql from sqlite_master;");
	bool reinitdb = false;
	int tablecount = 0;
	for(std::array<sqlite::variant, 3> &table : tables) {
		if(std::get<std::string>(table[0]) == "table") {
			if(structure[std::get<std::string>(table[1])] != std::get<std::string>(table[2])) {
				reinitdb = true;
			}
			tablecount++;
		}
	}
	if(reinitdb || tablecount != 4) {
		for(std::string str : split(read("init.sql"), ";")) {
			db->exec(str);
		}
	}
	db->exec("update users set sid = \'\', session = \'\' where name not like \'test\';");
}

void __attribute__((destructor)) deinitPlugin() {
	delete db;
}
