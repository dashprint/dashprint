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
	std::string authenticateOctoprintCompat(std::string_view key) const;
	std::string generateToken(const char* username);
	std::string checkToken(std::string_view) const;
	std::string userOctoprintCompatKey(const char* username) const;
private:
	static std::string generateSalt();
	static std::string hashPassword(const char* password);
	static bool checkPassword(const char* password, std::string_view hash);
	static std::string generateSharedSecret(size_t length);

	void setupSharedSecret();
private:
	boost::property_tree::ptree& m_config;
	mutable std::mutex m_mutex;
	std::string m_sharedSecret;
};

#endif
