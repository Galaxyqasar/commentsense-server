#include "server.hpp"

logfile *serverlog;

Server::Server(network::address addr, unsigned short port, bool defaultp){
	this->port = port;
	socket = new network::tcpsocket(AF_INET, SOCK_STREAM, 0);
	socket->bind(addr, port);

	if(defaultp){
		plugins.push_back({ANY - GET, "default", "/", {}, [](PluginArg arg) -> json {
			return json::object("response", {
				arg.request,
				json::string("version", "HTTP/1.1"),
				json::number("status", 501),
				json::array("header", {}),
				json::string("data", arg.request.print())
			});
		}});
	}
}

Server::~Server(){
	delete socket;
}

void Server::start(){
	listenThread = new std::thread(&Server::listen, this);
	if(!parent){
		console();
	}
}

void Server::listen(){
	if(socket->listen()) {
		while(!quit){
			tcpsocket *client = socket->accept({5,0});
			std::thread handler(&Server::handleClient, this, client);
			handler.detach();
		}
	}
}

void Server::console(){
	std::string input;
	while(!quit){
		std::cout<<"\n>";
		std::cin>>input;
		if(input == "stop"){
			for(Server *child : children){
				child->stop();
			}
			stop();
		}
		if(input.find('=') != std::string::npos){
			std::string key = split(input, '=')[0];
			int val = atoi(split(input, '=')[1].c_str());
			if(key == "cors"){
				option(key) = val;
			}
		}
	}
}

void Server::stop(){
	quit = true;
	tcpsocket s(AF_INET, SOCK_STREAM, 0);
	s.connect("localhost", port);	//connect to myself so the listenthread stops
	s.send("\n\n");	//send smthing so the handleclient function returns
	s.recv(1);
	s.disconnect();
	listenThread->join();
	delete listenThread;
}

void Server::handleClient(tcpsocket *client){
	json header = json::object("header", {});
	std::string line = client->recvLine();
	std::vector<std::string> parts = split(line, ' ');
	if(parts.size() == 3){
		while(line.size() >= 3){
			line = client->recvLine();
			size_t separator = line.find(':');
			if(separator != std::string::npos){
				std::string key(line.begin(), line.begin() + separator);
				std::string value(line.begin() + separator + 2, line.end());
				header += json::string(toLower(key), value);
			}
		}
		json request = json::object("request", {
			json::string("origin", client->getClientIP()),
			json::string("method", parts[0]),
			json::string("url", decodeUrl(parts[1])),
			json::string("version", parts[2]),
			header});
		log(*serverlog, "[", client->getClientIP(), "]\t", request.print(json::minified));

		int method = parseMethod(parts[0]);
		std::string response;
		for(Plugin &p : plugins){
			if((p.method & method) && (parts[1].find(p.suburl) == 0)){	//url starts with p.suburl
				try {
					response = responseToString(p.func({request, this, client}));
				}
				catch (std::bad_variant_access &e) {
					log(*serverlog, "    error: bad_variant_access exception thrown in plugin ", p.name);
					response = responseToString(json::object("response", {
						request,
						json::string("version", "HTTP/1.1"),
						json::number("status", HttpStatus_InternalServerError),
						json::array("header", {
							json::string("Content-Type", "application/json")
						}),
						json::string("data", json::object("", {json::string("status", "error: internal sqlite error")}).toStr())
					}));
				}
				break;
			}
		}
		if(response != ""){
			client->send(response);
		}
	}
	else if(line.size() == 0){
		log(*serverlog, "[", client->getClientIP(), "]\t", "sent nothing");
	}
	else{
		while(line.size() > 0){
			log(*serverlog, "[", client->getClientIP(), "]\t", "received line : \"", line, "\"|", stringToHex(line, true));
			line = client->recvLine();
		}
	}
	//disconnect
	client->disconnect();
	delete client;
}

std::string Server::responseToString(json response, bool payload){
	if(response.isNull())
		return "";
	std::string version = response["version"].toStr();
	std::string status = HttpStatusReasonPhrase(response["status"].toInt());
	std::string data = response["data"].valueStr();
	json header = response["header"];

	std::stringstream result;
	result<<version<<" "<<response["status"].toInt()<<" "<<status<<"\n";
	for(json &e : header){
		result<<e.name()<<": "<<e.toStr()<<"\n";
	}

	if(options["cors"]){
		result<<"Access-Control-Allow-Origin: *\n";
		result<<"Access-Control-Allow-Methods: *\n";
		result<<"Access-Control-Allow-Headers: *\n";
		result<<"Access-Control-Allow-Credentials: true\n";
	}
	result<<"Connection: close\n";
	if(data.size() > 0 && payload){
		result<<"Content-Length: "<<data.size()<<"\n";
		result<<"\n";
		result<<data;
	}
	return result.str();
}

std::string Server::requestToString(json data){
	std::stringstream result;
	result<<data["method"].toStr()<<" "<<data["url"].toStr()<<" "<<data["version"].toStr()<<"\n";
	for(json &e : data["header"]){
		result<<e.name()<<": "<<e.toStr()<<"\n";
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

void Server::sync(Server *other){
	parent = other;
	parent->children.push_back(this);
}

dll& Server::loadLib(std::string name){
	if(libs.count(name) == 0){
		libs[name] = dll(name);
		std::cout<<"loaded shared library \""<<name<<"\"\n";
	}
	return libs[name];
}

void Server::addPlugin(Plugin p){
	plugins.push_back(p);
}

void Server::loadPlugins(std::string fileName){
	json data = json::parse(file::readAll(fileName));
	for(json &e : data){
		std::string name = e["name"].toStr();
		std::string libname = e["dll"].toStr();
		std::string func = e["function"].toStr();
		std::string method = e["method"].toStr();
		std::string suburl = e["suburl"].toStr();
		std::vector<std::string> requirements;

		for(json& r : e["requirements"]){
			loadLib(r.toStr());
		}
		dll &lib = loadLib(libname);
		plugins.push_back({parseMethod(method), name, suburl, requirements, lib.get<Plugin::callback>(func)});
	}
	sortPlugins();
	std::cout<<"loaded "<<plugins.size()<<" plugins\n";
	for(Plugin &p : plugins){
		std::cout<<p.name<<":"<<p.suburl<<"\n";
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


int main(int argc, char *argv[]){
	srand(unsigned(time(nullptr)));
	serverlog = new logfile("./log.txt");

	Server *server = new Server(address::Any, 80, false);
	server->loadPlugins("./plugins.json");
	server->option("cors") = 1;
	server->start();
	delete server;
	return 0;
}