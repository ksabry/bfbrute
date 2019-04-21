#pragma once

#include <iostream>
#include <fstream>

#include "DataCache.h"
#include "Util.h"

template<uint_fast32_t data_size, uint_fast32_t cache_size, uint_fast32_t max_program_size = 32>
class LinearIterator
{
public:
	LinearIterator()
		: cache(nullptr)
	{
	}
	~LinearIterator()
	{
	}

private:
	DataCache<data_size, cache_size>* cache;
	uint_fast32_t stackSize;
	uint_fast32_t firstCacheSize;

	static const uint_fast32_t max_stack_size = (max_program_size - 1) / cache_size + 1;

	uint_fast32_t indexStack[max_stack_size];
	bool balancedStack[max_stack_size];
	AlignedData<data_size> dataStack[max_stack_size];

	bool first;

public:
	inline void SetCache(DataCache<data_size, cache_size>* cache)
	{
		this->cache = cache;
	}

	void Start(uint_fast32_t programSize)
	{
		stackSize = programSize == 0 ? 1 : (programSize - 1) / cache_size + 1;
		firstCacheSize = programSize == 0 ? 0 : (programSize - 1) % cache_size + 1;
	
		indexStack[0] = cache->bordersBalanced[firstCacheSize];
		balancedStack[0] = true;
		for (uint_fast32_t i = 1; i < stackSize; i++)
		{
			indexStack[i] = cache->bordersBalanced[cache_size];
			balancedStack[i] = true;
		}
		first = true;
	}

	bool Next()
	{
		if (first)
		{
			first = false;
			for (int i = stackSize - 1; i >= 0; i--) CalcData(i);
			return true;
		}
		return Inc();
	}

	inline AlignedData<data_size>& Data()
	{
		return dataStack[0];
	}
	
	inline bool IsBalanced() const
	{
		return dataStack[0].idx == 0;
	}

private:
	bool Inc()
	{
		if (IncSingle(indexStack[0], balancedStack[0], firstCacheSize))
		{
			CalcData(0);
			return true;
		}
		for (uint_fast32_t i = 1; i < stackSize; i++)
		{
			if (IncSingle(indexStack[i], balancedStack[i], cache_size))
			{
				for (int j = i; j >= 0; j--) CalcData(j);
				return true;
			}
		}
		return false;
	}

	bool IncSingle(uint_fast32_t& index, bool& balanced, uint_fast32_t frame_size)
	{
		uint_fast32_t end = balanced ? cache->bordersBalanced[frame_size + 1] : cache->bordersUnbalanced[frame_size + 1];
		
		if (++index >= end)
		{
			if (balanced)
			{
				if (firstCacheSize == 0) return false;
				index = cache->bordersUnbalanced[frame_size];
				balanced = false;
				return true;
			}
			else
			{
				index = cache->bordersBalanced[frame_size];
				balanced = true;
				return false;
			}
		}
		return true;
	}

	void CalcData(uint_fast32_t idx)
	{
		AlignedData<data_size>& data = balancedStack[idx] ? cache->dataBalanced[indexStack[idx]] : cache->dataUnbalanced[indexStack[idx]];
		if (idx == stackSize - 1)
		{
			dataStack[idx] = data;
		}
		else
		{
			AddData<data_size>(dataStack[idx], dataStack[idx + 1], data);
		}
	}

public:
	char currentProgram[256];

	char* GetProgram()
	{
		uint_fast32_t pIndex = 0;

		for (int i = static_cast<int>(stackSize - 1); i >= 0; i--)
		{
			uint_fast32_t idx = indexStack[i];
			uint_fast32_t size = i == 0 ? firstCacheSize : cache_size;
			
			char* part;
			if (balancedStack[i])
				part = cache->programsBalanced.data() + (idx * cache_size);
			else
				part = cache->programsUnbalanced.data() + (idx * cache_size);
			
			memcpy(currentProgram + pIndex, part, size * sizeof(char));
			pIndex += size;
		}
		currentProgram[pIndex] = '\0';
		return currentProgram;
	}

	inline char FirstChar()
	{
		return balancedStack[0] ?
			cache->programsBalanced[indexStack[0] * cache_size] :
			cache->programsUnbalanced[indexStack[0] * cache_size];
	}
};

template<uint_fast32_t data_size, uint_fast32_t cache_size, uint_fast32_t max_program_size>
struct LinearIterator_RecoverData
{
	uint_fast32_t stackSize;
	uint_fast32_t firstCacheSize;

	static const uint_fast32_t max_stack_size = (max_program_size - 1) / cache_size + 1;

	uint_fast32_t indexStack[max_stack_size];
	bool balancedStack[max_stack_size];
	AlignedData<data_size> dataStack[max_stack_size];

	bool first;
};