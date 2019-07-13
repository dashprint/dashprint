#ifndef _AUTHMANAGER_H
#define _AUTHMANAGER_H
#include <boost/property_tree/ptree.hpp>
#include <string_view>
#include <string>
#include <mutex>

class AuthManager
{
public:
	AuthManager(boost::property_tree::ptree& config);

	void createUser(const char* username, const char* password);
	bool authenticate(const char* username, const char* password);
	std::string authenticateOctoprintCompat(const char* key);
	std::string generateToken(const char* username);
	std::string checkToken(const char* token);
private:
	static std::string generateSalt();
	static std::string hashPassword(const char* password);
	static bool checkPassword(const char* password, std::string_view hash);
	static std::string generateSharedSecret(size_t length);

	void setupSharedSecret();
private:
	boost::property_tree::ptree& m_config;
	std::mutex m_mutex;
	std::string m_sharedSecret;
};

#endif
