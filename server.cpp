#include "server.hpp"

Server::Server(network::address addr, unsigned short port, std::string passPhrase) : port(port), passPhrase(passPhrase){
	socket = new network::tcpsocket(AF_INET, SOCK_STREAM, 0);
	socket->bind(addr, port);

	registerPlugin({ANY - GET, "default", "/", [](json request, Server*, tcpsocket*){
		spdlog::error("no plugin found");
		return json::object{
			{"request", request},
			{"version", "HTTP/1.1"},
			{"status", 501},
			{"header", json::object{}},
			{"data", request.print()}
		};
	}});
}

Server::~Server(){
	delete socket;
}

void Server::start(){
	socket->listen();
	while(!quit){
		tcpsocket *client = socket->accept({5,0});
		std::thread handler([this, &client]() {
			try {
				handleClient(client);
			} catch(std::exception &e) {
				spdlog::error("exception while handling client: \'{}\'", e.what());
			}
		});
		handler.detach();
	}
}

void Server::stop(){
	quit = true;
	tcpsocket s(AF_INET, SOCK_STREAM, 0);
	s.connect("localhost", port);	//connect to myself so the listenthread stops
	s.send("\n\n");	//send smthing so the handleclient function returns
	s.recv(1);
	s.disconnect();
}

void Server::handleClient(tcpsocket *client){
	json request = json::object{
		{"origin", client->getClientIP()},
		{"method", client->recvLine(' ', 8)},
		{"url", client->recvLine(' ', 2048)},
		{"version", client->recvLine('\n', 16)},
		{"header", [client]() -> json::object {
			json::object headers;
			std::string line;
			do {
				line = client->recvLine();
				size_t pos = line.find(':');
				if(line.size() >= 3 && pos != std::string::npos && pos > 0 && pos < line.size() - 1) {
					headers[toLower(std::string(line.begin(), line.begin() + pos))] = toLower(std::string(line.begin() + pos + 1, line.end()));
				}
			} while(line.size() >= 3);
			return headers;
		}()}
	};
	std::string url = request["url"].toString();
	if(url.find('?') < url.length() - 1) {
		request["url"] = url.substr(0, url.find('?'));
		std::string parameters = url.substr(url.find('?') + 1);
		request["parameters"] = [&parameters]() -> json {
			json::object result;
			for(std::string &param : split(parameters, '&')) {
				std::string key = param.substr(0, param.find('='));
				std::string val = "";
				if(param.find('=') < param.length() - 1) {
					val = param.substr(param.find('=') + 1);
				}
				result[key] = val;
			}
			return result;
		}();
	}
	if(request["header"]["cookie"].isString()) {
		json::object cookies;
		for(std::string &cookie : split(request["header"]["cookie"].toString(), ';')) {
			if(isspace(cookie[0]))
				cookie.erase(0, 1);
			std::string key = cookie.substr(0, cookie.find('='));
			std::string val = "";
			if(cookie.find('=') < cookie.length() - 1) {
				val = cookie.substr(cookie.find('=') + 1);
			}
			cookies[key] = val;
		}
		request["cookies"] = cookies;
	}
	else {
		request["cookies"] = json::object{};
	}
	spdlog::info("request: {}", request.print(json::minified));

	int method = parseMethod(request["method"].toString());
	std::string response;
	for(const Plugin &p : plugins){
		if((p.method & method) && (request["url"].toString().find(p.suburl) == 0)){	//url starts with p.suburl
			try {
				response = responseToString(p.callback(request, this, client));
			}
			catch (std::exception &e) {
				spdlog::error("exception thrown in plugin {} : {}", p.name, e.what());
				response = responseToString(json::object{
					{"request", request},
					{"version", "HTTP/1.1"},
					{"status", HttpStatus_InternalServerError},
					{"header", json::object{
						{"Content-Type", "application/json"}
					}}
				});
			}
			break;
		}
	}
	spdlog::info("response: {}", inflate(response.substr(0, response.find("\n\n"))));
	if(response != ""){
		client->send(response);
	}

	//disconnect
	client->disconnect();
	delete client;
}

std::string Server::responseToString(json response, bool payload){
	if(response.isNull())
		return "";
	std::stringstream result;
	result<<response["version"].toString()<<" ";
	result<<int(response["status"].toInt())<<" ";
	result<<HttpStatusReasonPhrase(response["status"].isNumber() ? response["status"].toInt() : 500)<<"\n";

	json header = response["header"].isObject() ? response["header"] : json::object{};
	for(auto & [key, val] : header.getObject()){
		result<<key<<": "<<val.toString()<<"\n";
	}

	if(options["cors"]){
		result<<"Access-Control-Allow-Origin: \n";
		result<<"Access-Control-Allow-Methods: *\n";
		result<<"Access-Control-Allow-Headers: *\n";
		result<<"Access-Control-Allow-Credentials: true\n";
	}
	result<<"Connection: close\n";

	std::string data = response["data"].toString();
	if(data.size() > 0 && payload){
		result<<"Content-Length: "<<data.size()<<"\n";
		result<<"\n";
		result<<data;
	}
	return result.str();
}

