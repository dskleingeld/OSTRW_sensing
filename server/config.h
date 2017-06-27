#ifndef CONFIG
#define CONFIG

#include <cstdint> //uint16_t

namespace config {

  constexpr int POSTBUFFERSIZE = 512;
	constexpr int MAXNAMESIZE = 20;
	constexpr int MAXANSWERSIZE = 512;

	constexpr int HTTPSERVER_PORT = 8442;
	constexpr const char* HTTPSERVER_USER = "test";
	constexpr const char* HTTPSERVER_PASS = "test"; //using random strings as passw
}

#endif
