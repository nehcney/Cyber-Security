#include "IntelWeb.h"
#include <iostream> // needed for any I/O
#include <fstream>  // needed in addition to <iostream> for file I/O
#include <sstream>  // needed in addition to <iostream> for string stream I/O
#include <queue>
using namespace std;

IntelWeb::IntelWeb()
{
	m_fileOpen = false;
}

IntelWeb::~IntelWeb()
{
	close();
}

bool IntelWeb::createNew(const string& filePrefix, unsigned int maxDataItems)
{
	close();

	float L = 0.75;	// update to change load factor
	int numBuckets = maxDataItems * (1 / L);

	if (forward.createNew(filePrefix + "_forward_hash_table.dat", numBuckets)
		&& reverse.createNew(filePrefix + "_reverse_hash_table.dat", numBuckets))
	{
		m_fileOpen = true;
		return true;
	}

	close();
	return false;
}

bool IntelWeb::openExisting(const string& filePrefix)
{
	close();

	if (forward.openExisting(filePrefix + "_forward_hash_table.dat")
		&& reverse.openExisting(filePrefix + "_reverse_hash_table.dat"))
	{
		m_fileOpen = true;
		return true;
	}

	close();
	return false;
}

void IntelWeb::close()
{
	forward.close();
	reverse.close();
}

bool IntelWeb::ingest(const string& telemetryFile)
{
	if (!m_fileOpen)
		return false;

	// Open the file for input
	ifstream inf(telemetryFile);
	if (!inf)
		return false;

	string line;
	while (getline(inf, line))
	{
		istringstream iss(line);
		string context, from, to;
		if (!(iss >> context >> from >> to))
			continue; // line has bad formatting, skip

		// Insert into respective diskmultimaps
		if (!forward.insert(from, to, context)
			|| !reverse.insert(to, from, context))
			return false;
	}
	return true;
}

unsigned int IntelWeb::crawl(const vector<string>& indicators,
	unsigned int minPrevalenceToBeGood,
	vector<string>& badEntitiesFound,
	vector<InteractionTuple>& badInteractions) // !!! queue version
{
	if (!m_fileOpen)
		return 0;

	badEntitiesFound.clear();
	badInteractions.clear();
	set<string> badEntitiesSet;
	set<InteractionTuple> badInteractionsSet;

	// Add all indicators into a queue.
	queue<string> q;
	for (int i = 0; i < indicators.size(); ++i)
		q.push(indicators[i]);

	// Do a search through the queue of indicators
	unsigned int count = 0;
	while (!q.empty())
	{
		string cur = q.front();
		q.pop();
		DiskMultiMap::Iterator it = forward.search(cur); // 1st, search our forward hash table
		if (it.isValid())
		{
			// If we found the indicator in our data, insert the indicator into our set
			// of badEntitiesFound. If it was already in our set, do nothing.
			pair<set<string>::iterator, bool> ret;
			ret = badEntitiesSet.insert(cur);
			if (ret.second) // insert succeeded, item is unique
			{
				count++;
				// Now we do a check for any newly associated entities. Because our indicator
				// is always in the first slot, "key", and our third slot is always "context",
				// a newly associated entity must be in the second slot, "value".
				for (; it.isValid(); ++it)
				{
					MultiMapTuple m = *it;
					// Add each interaction to our badInteractions set if unique.
					badInteractionsSet.insert(toInteractionTuple(m, true));
					if (prevalenceUnderThreshold(m.value, minPrevalenceToBeGood))
						// New associated entity has a P-value below our threshold, and it
						// was successfully inserted to our set (which means it's unique).
						// New entity is now an indicator! Add to queue.
						q.push(m.value);
				}
				it = reverse.search(cur); // now loop through the reverse hash
				for (; it.isValid(); ++it)
				{
					MultiMapTuple m = *it;
					badInteractionsSet.insert(toInteractionTuple(m, false));
					if (prevalenceUnderThreshold(m.value, minPrevalenceToBeGood))
						q.push(m.value);
				}
			}
		}
		it = reverse.search(cur); // 2nd, search our reverse hash table
		if (it.isValid())
		{
			pair<set<string>::iterator, bool> ret;
			ret = badEntitiesSet.insert(cur);
			if (ret.second) // insert succeeded, item is unique
			{
				count++;
				for (; it.isValid(); ++it)
				{
					MultiMapTuple m = *it;
					badInteractionsSet.insert(toInteractionTuple(m, false));
					if (prevalenceUnderThreshold(m.value, minPrevalenceToBeGood))
						q.push(m.value);
				}
				it = forward.search(cur); // now loop through the forward hash
				for (; it.isValid(); ++it)
				{
					MultiMapTuple m = *it;
					badInteractionsSet.insert(toInteractionTuple(m, true));
					if (prevalenceUnderThreshold(m.value, minPrevalenceToBeGood))
						q.push(m.value);
				}
			}
		}
	}

	// Copy over the ordered entities/interactions into the vectors
	for (set<string>::iterator it = badEntitiesSet.begin();	it != badEntitiesSet.end(); ++it)
		badEntitiesFound.push_back(*it);
	for (set<InteractionTuple>::iterator it = badInteractionsSet.begin(); it != badInteractionsSet.end(); ++it)
		badInteractions.push_back(*it);

	return count;
}

bool IntelWeb::purge(const string& entity)
{
	bool purged = false;
	DiskMultiMap::Iterator it;
	for (it = forward.search(entity); it.isValid(); ++it)
	{
		purged = true;
		MultiMapTuple m = *it;
		forward.erase(m.key, m.value, m.context);
		reverse.erase(m.value, m.key, m.context);
		// Account for child-creating-parent situations
		forward.erase(m.value, m.key, m.context);
		reverse.erase(m.key, m.value, m.context);
	}
	for (it = reverse.search(entity); it.isValid(); ++it)
	{
		purged = true;
		MultiMapTuple m = *it;
		reverse.erase(m.key, m.value, m.context);
		forward.erase(m.value, m.key, m.context);
		// Account for child-creating-parent situations
		reverse.erase(m.value, m.key, m.context);
		forward.erase(m.key, m.value, m.context);
	}
	return purged;
}


/////////////////////////////////
//	Helper Functions
/////////////////////////////////

bool IntelWeb::prevalenceUnderThreshold(const string& key, unsigned int threshold)
{
	DiskMultiMap::Iterator it;
	unsigned int i = 0;
	for (it = forward.search(key); it.isValid(); ++it, ++i)
		if (i >= threshold)
			return false;
	for (it = reverse.search(key); it.isValid(); ++it, ++i)
		if (i >= threshold)
			return false;
	return i >= threshold ? false : true;
}

InteractionTuple IntelWeb::toInteractionTuple(const MultiMapTuple& m, bool forward)
{
	return forward ? InteractionTuple(m.key, m.value, m.context) 
		: InteractionTuple(m.value, m.key, m.context);
}
