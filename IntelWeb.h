// The IntelWeb class is responsible for loading all of the log data from one or more
// telemetry log data files, placing this data onto disk (using DiskMultiMap class),
// and discovering new attacks by searching through this data structure. It can also
// purge specific telemetry data items from its data structures.
//
// In order to facilitate malicious entity discovery in both directions (ie. a known
// malicious website downloading an unknown file would implicate the file, but also
// an unknown website downloading a known malicious file would implicate the website!)
// we create two DiskMultiMaps, one called forward (ie. A creates B, where A is the key)
// and one called reverse (ie. B is created by A, where B is the key) so that both the 
// creator and the created can be discovered by searching our hash tables.
//
// ingest() - simply inserts all the data from a telemetry log file of the specified name
//     into the appropriate disk-based data structures (DiskMultiMap).
// crawl() - responsible for (a) discovering and outputting an ordered vector of all 
//     malicious entities found in the previously-ingested telemetry, and (b) outputting
//     an ordered vector of every interaction discovered that includes at least one
//     malicious entity, by "crawling" through the ingested telemetry. In order to discover
//     new malicious entities, we enter all known threat indicators into a queue, then 
//     search our forward and reverse hash tables. If the indicator is found, and the 
//     associated entity with that indicator has not yet been tagged as a threat AND has a
//     prevalence under our threshold, then we add that associated entity to our threat
//     indicators queue. Loop until all threat indicators have been processed.
// purge() - used to remove all references to a specified entity (eg. a filename or website)
//     from the IntelWeb disk-based data structures (forward and reverse DiskMultiMap).

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
