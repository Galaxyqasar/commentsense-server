#include "yeetlib.hpp"
#include "server.hpp"

#define hidden __attribute__((visibility ("hidden")))
#define export extern "C" __attribute__((visibility ("default")))

const size_t max_chunk_size = 50*1024*1024;	//50 Mb
hidden sqlite::database *db;
hidden logfile *pluginlog;
hidden std::map<std::string, std::string> *contentTypes;

hidden int getUserId(std::string username) {
	std::vector<std::array<sqlite::variant, 1>> result = db->exec<1>("select id from users where name like ?1;", {username});
	int userId = -1;
	if(result.size()) {
		userId = std::get<int>(result[0][0]);
	}
	log(*pluginlog, "getUserId(", username, ") -> ", userId);
	return userId;
}

hidden std::string getContentType(std::string filename) {
	unsigned pos = filename.rfind('.');
	std::string ending(filename.begin() + (pos < filename.length() ? pos+1 : 0), filename.end());
	return (*contentTypes)[ending];
}

hidden bool isUserValid(std::string username, std::string password) {
	std::vector<std::array<sqlite::variant, 1>> result = db->exec<1>("select password from users where name like ?1;", {username});
	bool valid = false;
	if(result.size() == 1){
		valid = (std::get<std::string>(result[0][0]) == password);
	}
	log(*pluginlog, "isUserValid(", username, ", ", password, ") -> ", valid ? "true" : "false");
	return valid;
}

hidden bool isSessionValid(std::string sid) {
	if(sid.length() == 64) {
		std::vector<std::array<sqlite::variant, 1>> result = db->exec<1>("select session from users where sid like ?1;", {sid});
		if(result.size() == 1){
			std::vector<std::string> session = split(std::get<std::string>(result[0][0]), '|');
			if(session.size() == 5) {
				int start = atoi(session[2].c_str());
				int timeout = atoi(session[3].c_str());
				if((start + timeout) < clock() && timeout > 0) {//session timed out
					db->exec("update users set sid = \'\', session = \'\' where name like ?1;", {session[0]});
				}
				else {
					log(*pluginlog, "checksid(", sid, ") -> true");
					return true;
				}
			}
		}
	}
	log(*pluginlog, "checksid(", sid, ") -> false");
	return false;
}

hidden std::string createSession(std::string username, std::string password, int timeout = 86400) {	//default timeout: 1 day
	std::string sid = "";
	if(isUserValid(username, password)) {
		sid = std::get<std::string>(db->exec<1>("select sid from users where name like ?1;", {username})[0][0]);
		if(!isSessionValid(sid)) {
			std::string key = join({username, sys::getTimeStr(), std::to_string(clock()), std::to_string(timeout*CLOCKS_PER_SEC), std::to_string(rand())}, '|');
			sid = crypt::sha256::hash(key);
			int changes = 0;
			db->exec("update users set sid = ?1, session = ?2 where name like ?3;", {sid, key, username}, &changes);
			if(!changes) {
				sid = "";
			}
		}
	}
	log(*pluginlog, "createSession(", username, ", ", password, ") -> ", sid);
	return sid;
}

hidden std::string read(std::string filename){
	std::fstream f(filename);
	std::stringstream s;
	s<<f.rdbuf();
	return s.str();
}

///////////////////////////////////////////////
/////////////exported functions////////////////
///////////////////////////////////////////////

export json getStats(PluginArg arg) {
	std::string password = arg.request["url"].toStr();
	return json::object("response", {
		arg.request,
		json::string("version", "HTTP/1.1"),
		json::number("status", HttpStatus_OK),
		json::array("header", {
			json::string("Content-Type", getContentType("json"))
		}),
		json::string("data", crypt::vigenere(json::object("", {
			json::string("/proc/stat", read("/proc/stat")),
			json::string("/proc/uptime", read("/proc/uptime")),
			json::string("/proc/meminfo", read("/proc/meminfo")),
			json::string("/proc/cpuinfo", read("/proc/cpuinfo")),
			json::string("/proc/self/status", read("/proc/self/status"))
		}).toStr(), crypt::sha256::hash("fce19d339f85f986b0caaf473a471f70f923948227c8c4c0633d077c22ff235e" + password), crypt::encrypt), false)
	});
}

