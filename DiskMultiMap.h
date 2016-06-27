// DiskMultiMap is a disk-based multimap hash table. Uses a hand-coded Iterator
// with a small cache. Structurally, the multimap is implemented as a disk-based
// open hash table with "horizontal" and "vertical" singly-linked lists: the
// horizontal component links the different keys within the same hash bucket,
// and the vertical component links nodes with the same key. These are linked with
// the BinaryFile::Offset variables next_key and next_equal, respectively (Offset
// is an int that corresponds to the address location on disk).
//
// Each DiskMultiMap disk file contains the following information:
//   - A Header struct that includes:
//       - Number of buckets (unsigned int)
//       - Offset of the next freespace DiskNode
//           - (For this I used a separate linked list. Each time a DiskNode was 
//             "deleted", it was added to the freespace linked list)
//   - "Array" of buckets, each containing the Offset of a DiskNode's location
//   - All data that follows are the actual DiskNodes

#ifndef DISKMULTIMAP_H_
#define DISKMULTIMAP_H_

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER

#include <cstring>
#include <string>
#include "MultiMapTuple.h"
#include "BinaryFile.h"

// Global Variables
const BinaryFile::Offset CHARLIMIT = 120;

class DiskMultiMap
{
public:
	class Iterator
	{
	public:
		Iterator();
		Iterator(BinaryFile* src, BinaryFile::Offset offset = 0);
		bool isValid() const;
		Iterator &operator++();
		MultiMapTuple operator*();

	private:
		BinaryFile* m_src;
		MultiMapTuple m_cache;
		BinaryFile::Offset it_offset;
		BinaryFile::Offset cache_offset;
	};

	DiskMultiMap();
	~DiskMultiMap();
	bool createNew(const std::string& filename, unsigned int numBuckets);
	bool openExisting(const std::string& filename);
	void close();
	bool insert(const std::string& key, const std::string& value, const std::string& context);
	Iterator search(const std::string& key);
	int erase(const std::string& key, const std::string& value, const std::string& context);

private:
	struct DiskNode
	{
		DiskNode(const char* data1 = nullptr, const char* data2 = nullptr, const char* data3 = nullptr,
			BinaryFile::Offset nextKey = 0, BinaryFile::Offset nextEqual = 0)
			: next_key(nextKey), next_equal(nextEqual)
		{
			strcpy(key, data1 != nullptr ? data1 : "");
			strcpy(value, data2 != nullptr ? data2 : "");
			strcpy(context, data3 != nullptr ? data3 : "");
		}
		char key[CHARLIMIT + 1];
		char value[CHARLIMIT + 1];
		char context[CHARLIMIT + 1];
		BinaryFile::Offset next_key;
		BinaryFile::Offset next_equal;
	};

	struct Header
	{
		Header(unsigned int numBuckets = 0)
			: m_numBuckets(numBuckets), m_freespace(0) {}

		unsigned int m_numBuckets;
		BinaryFile::Offset m_freespace;
	};

	BinaryFile	bf;
	bool		m_fileOpen;
	Header		m_header;

private:
	BinaryFile::Offset getBucketOffsetFromKey(const std::string& key);
};

#endif // DISKMULTIMAP_H_
