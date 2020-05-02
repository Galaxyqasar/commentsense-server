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
#include <angelscript/angelscript.h>
#include <angelscript/plugins/all.hpp>
#include <crypt/sha.hpp>
#include <crypt/crypt.hpp>
#include <crypt/rijndael.hpp>

using namespace network;
using utils::dll;

class Server;

struct Plugin {
	int method;
	std::string name;
	std::string suburl;
	std::function<json(json, Server*, tcpsocket*)> callback;
	bool active;
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
	
	static int parseMethod(std::string method);
	static std::string decodeUrl(std::string url);

	dll& loadLib(std::string name);
	void loadPlugins(std::string fileName);
	void registerPlugin(const Plugin &plugin);
	void sortPlugins();

	std::string getPassPhrase();

	void setOption(std::string key, int val);
	void setPluginActive(std::string name, bool active);

protected:
	network::tcpsocket *socket = nullptr;

private:
	bool quit = false;
	unsigned short port = 0;
	std::map<std::string, int> options;
	std::map<std::string, dll*> libs;
	std::vector<Plugin> plugins;
	std::string passPhrase;
};