export json getFile(PluginArg arg) {
	std::string url = std::string("./www") + arg.request["url"].toStr();
	if(url == "./www/")
		url += "index.html";
	log(*pluginlog, "getFile(\'", url, "\')");

	std::string encoding;
	std::vector<std::string> acceptEncodings = utils::split(utils::removeAll(arg.request["headers"]["accept-encoding"].toStr(), " "), ',');
	if(std::find(acceptEncodings.begin(), acceptEncodings.end(), "br") != acceptEncodings.end() && file::exists(url + ".br")) {
		url += ".br";
		encoding = "br";
	}
	else if(std::find(acceptEncodings.begin(), acceptEncodings.end(), "gzip") != acceptEncodings.end() && file::exists(url + ".gz")) {
		url += ".gz";
		encoding = "gzip";
	}
	else {
		encoding = "identity";
	}

	file f(url);
	if(f.open("rb+")) {
		size_t size = f.size();
		if(size < max_chunk_size) {
			return json::object("response", {
				arg.request,
				json::string("version", "HTTP/1.1"),
				json::number("status", HttpStatus_OK),
				json::array("header", {
					json::string("Content-Encoding", encoding),
					json::string("Content-Type", getContentType(url))
				}),
				json::string("data", f.readAll(), false)
			});
		}
		else{	//the client wants a really big file so it is sent in chunks
			arg.client->send(arg.server->responseToString(json::object("response", {
					arg.request,
					json::string("version", "HTTP/1.1"),
					json::number("status", HttpStatus_OK),
					json::array("header", {
						json::string("Content-Encoding", encoding),
						json::string("Content-Type", getContentType(url)),
						json::string("Content-Length", std::to_string(size))
					})
				}), false) + "\n");
			size_t sent = 0;
			while(sent < size && arg.client->isConnected()){
				arg.client->send(f.read(max_chunk_size));
				sent += max_chunk_size;
			}
			return json::null();
		}
	}
	else {
		return json::object("response", {
			arg.request,
			json::string("version", "HTTP/1.1"),
			json::number("status", HttpStatus_NotFound),
			json::array("header", {
				json::string("Content-Type", getContentType(url))
			}),
			json::string("data", file::readAll("./data/index.html"))
		});
	}
}

export json getComments(PluginArg arg){
	std::string url = arg.request["url"].toStr();
	std::string site, name;
	int count = INT_MAX;
	if(url.length() > 14){
		for(std::string str : split(std::string(url.begin() + 14, url.end()), '&')){
			if(str.find("site=") == 0 && str.length() > 5){
				site = std::string(str.begin() + 5, str.end());
			}
			else if(str.find("username=") == 0 && str.length() > 9){
				name = std::string(str.begin() + 9, str.end());
			}
			else if(str.find("count=") == 0 && str.length() > 6){
				count = atoi(std::string(str.begin() + 6, str.end()).c_str());
			}
		}
	}
	log(*pluginlog, "getComments(\'", site, "\', \'", name, "\', ", count, ")");
	json comments = json::array("comments", {});
	int userId = getUserId(name);	
	site.push_back('%');
	static sqlite::stmt stmt("select id,headline,content,author,votes,url,date,"
						"(length(votes)-length(replace(votes, \",\", \"\"))) + min(1, min(length(votes),1)) as count "
						"from comments where url like ?1 order by count desc limit ?2;", db, {site, count});
	stmt.setArgs({site, int(count)});	//resets the statement to be re-executed and updates the arguments
	
	while(stmt.status() == SQLITE_ROW){
		std::array<sqlite::variant, 8> line = stmt.next<8>();
		if(line[0].index() && line[1].index() && line[2].index() && line[3].index() && line[4].index() && line[5].index() && line[6].index()) {
			std::vector<std::string> votes = split(std::get<std::string>(line[4]), ',');
			comments += json::object("", {
				json::number("id", std::get<int>(line[0])),
				json::string("headline", std::get<std::string>(line[1])),
				json::string("content", std::get<std::string>(line[2])),
				json::string("author", std::get<std::string>(line[3])),
				json::string("date", std::get<std::string>(line[6])),
				json::number("likes", std::get<int>(line[7])),
				json::number("voted", std::find(votes.begin(), votes.end(), std::to_string(userId)) != votes.end()),
				json::string("url", std::get<std::string>(line[5]))
			});
		}
	}
	return json::object("response", {
		arg.request,
		json::string("version", "HTTP/1.1"),
		json::number("status", 200),
		json::array("header", {
			json::string("Content-Type", getContentType("json"))
		}),
		json::string("data", json::object("", {comments}).toStr())
	});
}

