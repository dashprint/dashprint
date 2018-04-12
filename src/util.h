//
// Created by lubos on 11.4.18.
//

#ifndef PI3PRINT_UTIL_H
#define PI3PRINT_UTIL_H

std::string urlSafeString(const char* str, const char* fallback);

void loadConfig();
void saveConfig();

#endif //PI3PRINT_UTIL_H
