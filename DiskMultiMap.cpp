#include "DiskMultiMap.h"
#include <functional>
#include <vector>
using namespace std;

DiskMultiMap::DiskMultiMap()
	: m_fileOpen(false) {}

DiskMultiMap::~DiskMultiMap()
{
	close();
}

bool DiskMultiMap::createNew(const string& filename, unsigned int numBuckets)
{
	close();

	// Create empty file
	if (!bf.createNew(filename))
		return false;

	// Create necessary header in the file
	m_header.m_numBuckets = numBuckets;
	if (!bf.write(m_header, 0))
		return false;

	// Create "array" of numBuckets buckets, intialized to 0
	BinaryFile::Offset* array = new BinaryFile::Offset[numBuckets]();
	if (!bf.write(reinterpret_cast<const char*>(array), sizeof(size_t) * numBuckets, sizeof(m_header)))
		return false;
	delete [] array;
	
	// Update private member variables
	m_fileOpen = true;
	return true;
}

bool DiskMultiMap::openExisting(const string& filename)
{
	close();
	if (!bf.openExisting(filename))
		return false;

	// Update private member variables
	if (!bf.read(m_header, 0))
		return false;
	m_fileOpen = true;
	return true;
}

void DiskMultiMap::close()
{
	bf.close();
}

bool DiskMultiMap::insert(const string& key, const string& value, const string& context)
{
	if (key.size() > CHARLIMIT || value.size() > CHARLIMIT || value.size() > CHARLIMIT
		|| !m_fileOpen)
		return false;
	
	// Find first available space
	BinaryFile::Offset firstOpen;
	if (!m_header.m_freespace) // == 0
		firstOpen = bf.fileLength();
	else
	{
		firstOpen = m_header.m_freespace;
		// Since we will be overwriting a previously used node,
		// we need to update the freespace list by pointing its
		// head at the next node on the freespace list.
		DiskNode temp;
		bf.read(temp, m_header.m_freespace); // store 1st free node to temp
		m_header.m_freespace = temp.next_key; // update new head to next free node
		bf.write(m_header, 0); // write to file
	}

	// Write new DiskNode at first available freespace.
	// Find the bucket offset using the built-in hash function.
	// 1) If bucket is empty, bucket offset points to new DiskNode, 
	// and set new next_key and next_equal to 0.
	const char* key_c = key.c_str();
	const char* value_c = value.c_str();
	const char* context_c = context.c_str();
	BinaryFile::Offset bucketOffset = getBucketOffsetFromKey(key);
	BinaryFile::Offset bucketValue;
	bf.read(bucketValue, bucketOffset);
	if (!bucketValue) // empty bucket
	{
		bf.write(DiskNode(key_c, value_c, context_c), firstOpen);
		bf.write(firstOpen, bucketOffset);
	}
	// 2) If bucket is non-empty, search through the nodes until 
	// a matching key has been found, or next_key == 0. 
	//		a) If a matching key has been found, add new DiskNode,
	//		set next_equal == cur.next_equal,
	//		and cur.next_equal = new DiskNode.
	//		b) If no matching key can be found, add new DiskNode 
	//		to the front of the list, assign its next_key to the
	//		node pointed to by bucket, then update bucket to the
	//		new node.
	else
	{
		BinaryFile::Offset curOffset = bucketValue; 
		DiskNode cur;
		while (curOffset) // valid node
		{
			bf.read(cur, curOffset);
			if (strcmp(cur.key, key_c) == 0)
				break;
			curOffset = cur.next_key;
		}
		if (curOffset) // found a matching key!
		{
			bf.write(DiskNode(key_c, value_c, context_c, 0, cur.next_equal), firstOpen);
			cur.next_equal = firstOpen;
			bf.write(cur, curOffset);
		}
		else
		{
			bf.write(DiskNode(key_c, value_c, context_c, bucketValue), firstOpen);
			bf.write(firstOpen, bucketOffset);
		}
	}
	return true;
}

DiskMultiMap::Iterator DiskMultiMap::search(const string& key)
{
	DiskMultiMap::Iterator nothing;
	if (!m_fileOpen || key.size() > CHARLIMIT)
		return nothing;
	
	BinaryFile::Offset bucketOffset = getBucketOffsetFromKey(key);
	BinaryFile::Offset bucketValue;
	bf.read(bucketValue, bucketOffset);
	if (bucketValue) // bucket not empty
	{
		const char* key_c = key.c_str();
		BinaryFile::Offset curOffset = bucketValue;
		DiskNode cur;
		while (curOffset) // valid node
		{
			bf.read(cur, curOffset);
			if (strcmp(cur.key, key_c) == 0)
				break;
			curOffset = cur.next_key;
		}
		if (curOffset) // found a matching key!
		{
			DiskMultiMap::Iterator temp(&bf, curOffset);
			return temp;
		}
	}
	return nothing;
}

