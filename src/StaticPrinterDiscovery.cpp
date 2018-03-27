/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   StaticPrinterDiscovery.cpp
 * Author: lubos
 * 
 * Created on 24. března 2018, 23:57
 */

#include "StaticPrinterDiscovery.h"
#include <dirent.h>
#include <cstring>

void StaticPrinterDiscovery::enumerate(std::vector<DiscoveredPrinter>& out)
{
	out.clear();
	
	// First try discovering devices in /dev/serial/by-id
	// If that fails, try /dev/ttyUSB* and /dev/ttyACM*
	// TODO
	DIR* dir = ::opendir("/dev/serial/by-id/");
	struct dirent* ent;
	
	if (dir != nullptr)
	{
		while ((ent = ::readdir(dir)) != nullptr)
		{
			if (ent->d_type == DT_DIR)
				continue;
			
			DiscoveredPrinter dp;
			
			dp.devicePath = "/dev/serial/by-id/";
			dp.devicePath += ent->d_name;
			dp.deviceName = ent->d_name;
			dp.deviceSerial = ent->d_name;
			
			out.push_back(dp);
		}
		::closedir(dir);
	}
	
	if (out.empty())
	{
		dir = ::opendir("/dev/");
		
		if (dir != nullptr)
		{
			while ((ent = ::readdir(dir)) != nullptr)
			{
				if (std::strncmp(ent->d_name, "ttyUSB", 6) == 0 || std::strncmp(ent->d_name, "ttyACM", 6) == 0)
				{
					DiscoveredPrinter dp;
			
					dp.devicePath = "/dev/";
					dp.devicePath += ent->d_name;
					dp.deviceName = ent->d_name;
					dp.deviceSerial = ent->d_name;

					out.push_back(dp);
				}
			}
			::closedir(dir);
		}
	}
}
