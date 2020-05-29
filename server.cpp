#include "server.hpp"

std::string constructHttpResponse(json response, ServerConfig *config) {
	if(response.isNull())
		return "";
	std::stringstream result;
	result<<response["version"].toString()<<" ";
	result<<int(response["status"].toInt())<<" ";
	result<<inet::HttpStatusReasonPhrase(response["status"].isNumber() ? response["status"].toInt() : 500)<<"\n";

	json header = response["header"].isObject() ? response["header"] : json::object{};
	for(auto & [key, val] : header.getObject()){
		result<<key<<": "<<val.toString()<<"\n";
	}

	if(config && config->getOption("cors")){
#if defined(__CUSTOM_BUILD_FOR_NICLAS__)
		result<<"Access-Control-Allow-Origin: http://localhost:3000\n";
#else
		result<<"Access-Control-Allow-Origin: \n";
#endif
		result<<"Access-Control-Allow-Methods: *\n";
		result<<"Access-Control-Allow-Headers: *\n";
		result<<"Access-Control-Allow-Credentials: true\n";
	}
	result<<"Connection: close\n";

	std::string data = response["data"].toString();
	if(data.size() > 0){
		result<<"Content-Length: "<<data.size()<<"\n";
		result<<"\n";
		result<<data;
	}
	return result.str();
}

std::string constructHttpRequest(json request, ServerConfig *config) {
	std::stringstream result;
	result<<request["method"].toString()<<" "<<request["url"].toString()<<" "<<request["version"].toString()<<"\n";
	for(auto & [key, val] : request["header"].getObject()){
		result<<key<<": "<<val.toString()<<"\n";
	}
	std::cout<<result.str()<<"\n";
	return result.str();
}

std::string decodeUrl(const std::string &url) {
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

int parseMethod(const std::string &method) {
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

#include <signal.h>
#include <angelscript/angelscript.h>
#include <angelscript/plugins/all.hpp>
#include <crypt/sha.hpp>
#include <crypt/crypt.hpp>
#include <crypt/rijndael.hpp>

std::stringstream buffer;

void print(std::string str) { buffer<<str; }
void print(int i) { buffer<<i; }
void print(double d) { buffer<<d; }

void MessageCallback(const asSMessageInfo *msg) {
	switch(msg->type) {
		case asMSGTYPE_ERROR: 		spdlog::error("[{}:{}] {} : {}", msg->row, msg->col, msg->section, msg->message); break;
		case asMSGTYPE_WARNING: 	spdlog::warn("[{}:{}] {} : {}", msg->row, msg->col, msg->section, msg->message); break;
		case asMSGTYPE_INFORMATION: spdlog::info("[{}:{}] {} : {}", msg->row, msg->col, msg->section, msg->message); break;
	}
	buffer<<(msg->type == asMSGTYPE_ERROR ? "[ERROR]" : msg->type == asMSGTYPE_WARNING ? "[WARNING]" : "[INFO]");
	buffer<<"["<<msg->row<<":"<<msg->col<<"] "<<msg->section<<" : "<<msg->message<<"\n";
}

using namespace inet;
#if defined(IPV6)
	using address_t = inet::ipv6address;
#else
	using address_t = inet::ipv4address;
#endif

int main(int argc, char *argv[]) {
	struct sigaction s{[](int i){
		spdlog::warn("received SIGPIPE: {}", i);
	}};
	sigaction(SIGPIPE, &s, NULL);
	srand(unsigned(time(nullptr)));
	std::vector<std::string> args(argv + 1, argv + argc);

	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$]\t%v");
	std::string passPhrase = "";
	bool cors = false;

	for(unsigned i = 0; i < args.size(); i++) {
		if(args[i] == "-p" && i < args.size() - 1)
			passPhrase = stringFromHex(toUpper(crypt::sha256::hash(args[i + 1])));
		if(args[i] == "-cors")
			cors = true;
	}

	asIScriptEngine *engine = asCreateScriptEngine();
	engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL);
	RegisterScriptAny(engine);
	RegisterScriptMath(engine);
	RegisterScriptArray(engine, true);
	RegisterStdString(engine);
	RegisterStdStringUtils(engine);

	HttpServer<address_t> server(passPhrase);

	engine->RegisterGlobalFunction("void setOption(string name, int state)", asMETHODPR(HttpServer<address_t>, setOption, (std::string, int), void), asCALL_THISCALL_ASGLOBAL, &server);
	engine->RegisterGlobalFunction("void setPluginActive(string name, bool active)", asMETHODPR(HttpServer<address_t>, setPluginActive, (std::string, bool), void), asCALL_THISCALL_ASGLOBAL, &server);
	engine->RegisterGlobalFunction("void stop()", asMETHODPR(HttpServer<address_t>, stop, (void), void), asCALL_THISCALL_ASGLOBAL, &server);
	engine->RegisterGlobalFunction("void print(string val)", asFUNCTIONPR(print, (std::string), void), asCALL_CDECL);
	engine->RegisterGlobalFunction("void print(int val)", asFUNCTIONPR(print, (int), void), asCALL_CDECL);
	engine->RegisterGlobalFunction("void print(double val)", asFUNCTIONPR(print, (double), void), asCALL_CDECL);
	server.registerPlugin({GET, "call as function", "/api/server", 
		[engine](json request, ServerConfig *server, inet::tcpclient<address_t>*){
			buffer.str("");	//clear buffer
			std::string code = request["parameters"]["code"].toString();
			if(server->getPassPhrase().length())
				code = crypt::rijndael(stringFromHex(toUpper(code)), server->getPassPhrase(), crypt::decrypt);
			spdlog::info("running as code \'{}\'", code);

			ExecuteString(engine, code.c_str());
			asThreadCleanup();
			std::string result = buffer.str();
			if(server->getPassPhrase().length()) {
				result = crypt::rijndael(result, server->getPassPhrase(), crypt::encrypt);
				result = toLower(stringToHex(result));
			}
			return json::object{
				{"request", request},
				{"version", "HTTP/1.1"},
				{"status", 200},
				{"header", json::object{}},
				{"data", result}
			};
		}, true
	});

	std::cout<<server.getPassPhrase()<<"\n";
	(&server)->setOption("cors", cors);
	uint16_t port = 80;
#if defined(__TLS__)
	port = 443;
#endif
	server.loadPlugins("./plugins.json");
	server.run(address_t(address_t::Any, port));
	return 0;
}