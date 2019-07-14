#include "AuthManager.h"
#include <iostream>
#include <stdexcept>
#include <random>
#include <string>
#include <cstring>
#include <chrono>
#include "util.h"
#include "bcrypt/blf.h"
#include "nlohmann/json.hpp"
#include <jwt/jwt.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/log/trivial.hpp>
#include <iostream>

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

	setupSharedSecret();
}

void AuthManager::setupSharedSecret()
{
	m_sharedSecret = m_config.get("_system.secret", "");
	if (!m_sharedSecret.empty())
		return;

	auto sharedSecret = generateSharedSecret(60);
	m_config.put("_system.secret", sharedSecret);
	saveConfig();
}

void AuthManager::createUser(const char* username, const char* password)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	auto existing = m_config.get_child_optional(username);
	if (existing)
		throw std::runtime_error("User already exists");

	boost::property_tree::ptree props;

	props.put("password", hashPassword(password));

	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	props.put("octoprint_compat_key", uuid);

	m_config.put_child(username, props);

	lock.unlock();
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
	std::unique_lock<std::mutex> lock(m_mutex);

	if (std::strcmp(username, "_system") == 0)
		return false;

	auto user = m_config.get_child_optional(username);
	if (!user)
		return false;
	
	return checkPassword(password, user->get<std::string>("password"));
}

std::string AuthManager::generateToken(const char* username)
{
	jwt::jwt_object obj{jwt::params::algorithm("hs256"), jwt::params::secret(m_sharedSecret), jwt::params::payload({{"username", username}})};
	obj.add_claim("exp", std::chrono::system_clock::now() + std::chrono::minutes{15});

	std::error_code ec;
  	auto str = obj.signature(ec);

	if (ec)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to generate a JWT: " << ec.message();
		throw std::runtime_error("Cannot generate JWT");
	}

	std::cout << "Generated token: " << str << std::endl;
	std::cout << "Validation result: " << checkToken(str.c_str()) << std::endl;

	return str;
}

std::string AuthManager::checkToken(std::string_view token) const
{
	std::error_code ec;
	auto dec_obj = jwt::decode(token, jwt::params::algorithms({"hs256"}), ec, jwt::params::secret(m_sharedSecret), jwt::params::verify(true));

	if (ec)
	{
		BOOST_LOG_TRIVIAL(warning) << "Token " << token << " failed to validate: " << ec.message();
		return std::string();
	}

	std::string username = dec_obj.payload().create_json_obj()["username"].get<std::string>();
	return username;
}

std::string AuthManager::generateSharedSecret(size_t length)
{
	std::random_device rd;  
    const char alphanumeric[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    std::mt19937 eng(rd()); 
    std::uniform_int_distribution<> distr(0, sizeof(alphanumeric) - 1);

	std::string secret(length, 0);
	std::generate_n(secret.begin(), length, [&]() { return alphanumeric[distr(eng)]; });

	return secret;
}

std::string AuthManager::authenticateOctoprintCompat(std::string_view key) const
{
	std::unique_lock<std::mutex> lock(m_mutex);

	for (const auto& user : m_config)
	{
		if (user.first == "_system")
			continue;
		
		if (user.second.get<std::string>("octoprint_compat_key") == key)
			return user.first;
	}

	return std::string();
}

std::string AuthManager::userOctoprintCompatKey(const char* username) const
{
	std::unique_lock<std::mutex> lock(m_mutex);

	auto existing = m_config.get_child_optional(username);
	if (!existing)
		return std::string();
	
	return existing->get<std::string>("octoprint_compat_key");
}
