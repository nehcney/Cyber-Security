#ifndef INTELWEB_H_
#define INTELWEB_H_

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER

#include "InteractionTuple.h"
#include "DiskMultiMap.h"
#include <string>
#include <vector>
#include <set>

class IntelWeb
{
public:
	IntelWeb();
	~IntelWeb();
	bool createNew(const std::string& filePrefix, unsigned int maxDataItems);
	bool openExisting(const std::string& filePrefix);
	void close();
	bool ingest(const std::string& telemetryFile);
	unsigned int crawl(const std::vector<std::string>& indicators,
		unsigned int minPrevalenceToBeGood,
		std::vector<std::string>& badEntitiesFound,
		std::vector<InteractionTuple>& badInteractions);
	bool purge(const std::string& entity);

private:
	bool m_fileOpen;
	DiskMultiMap forward;
	DiskMultiMap reverse;
	
private:
	bool prevalenceUnderThreshold(const std::string& key, unsigned int threshold);
	unsigned int recursiveSearch(const std::string& indicator, unsigned int minPrevalenceToBeGood,
		std::set<std::string>& badEntitiesFound,
		std::set<InteractionTuple>& badInteractions);
	InteractionTuple toInteractionTuple(const MultiMapTuple& m, bool forward);
};

// Comparison Operator for InteractionTuple

inline bool operator<(const InteractionTuple& a, const InteractionTuple& b)
{
	if (a.context < b.context)
		return true;
	if (a.context == b.context && a.from < b.from)
		return true;
	if (a.context == b.context && a.from == b.from && a.to < b.to)
		return true;
	return false;
}

#endif // INTELWEB_H_