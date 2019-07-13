#include "AuthManager.h"
#include <iostream>
#include <stdexcept>
#include <random>
#include <string>
#include "util.h"
#include "bcrypt/blf.h"

AuthManager::AuthManager(boost::property_tree::ptree& config)
: m_config(config)
{
	if (m_config.begin() == m_config.end())
	{
		std::cout << "\nCreating initial user:\n"
			"Username: admin\n"
			"Password: dashprint\n\n";

		createUser("admin", "dashprint");
	}
}

void AuthManager::createUser(const char* username, const char* password)
{
	auto existing = m_config.get_child_optional(username);
	if (existing)
		throw std::runtime_error("User already exists");

	boost::property_tree::ptree props;
	props.put("password", hashPassword(password));
	m_config.put_child(username, props);

	saveConfig();
}

std::string AuthManager::hashPassword(const char* password)
{
	char hash[_PASSWORD_LEN+1] = {0};
	std::string salt = generateSalt();

	::bcrypt(password, salt.c_str(), hash);

	return hash;
}

std::string AuthManager::generateSalt()
{
	uint8_t seed[BCRYPT_MAXSALT];
	char salt[BCRYPT_MAXSALT+1] = {0};

	std::independent_bits_engine<std::default_random_engine, CHAR_BIT, uint8_t> rbe;
	std::generate(seed, seed+sizeof(seed), std::ref(rbe));

	::bcrypt_gensalt('a', 10, seed, salt);
	return salt;
}

bool AuthManager::checkPassword(const char* password, std::string_view hash)
{
	char hash2[_PASSWORD_LEN+1] = {0};
	std::string salt(hash, 0, 29);

	::bcrypt(password, salt.c_str(), hash2);

	return hash == hash2;
}

bool AuthManager::authenticate(const char* username, const char* password)
{
	auto user = m_config.get_child_optional(username);
	if (!user)
		return false;
	
	return checkPassword(password, m_config.get<std::string>("password"));
}