export json getTopSites(PluginArg arg){
	int count = 5;
	std::string url = arg.request["url"].toStr();
	if(url.length() > 11){
		for(std::string str : split(std::string(url.begin() + 11, url.end()), '&')){
			if(str.find("count=") == 0 && str.length() > 6){
				count = atoi(std::string(str.begin() + 6, str.end()).c_str());
			}
		}
	}
	static sqlite::stmt stmt("select count(id) as n,url from comments group by url order by n desc limit ?1;", db, {count});
	stmt.setArgs({count});	//resets the statement to be reexecuted and updates the arguments
	json sites = json::array("sites", {});
	while(stmt.status() == SQLITE_ROW){
		std::array<sqlite::variant, 2> line = stmt.next<2>();
		if(line[0].index() && line[1].index()){
			sites += json::object("", {
				json::string("url", std::get<std::string>(line[1])),
				json::number("comments", std::get<int>(line[0]))
			});
		}
	}
	return json::object("response", {
		arg.request,
		json::string("version", "HTTP/1.1"),
		json::number("status", 200),
		json::array("header", {
			json::string("Content-Type", getContentType("json"))
		}),
		json::string("data", json::object("", {sites}).toStr())
	});
}

export json postComment(PluginArg arg){
	std::string cookie = arg.request["header"]["cookie"].toStr();
	std::string payload = arg.client->recv(arg.request["header"]["content-length"].toInt());
	json data = json::parse(payload);
	std::string username = data["username"].toStr();
	std::string password = data["password"].toStr();
	std::string sid = data["sid"].toStr();
	std::string url = data["url"].toStr();
	std::string headline = data["headline"].toStr();
	std::string content = data["content"].toStr();

	if(!data["sid"].isStr() && cookie.find("sid=") == 0){
		sid = split(std::string(cookie.begin() + 4, cookie.end()), ';')[0];
	}

	log(*pluginlog, "postComment(", username, ", ", password, ", ", headline, ")");

	if(isUserValid(username, password) || isSessionValid(sid)){
		int changes = 0;
		db->exec("insert into comments (headline,content,author,url,votes,date) values(?1, ?2, ?3, ?4, \'\', datetime(\'now\'));", {
								headline, content, username, url}, &changes);
		if(changes > 0){
			return json::object("response", {
				arg.request,
				json::string("version", "HTTP/1.1"),
				json::number("status", HttpStatus_OK),
				json::array("header", {
					json::string("Content-Type", getContentType("json"))
				}),
				json::string("data", 
					json::object("", {
						json::string("status", "success: comment posted")
					}).toStr()
				)
			});
		}
		return json::object("response", {
			arg.request,
			json::string("version", "HTTP/1.1"),
			json::number("status", HttpStatus_InternalServerError),
			json::array("header", {
				json::string("Content-Type", getContentType("json"))
			}),
			json::string("data", 
				json::object("", {
					json::string("status", "error: sql error")
				}).toStr()
			)
		});
	}
	return json::object("response", {
		arg.request,
		json::string("version", "HTTP/1.1"),
		json::number("status", HttpStatus_Forbidden),
		json::array("header", {
			json::string("Content-Type", getContentType("json"))
		}),
		json::string("data", 
			json::object("", {
				json::string("status", "error: (wrong username or password) or (sid invalid)")
			}).toStr()
		)
	});
}