std::string Server::requestToString(json data){
	std::stringstream result;
	result<<data["method"].toString()<<" "<<data["url"].toString()<<" "<<data["version"].toString()<<"\n";
	for(auto & [key, val] : data["header"].getObject()){
		result<<key<<": "<<val.toString()<<"\n";
	}
	std::cout<<result.str()<<"\n";
	return result.str();
}

int& Server::option(const std::string &key){
	if(parent)
		return parent->option(key);
	return options[key];
}

int Server::parseMethod(std::string method){
	if(method == "CONNECT")
		return CONNECT;
	else if(method == "DELETE")
		return DELETE;
	else if(method == "GET")
		return GET;
	else if(method == "HEAD")
		return HEAD;
	else if(method == "OPTIONS")
		return OPTIONS;
	else if(method == "PATCH")
		return PATCH;
	else if(method == "POST")
		return POST;
	else if(method == "PUT")
		return PUT;
	else if(method == "TRACE")
		return TRACE;
	return NONE;
}

std::string Server::decodeUrl(std::string url){
	unsigned int len = url.length();
	std::string decoded(len, 0);
	for(unsigned int i = 0, k = 0; i < len; i++, k++){
		int c = url[i];
		if(c == '%' && i < len - 2){
			std::string byte = &url[i+1];
			byte.resize(2);
			std::stringstream ss;
			ss<<std::hex<<byte;
			ss>>c;
			i += 2;
		}
		decoded[k] = char(c);
	}
	return decoded.data();
}

dll& Server::loadLib(std::string name){
	if(libs.count(name) == 0){
		libs[name] = new dll(name);
		spdlog::info("loaded shared library \"{}\"", name);
	}
	return *libs[name];
}

void Server::registerPlugin(const Plugin &p){
	plugins.push_back(p);
}

void Server::loadPlugins(std::string fileName){
	std::ifstream f(fileName);
	std::string source((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	json data = json::parse(source);
	//std::cout<<data.print()<<"\n";
	for(auto &e : data.getArray()){
		try {
			std::string libname = e["dll"].toString();
			std::string func = e["function"].toString();
			std::string method = e["method"].toString();
			std::string name = e["name"].toString();
			std::string suburl = e["suburl"].toString();
			dll &lib = loadLib(libname);
			if(lib.get<json(*)(json, Server*, tcpsocket*)>(func)) {
				registerPlugin({
					parseMethod(method), name, suburl,
					lib.get<json(*)(json, Server*, tcpsocket*)>(func)
				});
			}
		}
		catch (std::exception &exception) {
			spdlog::error("couldn't load plugin (exception(\'{}\')): \'{}\'", exception.what(), e.print(json::minified));
		}
	}
	sortPlugins();
	spdlog::info("loaded {} plugins: ", plugins.size());
	for(Plugin p : plugins){
		spdlog::info("    {} : {}", p.name, p.suburl);
	}
}

void Server::sortPlugins(){
	std::sort(plugins.begin(), plugins.end(), [](Plugin &a, Plugin &b) -> bool {
		std::vector<std::string> la = split(a.suburl, '/');
		std::vector<std::string> lb = split(b.suburl, '/');
		if(la.size() == lb.size()){
			for(unsigned i = 0; i < la.size(); i++){
				std::string sa = la[i], sb = lb[i];
				if(sa[i] != sb[i]){
					if(sa.find(sb) == 0){
						return false;
					}
					else if(sb.find(sa) == 0){
						return true;
					}
					else{
						return sa > sb;
					}
				}
			}
			return a.suburl > b.suburl;
		}
		else{
			return la.size() > lb.size();
		}
	});
}

std::string Server::getPassPhrase() {
	return passPhrase;
}

int main(int argc, char *argv[]){
	srand(unsigned(time(nullptr)));
	std::vector<std::string> args(argv + 1, argv + argc);

	//auto logger = spdlog::daily_logger_mt("logger", "logs/daily.txt", 0, 0);
	//spdlog::set_default_logger(logger);
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$]\t%v");
	std::string passPhrase = "";

	for(unsigned i = 0; i < args.size() - 1; i++) {
		if(args[i] == "-p")
			passPhrase = args[i + 1];
	}
	Server *server = new Server(address::Any, 80, passPhrase);
	server->loadPlugins("./plugins.json");
	server->option("cors") = 1;
	server->start();
	delete server;
	return 0;
}