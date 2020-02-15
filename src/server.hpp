#include "yeetlib.hpp"
#include <time.h>
#include <map>
#include <thread>
#include <functional>

using namespace utils;
using namespace network;

class Server;

struct PluginArg{
	json request;
	Server *server;
	network::tcpsocket *client;
};

struct Plugin{
	typedef json (*callback)(PluginArg);

	int method;
	std::string name;
	std::string suburl;
	std::vector<std::string> requirements;
	std::function<json(PluginArg)> func;
};

class Server{
public:
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

	Server(network::address addr, unsigned short port, bool defaultp = true);
	~Server();
	void start();
	void listen();
	void console();
	void stop();
	void handleClient(network::tcpsocket *client);

	std::string requestToString(json data);
	std::string responseToString(json response, bool payload = true);

	int& option(const std::string &key);
	
	static int parseMethod(std::string method);
	static std::string decodeUrl(std::string url);

	void sync(Server *other);

	void addPlugin(Plugin p);
	dll& loadLib(std::string name);
	void loadPlugins(std::string fileName);
	void sortPlugins();

protected:
	network::tcpsocket *socket = nullptr;
	std::thread *listenThread = nullptr;

private:
	bool quit = false;
	unsigned short port = 0;
	std::map<std::string, int> options;
	std::map<std::string, dll> libs;
	std::vector<Plugin> plugins;
	Server *parent = nullptr;
	std::vector<Server*> children;
};