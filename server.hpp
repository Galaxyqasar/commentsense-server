#pragma once

#include <network/address.hpp>
#include <network/statuscodes.hpp>
#include <network/tcpsocket.hpp>
#include <utils/json.hpp>
#include <utils/dll.hpp>

#include <map>
#include <string>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

class ServerConfig {
public:
	inline ServerConfig(std::string passphrase = "") : passphrase(passphrase) {}
	inline int getOption(std::string key) {
		return options[key];
	}
	inline void setOption(std::string key, int value) {
		options[key] = value;
	}
	std::string getPassPhrase() {
		return passphrase;
	}
private:
	std::string passphrase;
	std::map<std::string, int> options;
};

enum methods{
	NONE = 		0,
	CONNECT = 	1<<0,
	DELETE = 	1<<1,
	GET = 		1<<2,
	HEAD = 		1<<3,
	OPTIONS = 	1<<4,
	PATCH = 	1<<5,
	POST = 		1<<6,
	PUT = 		1<<7,
	TRACE = 	1<<8,
	ANY = (1<<0) + (1<<1) + (1<<2) + (1<<3) + (1<<4) + (1<<5) + (1<<6) + (1<<7) + (1<<8)
};

std::string constructHttpResponse(json response, ServerConfig *config = nullptr);
std::string constructHttpRequest(json request, ServerConfig *config = nullptr);
std::string decodeUrl(const std::string &url);
int parseMethod(const std::string &method);

template <typename address_t>
class HttpServer : public ServerConfig{
public:
	struct Plugin {
		int method;
		std::string name;
		std::string suburl;
		std::function<json(json, ServerConfig*, inet::tcpclient<address_t>&)> callback;
		bool active;
	};

	template <typename ...Args>
	HttpServer(Args ...args) : ServerConfig(args...) {}

	json recvRequest(inet::tcpclient<address_t> &client) {
		return json::object{
			{"method", parseMethod(client.recvline(' ', 8))},
			{"url", client.recvline(' ', 2048)},
			{"version", client.recvline('\n', 16)},
			{"header", [&client]() -> json::object {
				json::object headers;
				std::string line;
				do {
					line = client.recvline();
					size_t pos = line.find(':');
					if(line.size() >= 3 && pos != std::string::npos && pos > 0 && pos < line.size() - 1) {
						headers[toLower(std::string(line.begin(), line.begin() + pos))] = toLower(std::string(line.begin() + pos + 1, line.end()));
					}
				} while(line.size() >= 3);
				return headers;
			}()}
		};
	}
	json parseRequest(json &request) {
		std::string url = decodeUrl(request["url"].toString());
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
		return request;
	}
	void run(address_t addr) {
		quit = false;
		inet::tcpserver<address_t> server;
		server.bind(addr);
		server.listen(64);
		while(!quit) {
			address_t peeraddr;
			inet::tcpclient<address_t> client = server.accept(1s, &peeraddr);
			if(client) {
				std::thread([this](inet::tcpclient<address_t> client, address_t peeraddr){
					spdlog::info("client connected from {}", peeraddr.to_string());
					try {
						client.setRecvTimeout(30s);
						client.setSendTimeout(30s);
						json request = recvRequest(client);
						parseRequest(request);
						spdlog::info("request: {}", request.print(json::minified));
						if(request["version"].toString() != "HTTP/1.1" || request["method"].toInt() == NONE) {
							spdlog::error("request not following http/1.1 standard");
							return;
						}
						json response = handleClient(client, request);
						std::string res = constructHttpResponse(response, this);
						spdlog::info("sending response: {}", removeAll(res.substr(0, res.find("\n\n")), "\n"));
						client.send(res);
					} catch (std::exception &e) {
						std::cout<<e.what();
					}
				}, std::move(client), peeraddr).detach();
			}
		}
	}
	json handleClient(inet::tcpclient<address_t> &client, json &request) {
		int method = request["method"].toInt();
		for(const Plugin &p : plugins){
			if(p.active && (p.method & method) && (request["url"].toString().find(p.suburl) == 0)){
				try {
					return p.callback(request, this, client);
				}
				catch (std::exception &e) {
					spdlog::error("exception thrown in plugin {} : {}", p.name, e.what());
					return json::object{
						{"request", request},
						{"version", "HTTP/1.1"},
						{"status", inet::HttpStatus_InternalServerError}
					};
				}
			}
		}
		return json::object{
			{"request", request},
			{"version", "HTTP/1.1"},
			{"status", inet::HttpStatus_InternalServerError}
		};
	}
	void stop() {
		quit = true;
	}
	void setPluginActive(std::string name, bool active) {
		for(Plugin &p : plugins) {
			if(p.name == name) {
				p.active = active;
			}
		}
	}

	utils::dll& loadLib(std::string name) {
		if(libs.count(name) == 0){
			libs[name] = new utils::dll(name);
			spdlog::info("loaded shared library \"{}\"", name);
		}
		return *libs[name];
	}
	void loadPlugins(std::string fileName) {
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
				utils::dll &lib = loadLib(libname);
				if(lib.get<json(*)(json, ServerConfig*, inet::tcpclient<address_t>&)>(func)) {
					plugins.push_back(Plugin{
						parseMethod(method), name, suburl,
						lib.get<json(*)(json, ServerConfig*, inet::tcpclient<address_t>&)>(func), true
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
	void registerPlugin(const Plugin &plugin) {
		plugins.push_back(plugin);
	}
	void sortPlugins() {	
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

private:
	bool quit = false;
	std::map<std::string, utils::dll*> libs;
	std::vector<Plugin> plugins;
};