export json voteComment(PluginArg arg){
	std::string cookie = arg.request["header"]["cookie"].toStr();
	std::string payload = arg.client->recv(arg.request["header"]["content-length"].toInt());
	json data = json::parse(payload);

	std::string username = data["username"].toStr();
	std::string password = data["password"].toStr();
	std::string sid = data["sid"].toStr();

	int id = data["id"].toInt();
	bool vote = data["vote"].toInt();
	std::string uid = std::to_string(getUserId(username));

	if(!data["sid"].isStr() && cookie.find("sid=") == 0) {
		sid = split(std::string(cookie.begin() + 4, cookie.end()), ';')[0];
	}

	log(*pluginlog, "voteComment(", username, ", ", password, ", ", vote ? "true" : "false", ")");

	int status = HttpStatus_Forbidden;
	std::string statusstr = "error : password or sid wrong";

	if(isUserValid(username, password) || isSessionValid(sid)) {
		std::vector<std::array<sqlite::variant, 1>> result = db->exec<1>("select votes from comments where id like ?1;", {id});
		if(result.size()) {
			std::vector<std::string> votes = split(std::get<std::string>(result[0][0]), ',');
			std::vector<std::string>::iterator loc = std::find(votes.begin(), votes.end(), uid);
			if(vote) {
				if(loc == votes.end()) {
					votes.push_back(uid);
				}
				else {
					status = HttpStatus_BadRequest;
					statusstr = "error: already voted";
				}
			}
			else {
				if(loc != votes.end()) {
					votes.erase(loc);
				}
				else {
					status = HttpStatus_BadRequest;
					statusstr = "error: you need to vote before you can unvote";
				}
			}
			if(status != HttpStatus_BadRequest) {
				int changes = 0;
				db->exec("update comments set votes = ?1 where id like ?2;", {join(votes, ','), id}, &changes);
				if(changes) {
					status = HttpStatus_OK;
					statusstr = "success!";
				}
				else {
					status = HttpStatus_InternalServerError;
					statusstr = "error: sql error";
				}
			}
		}
		else {
			status = HttpStatus_BadRequest;
			statusstr = "error : this comment doesn't exist";			
		}
	}
	return json::object("response", {
		arg.request,
		json::string("version", "HTTP/1.1"),
		json::number("status", status),
		json::array("header", {
			json::string("Content-Type", getContentType("json"))
		}),
		json::string("data", json::object("", {json::string("status", statusstr)}).toStr())
	});
}

export json signup(PluginArg arg){
	std::string payload = arg.client->recv(arg.request["header"]["content-length"].toInt());
	json data = json::parse(payload);

	std::string username = data["username"].toStr();
	std::string password = data["password"].toStr();
	std::string email = data["email"].toStr();

	log(*pluginlog, "signup(", username, ", ", password, ")");

	int status = 0;
	std::string statusstr;

	if(db->exec<1>("select id from users where name like ?1;", {username}).size() == 0){
		int changes = 0;
		db->exec("insert into users (name, password, email, session, sid) values(?1, ?2, ?3, \'\', \'\');", {username, password, email}, &changes);
		if(changes){
			status = HttpStatus_OK;
			statusstr = "successfully created user";
		}
		else{
			status = HttpStatus_InternalServerError;
			statusstr = "error: sql error";
		}
	}
	else{
		status = HttpStatus_Forbidden;
		statusstr = "error: user already exists";
	}
	return json::object("response", {
		arg.request,
		json::string("version", "HTTP/1.1"),
		json::number("status", status),
		json::array("header", {
			json::string("Content-Type", getContentType("json"))
		}),
		json::string("data", json::object("", {json::string("status", statusstr)}).toStr())
	});
}

export json signin(PluginArg arg){
	std::string url = arg.request["url"].toStr();
	std::string username, password;
	int timeout = 86400;	// 1 day
	if(url.length() > 11){
		for(std::string str : split(std::string(url.begin() + 12, url.end()), '&')){
			if(str.find("username=") == 0 && str.length() > 9){
				username = std::string(str.begin() + 9, str.end());
			}
			else if(str.find("password=") == 0 && str.length() > 9){
				password = std::string(str.begin() + 9, str.end());
			}
			else if(str.find("timeout=") == 0 && str.length() > 8){
				timeout = std::stoi(std::string(str.begin() + 8, str.end()));
			}
		}
	}

	log(*pluginlog, "signin(", username, ", ", password, ")");

	std::string sid = createSession(username, password, timeout);
	return json::object("response", {
		arg.request,
		json::string("version", "HTTP/1.1"),
		json::number("status", (sid != "" ? HttpStatus_OK : HttpStatus_Forbidden)),
		json::array("header", {
			json::string("Set-Cookie", "sid=" + sid + ";PATH=/"),
			json::string("Content-Type", getContentType("json"))
		}),
		json::string("data", json::object("", {json::string("sid", sid)}).toStr())
	});
}

