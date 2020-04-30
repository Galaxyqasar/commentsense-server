#include <map>
#include <thread>
#include <ctime>
#include <functional>
#include <csignal>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <network/statuscodes.hpp>
#include <network/tcpsocket.hpp>
#include <utils/json.hpp>
#include <utils/dll.hpp>
#include <utils/string.hpp>

using namespace network;
using utils::dll;

class Server;

struct Plugin {
	int method;
	std::string name;
	std::string suburl;
	std::function<json(json, Server*, tcpsocket*)> callback;
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

	Server(network::address addr, unsigned short port, std::string passPhrase = "");
	~Server();
	void start();
	void stop();
	void handleClient(network::tcpsocket *client);

	std::string requestToString(json data);
	std::string responseToString(json response, bool payload = true);

	int& option(const std::string &key);
	
	static int parseMethod(std::string method);
	static std::string decodeUrl(std::string url);

	void registerPlugin(const Plugin &p);
	dll& loadLib(std::string name);
	void loadPlugins(std::string fileName);
	void sortPlugins();

	std::string getPassPhrase();

protected:
	network::tcpsocket *socket = nullptr;

private:
	bool quit = false;
	unsigned short port = 0;
	std::map<std::string, int> options;
	std::map<std::string, dll*> libs;
	std::vector<Plugin> plugins;
	Server *parent = nullptr;
	std::string passPhrase;
};