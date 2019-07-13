#include <string_view>
#include <stdio.h>
#include <cctype>
#include <stdint.h>
#include <vector>
#include <array>

#define VISIBLE __attribute__((visibility("default")))

int main() { return 0; }

struct PrinterState
{
	float x, y, z, e;
	bool relXYZ, relE;
	std::vector<float> prints;
};

void processGcodeLine(size_t fileOffset, PrinterState& state, std::string_view line);
void trimString(std::string_view& str);
std::string_view nextWord(std::string_view& text);

extern "C" VISIBLE int layer_count = 0;
extern "C" VISIBLE float* layer_pointers = nullptr;
extern "C" VISIBLE int* move_pointers = nullptr;

extern "C" VISIBLE
void analyze_gcode(const char* data, size_t length)
{
	std::string_view gcode(data, length);
	int end;
	PrinterState printerState = {0};

	layer_count = 0;

	while ((end = gcode.find('\n')) != std::string_view::npos)
	{
		std::string_view line = gcode.substr(0, end);
		processGcodeLine(line.data() - data, printerState, line);

		gcode.remove_prefix(end+1);
	}
	if (!gcode.empty())
		processGcodeLine(gcode.data() - data, printerState, gcode);
}

// More concise than std::atof. Even more so than std::stof.
float stof(const char* s)
{
  float rez = 0, fact = 1;
  if (*s == '-'){
    s++;
    fact = -1;
  };
  for (int point_seen = 0; *s; s++){
    if (*s == '.'){
      point_seen = 1; 
      continue;
    };
    int d = *s - '0';
    if (d >= 0 && d <= 9){
      if (point_seen) fact /= 10.0f;
      rez = rez * 10.0f + (float)d;
    };
  };
  return rez * fact;
}

inline float stof(std::string_view sv)
{
	return stof(sv.data());
}

void processGcodeLine(size_t fileOffset, PrinterState& state, std::string_view line)
{
	// Remove comments
	auto sep = line.find(';');

	if (sep != std::string_view::npos)
		line.remove_suffix(line.length()-sep);

	// Trim spaces
	trimString(line);

	if (line.empty())
		return;

	auto cmd = nextWord(line);
	if (cmd == "G1" || cmd == "G01")
	{
		// Movement
	}
	else if (cmd == "G90")
	{
		// Absolute positioning for XYZ (default)
		state.relXYZ = false;
	}
	else if (cmd == "G91")
	{
		// Relative positioning for XYZ
		state.relXYZ = true;
	}
	else if (cmd == "G92")
	{
		// Declare the current position as
		while (true)
		{
			auto sv = nextWord(line);
			if (sv.empty())
				break;
			if (sv[0] == 'X')
				state.x = stof(sv.substr(1));
			else if (sv[0] == 'Y')
				state.y = stof(sv.substr(1));
			else if (sv[0] == 'Z')
				state.z = stof(sv.substr(1));
			else if (sv[0] == 'E')
				state.e = stof(sv.substr(1));
		}
	}
	else if (cmd == "M82")
	{
		// Absolute positioning for E (default)
		state.relE = false;
	}
	else if (cmd == "M83")
	{
		// Relative positioning for E
		state.relE = true;
	}
}

void trimString(std::string_view& str)
{
	while (!str.empty() && std::isspace(str[0]))
		str.remove_prefix(1);
	while (!str.empty() && std::isspace(str.back()))
		str.remove_suffix(1);
}

std::string_view nextWord(std::string_view& text)
{
	int end = 0;

	trimString(text);
	while (end < text.length() && !std::isspace(text[end]))
		end++;

	std::string_view rv = text.substr(0, end);

	if (end < text.length())
		text.remove_prefix(end+1);
	else
		text = std::string_view();

	return rv;
}