export json checksid(PluginArg arg){
	std::string url = arg.request["url"].toStr();
	std::string cookie = arg.request["header"]["cookie"].toStr();

	std::string args = std::string(url.begin() + min(url.find('?')+1, url.length()-1), url.end());
	std::string sid = "0";
	log(*pluginlog, "checksid(", cookie, ", ", args, ")");

	if(args.find("sid=") == 0 && args.length() > 4) {
		sid = std::string(args.begin() + 4, args.end());
	}
	else if(cookie.find("sid=") == 0 && cookie.length() > 4) {
		sid = split(std::string(cookie.begin() + 4, cookie.end()), ';')[0];
	}
	log(*pluginlog, "checksid(", sid, ")");
	bool valid = isSessionValid(sid);
	log(*pluginlog, "checksid(", sid, ") -> ", valid ? "true" : "false");
	return json::object("response", {
		arg.request,
		json::string("version", "HTTP/1.1"),
		json::number("status", valid ? HttpStatus_OK : HttpStatus_Unauthorized)
	});
}

export json signout(PluginArg arg){
	std::string url = arg.request["url"].toStr();
	std::string cookie = arg.request["header"]["cookie"].toStr();

	std::string args = std::string(url.begin() + url.find('?'), url.end());
	log(*pluginlog, "signout(", args, ", ", cookie, ")");

	if(cookie.find("sid=") == 0){
		std::string sid = split(std::string(cookie.begin() + 4, cookie.end()), ';')[0];
		db->exec("update users set sid = \'\', session = \'\' where sid like ?1;", {sid});
	}
	else{
		if(args.find("?name=") == 0 && args.length() > 6){
			std::string name = std::string(args.begin() + 6, args.end());
			db->exec("update users set sid = \'\', session = \'\' where name like ?1;", {name});
		}
		else if(args.find("?sid=") == 0 && args.length() > 5){
			std::string sid = std::string(args.begin() + 5, args.end());
			db->exec("update users set sid = \'\', session = \'\' where sid like ?1;", {sid});
		}
	}
	return json::object("response", {
		arg.request,
		json::string("version", "HTTP/1.1"),
		json::number("status", 200)
	});
}

export json getUserData(PluginArg arg){
	std::string url = arg.request["url"].toStr();
	std::string cookie = arg.request["header"]["cookie"].toStr();
	std::string sid = "", username = "", password = "";

	for(std::string str : split(std::string(url.begin() + 10, url.end()), '&')){
		if(str.find("username=") == 0 && str.length() > 9){
			username = std::string(str.begin() + 9, str.end());
		}
		else if(str.find("password=") == 0 && str.length() > 9){
			password = std::string(str.begin() + 9, str.end());
		}
		else if(str.find("sid=") == 0 && str.length() > 4){
			sid = std::string(str.begin() + 4, str.end());
		}
	}
	if(!sid.length() && cookie.find("sid=") == 0 && cookie.length() > 4){
		sid = split(std::string(cookie.begin() + 4, cookie.end()), ';')[0];
	}
	json data;
	int status;
	if(isUserValid(username, password) || isSessionValid(sid)){
		std::vector<std::array<sqlite::variant, 2>> result = db->exec<2>("select email, sid != \'\' as session from users where name == ?1 or sid == ?2", {username, sid});
		data = json::object("", {
			json::string("email", std::get<std::string>(result[0][0])),
			json::boolean("session", std::get<int>(result[0][1]))
		});
		status = HttpStatus_OK;
	}
	else{
		data = json::object("", {
			json::string("status", "error: log-in data invalid")
		});
		status = HttpStatus_Unauthorized;
	}
	return json::object("response", {
		arg.request,
		json::string("version", "HTTP/1.1"),
		json::number("status", status),
		json::array("header", {
			json::string("Content-Type", getContentType("json"))
		}),
		json::string("data", data.print(json::minified))
	});
}

