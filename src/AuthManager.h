#ifndef _AUTHMANAGER_H
#define _AUTHMANAGER_H
#include <boost/property_tree/ptree.hpp>
#include <string_view>
#include <string>

class AuthManager
{
public:
	AuthManager(boost::property_tree::ptree& config);

	void createUser(const char* username, const char* password);
	bool authenticate(const char* username, const char* password);
private:
	static std::string generateSalt();
	static std::string hashPassword(const char* password);
	static bool checkPassword(const char* password, std::string_view hash);
private:
	boost::property_tree::ptree& m_config;
};

#endif
