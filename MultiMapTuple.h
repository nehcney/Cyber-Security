// MultiMapTuple is a structure used to hold the 3 string values of each telemetry item.

#ifndef MULTIMAPTUPLE_H_
#define MULTIMAPTUPLE_H_

#include <string>

struct MultiMapTuple
{
	std::string key;
	std::string value;
	std::string context;
};

#endif // MULTIMAPTUPLE_H_