int DiskMultiMap::erase(const string& key, const string& value, const string& context)
{
	if (key.size() > CHARLIMIT || value.size() > CHARLIMIT || context.size() > CHARLIMIT
		|| !m_fileOpen)
		return 0;

	const char* key_c = key.c_str();
	const char* value_c = value.c_str();
	const char* context_c = context.c_str();
	BinaryFile::Offset bucketOffset = getBucketOffsetFromKey(key);
	BinaryFile::Offset bucketValue;
	bf.read(bucketValue, bucketOffset);
	if (!bucketValue) // key not found
		return 0;

	BinaryFile::Offset curOffset = bucketValue, prevOffset = 0;
	DiskNode cur, prev;
	while (curOffset) // going through the linked list (horizontal)
	{
		bf.read(cur, curOffset);
		if (strcmp(cur.key, key_c) == 0)
			break;
		prevOffset = curOffset;
		curOffset = cur.next_key;
		prev = cur;
	}
	if (!curOffset) // key not found
		return 0;

	// Only the key has been found so far! Now loop through the linked list with matching keys (vertical).
	int numErased = 0;
	while (curOffset)
	{
		if (strcmp(cur.value, value_c) == 0 && strcmp(cur.context, context_c) == 0) // MATCH FOUND!
		{
			DiskNode next;
			// Update all necessary "pointers"
			if (bucketValue == curOffset) // 1st node of bucket
			{
				if (cur.next_equal)
				{
					// Since only the 1st node of the vertical linked list has a proper next_key pointer,
					// if we delete the first node, we need to update the 2nd node's next_key pointer
					bf.read(next, cur.next_equal);
					next.next_key = cur.next_key;
					bf.write(next, cur.next_equal);
					bf.write(cur.next_equal, bucketOffset); // update where bucket points to
				}
				bf.write(cur.next_key, bucketOffset); // update where bucket points to
			}
			else // not the 1st node of bucket
			{
				if (prev.next_key == curOffset) // cur is 1st node of a vertical list; prev and cur on SEPARATE vertical lists
				{
					if (cur.next_equal) // see case when 1st node of bucket above
					{
						bf.read(next, cur.next_equal);
						next.next_key = cur.next_key;
						bf.write(next, cur.next_equal);
					}
					prev.next_key = cur.next_equal ? cur.next_equal : cur.next_key;
				}
				else // cur is NOT 1st node of a vertical list; prev and cur are on SAME vertical list
					prev.next_equal = cur.next_equal;
				bf.write(prev, prevOffset);
			}

			// Add deleted node (cur) to our freespace list. cur is now at the 
			// front of our freespace list, and its next offset (using next_key)
			// must be the previous front of our freespace list.
			cur.next_key = m_header.m_freespace;
			m_header.m_freespace = curOffset;
			bf.write(cur, curOffset);
			bf.write(m_header, 0);

			numErased++; // update number of erased items
		}
		else // if no match found, update prevOffset and prev to cur
		{
			prevOffset = curOffset;
			prev = cur;
		}
		curOffset = cur.next_equal; // advance to next_equal
		if (curOffset)
			bf.read(cur, curOffset);
	}
	
	return numErased;
}


/////////////////////////////////
//	Iterator Implementations
/////////////////////////////////

DiskMultiMap::Iterator::Iterator()
{
	m_src = nullptr;
	it_offset = 0;
	cache_offset = 0;
}

DiskMultiMap::Iterator::Iterator(BinaryFile* src, BinaryFile::Offset offset)
{
	m_src = src;
	it_offset = offset;
	cache_offset = 0;
}

bool DiskMultiMap::Iterator::isValid() const
{
	return it_offset != 0;
}

DiskMultiMap::Iterator &DiskMultiMap::Iterator::operator++()
{
	if (!isValid())
		return *this;

	DiskNode temp;
	m_src->read(temp, it_offset);
	it_offset = temp.next_equal;
	return *this;
}

MultiMapTuple DiskMultiMap::Iterator::operator*()
{
	if (!isValid())
	{
		m_cache.key = "";
		m_cache.value = "";
		m_cache.context = "";
		cache_offset = it_offset;
	}
	else if (cache_offset != it_offset)
	{
		DiskNode temp;
		m_src->read(temp, it_offset);
		m_cache.key = temp.key;
		m_cache.value = temp.value;
		m_cache.context = temp.context;
		cache_offset = it_offset;
	}
	return m_cache;
}


/////////////////////////////////
//	Helper Functions
/////////////////////////////////

BinaryFile::Offset DiskMultiMap::getBucketOffsetFromKey(const string& key)
{
	hash<string> hashValue;
	return hashValue(key) % m_header.m_numBuckets * sizeof(int32_t) + sizeof(m_header);
}