export json changeUserData(PluginArg arg) {
	std::string payload = arg.client->recv(arg.request["header"]["content-length"].toInt());
	json data = json::parse(payload);

	std::string username = data["username"].toStr();
	std::string password = data["password"].toStr();
	std::string newEmail = data["email"].toStr();
	std::string newPassword = data["new-password"].toStr();

	if(isUserValid(username, password)) {
		int changes = 0;
		std::string querry;
		std::vector<sqlite::variant> args;
		if(data["email"].isStr()) {
			if(data["new-password"].isStr()) {
				querry = "update users set email = ?1, password = ?2 where name == ?3";
				args = {newEmail, newPassword, username};
			}
			else {
				querry = "update users set email = ?1 where name == ?2";
				args = {newEmail, username};
			}
		}
		else if(data["new-password"].isStr()) {
			querry = "update users set password = ?1 where name == ?2";
			args = {newPassword, username};
		}
		else {
			querry = "";
			args = {};
		}
		db->exec(querry, args, &changes);
		if(changes){
			return json::object("response", {
				arg.request,
				json::string("version", "HTTP/1.1"),
				json::number("status", HttpStatus_OK),
				json::array("header", {
					json::string("Content-Type", getContentType("json"))
				}),
				json::string("data", json::object("", {json::string("status", "success!")}).toStr())
			});
		}
		else{
			return json::object("response", {
				arg.request,
				json::string("version", "HTTP/1.1"),
				json::number("status", HttpStatus_Unauthorized),
				json::array("header", {
					json::string("Content-Type", getContentType("json"))
				}),
				json::string("data", json::object("", {json::string("status", "error: sql error")}).toStr())
			});
		}
	}
	else{
		return json::object("response", {
			arg.request,
			json::string("version", "HTTP/1.1"),
			json::number("status", HttpStatus_Unauthorized),
			json::array("header", {
				json::string("Content-Type", getContentType("json"))
			}),
			json::string("data", json::object("", {json::string("status", "error: log-in data invalid")}).toStr())
		});
	}
}

export json optionsAlwaysOK(PluginArg arg){
	return json::object("response", {
		arg.request,
		json::string("version", "HTTP/1.1"),
		json::number("status", 200)
	});
}

///////////////////////////////////////////////
///////////////constructor/////////////////////
///////////////////////////////////////////////

hidden void initdb() {
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
			std::cout<<std::get<std::string>(table[0])<<" | "<<std::get<std::string>(table[1])<<" | "<<std::get<std::string>(table[2])<<" | "<<bool(structure[std::get<std::string>(table[1])] != std::get<std::string>(table[2]))<<"\n";
			if(structure[std::get<std::string>(table[1])] != std::get<std::string>(table[2])) {
				reinitdb = true;
			}
			tablecount++;
		}
	}
	if(reinitdb || tablecount != 4) {
		for(std::string str : utils::split(utils::file::readAll("init.sql"), ";")) {
			db->exec(str);
		}
	}
	db->exec("update users set sid = \'\', session = \'\' where name not like \'test\';");
}

void __attribute__((constructor)) initPlugin(){
	pluginlog = new logfile("./log_plugin.txt");
	contentTypes = new std::map<std::string, std::string>();
	(*contentTypes)["aac"] = "audio/aac";
	(*contentTypes)["htm"] = "text/html";
	(*contentTypes)["html"] = "text/html";
	(*contentTypes)["jpeg"] = "image/jpeg";
	(*contentTypes)["jpg"] = "image/jpeg";
	(*contentTypes)["js"] = "text/javascript";
	(*contentTypes)["json"] = "application/json";
	(*contentTypes)["png"] = "image/png";
	(*contentTypes)["svg"] = "image/svg+xml";
	(*contentTypes)["txt"] = "text/plain";
	(*contentTypes)["xml"] = "text/xml";
	initdb();
	log(*pluginlog, "plugin \"comment sense\" initialized");
}

void __attribute__((destructor)) deinitPlugin() {
	delete pluginlog;
	delete contentTypes;
	delete db;
}