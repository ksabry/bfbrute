#pragma once

#include <unordered_set>

template<uint_fast32_t data_size>
struct LookupData
{
public:
	LookupData(int_fast32_t dataIndex, uint8_t * data)
	{
		this->dataIndex = dataIndex;
		memcpy(this->data, data, data_size);
	}
	int_fast32_t dataIndex;
	uint8_t data[data_size];

	bool operator==(const LookupData<data_size>& other) const
	{
		if (dataIndex != other.dataIndex)
			return false;
		for (int i = 0; i < data_size; i++)
			if (data[i] != other.data[i])
				return false;
		return true;
	}
};

namespace std
{
	template<uint_fast32_t data_size>
	struct hash<LookupData<data_size>>
	{
	public:
		size_t operator()(const LookupData<data_size>& data) const
		{
			size_t result = static_cast<size_t>(data.dataIndex);
			for (int i = 0; i < data_size; i++)
			{
				uint_fast32_t offset = (i + 5) % (8 * data_size);
				result ^= static_cast<size_t>(data.data[i]) << offset;
				result ^= static_cast<size_t>(data.data[i]) >> (8 * data_size - offset);
			}
			return result;
		}
	};
}