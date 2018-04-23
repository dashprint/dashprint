//
// Created by lubos on 4/23/18.
//

#define BOOST_TEST_MODULE PrinterTest
#include <boost/test/included/unit_test.hpp>
#include "Printer.h"

BOOST_AUTO_TEST_CASE(TestKVParse)
{
	const char* string = "FIRMWARE_NAME:Marlin V1.0.2; Sprinter/grbl mashup for gen6 FIRMWARE_URL:https://github.com/prusa3d/Prusa-i3-Plus/";
	std::map<std::string,std::string> kv;

	Printer::kvParse(string, kv);

	BOOST_TEST(kv["FIRMWARE_NAME"] == "Marlin V1.0.2; Sprinter/grbl mashup for gen6");
	BOOST_TEST(kv["FIRMWARE_URL"] == "https://github.com/prusa3d/Prusa-i3-Plus/");
}
