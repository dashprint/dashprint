#define BOOST_TEST_MODULE MultipartTest
#include <boost/test/included/unit_test.hpp>
#include "web/MultipartFormData.h"
#include <cstdlib>

BOOST_AUTO_TEST_CASE(TestKVParse)
{
	const char* string1 = "multipart/form-data; boundary=AaB03x";
	const char* string2 = "form-data; name=\"files\"; filename=\"file1.txt\"";

	MultipartFormData::Headers_t headers;
	std::string value;

	MultipartFormData::parseKV(string1, value, headers);

	BOOST_TEST(value == "multipart/form-data");
	BOOST_TEST((bool)(headers.find("boundary") != headers.end()));
	BOOST_TEST(headers["boundary"] == "AaB03x");

	MultipartFormData::parseKV(string2, value, headers);

	BOOST_TEST(value == "form-data");
	BOOST_TEST((bool)(headers.find("name") != headers.end()));
	BOOST_TEST((bool)(headers.find("filename") != headers.end()));
	BOOST_TEST(headers["name"] == "files");
	BOOST_TEST(headers["filename"] == "file1.txt");
}

BOOST_AUTO_TEST_CASE(TestMultipartParse)
{
	const char multipart[] = "Content-Type: multipart/form-data; boundary=AaB03x\r\n"
		"\r\n"
   		"--AaB03x\r\n"
		"Content-Disposition: form-data; name=\"submit-name\"\r\n"
		"\r\n"
   		"Larry\n"
   		"--AaB03x\r\n"
   		"Content-Disposition: form-data; name=\"files\"; filename=\"file1.txt\"\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"Contents of file1.txt--AaB03x--";
		   
	char tempfile[] = "/tmp/TestMultipartParseXXXXXX";
	int fd = mkstemp(tempfile);

	write(fd, multipart, sizeof(multipart)-1);
	close(fd);

	MultipartFormData mfd(tempfile);

	remove(tempfile);

	int callbacks = 0;

	mfd.parse([&](const MultipartFormData::Headers_t& headers, const char* name, const void* data, uint64_t length) {
		if (std::strcmp(name, "submit-name") == 0)
		{
			BOOST_TEST(length == 6);
			BOOST_TEST(std::memcmp((char*) data, "Larry\n", length) == 0);
		}
		else if (std::strcmp(name, "files") == 0)
		{
			BOOST_TEST(length == 21);
			BOOST_TEST(std::memcmp((char*) data, "Contents of file1.txt", length) == 0);
		}
		else
			BOOST_FAIL("Unexpected value name");

		callbacks++;
		return true;
	});

	BOOST_TEST(callbacks == 2);
}
