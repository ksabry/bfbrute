#pragma once

#include "DataCache.h"
#include "ModDivisionTable.h"
#include "LinearIterator.h"

#define MAX_ITERS 1000
#define MAX_FIRST_SIZE 1
// #define ONLY_UU
// #define SINGULAR_INNER_LOOP
// #define NO_LEFT_DATA

template<uint_fast32_t data_size, uint_fast32_t cache_data_size, uint_fast32_t cache_size, uint_fast32_t max_program_size = 32>
class LLRR_ProgramIterator
{
public:
	LLRR_ProgramIterator()
	{
	}
	~LLRR_ProgramIterator()
	{
	}

private:
	DataCache<cache_data_size, cache_size>* cache;
	ModDivisionTable* divisionTable;

public:
	void SetCache(DataCache<cache_data_size, cache_size>* cache)
	{
		this->cache = cache;
		for (uint_fast32_t i = 0; i < 4; i++)
		{
			iterators[i].SetCache(cache);
		}
	}

	void SetDivisionTable(ModDivisionTable* divisionTable)
	{
		this->divisionTable = divisionTable;
	}

private:
	uint_fast32_t threadDelta;
	uint_fast32_t programSize;

	LinearIterator<cache_data_size, cache_size, max_program_size> iterators[4];
	int_fast32_t iteratorIdx;
	int_fast32_t iteratorSizes[4];
	int_fast32_t remainingSize;

public:
	void Start(uint_fast32_t programSize, uint_fast32_t threadOffset, uint_fast32_t threadDelta)
	{
		this->programSize = programSize;
		this->threadDelta = threadDelta;

		memset(iteratorSizes, 0, 4 * sizeof(int_fast32_t));
		remainingSize = programSize - 6;
		iteratorSizes[0] = 1;
		iteratorSizes[2] = 1;
		iteratorSizes[3] = remainingSize;

		if (threadOffset > 0) NextIteratorSizes(threadOffset);
		iteratorIdx = 0;
		iterators[0].Start(iteratorSizes[0]);
	}

	bool Next()
	{
		while (!NextIterators())
		{
			if (!NextIteratorSizes(threadDelta)) return false;
			iteratorIdx = 0;
			iterators[0].Start(iteratorSizes[0]);
		}
		return true;
	}

private:
	bool NextIterators()
	{
		while (iteratorIdx >= 0)
		{
			assert(iteratorIdx < 4);
			if (iterators[iteratorIdx].Next())
			{
				if (iteratorIdx == 3) return true;

#ifdef SINGULAR_INNER_LOOP
				if (iteratorIdx == 2)
				{
					auto& data = iterators[2].Data();
					if (
						data.start != data.end - 1 || 
						(data.data[data.start + cache_data_size / 2] != 1 && 
						 data.data[data.start + cache_data_size / 2] != 255) )
					{
						continue;
					}
				}
#endif
				iteratorIdx++;
				iterators[iteratorIdx].Start(iteratorSizes[iteratorIdx]);
			}
			else
			{
				iteratorIdx--;
			}
		}
		return false;
	}

	bool NextIteratorSizes(uint_fast32_t count)
	{
		int_fast32_t iteratorSizesIdx;
	restart:;
		iteratorSizesIdx = 2;
		while (iteratorSizesIdx >= 0)
		{
			if (remainingSize == 0)
			{
				remainingSize += iteratorSizes[iteratorSizesIdx];
				iteratorSizes[iteratorSizesIdx] = 0;
				if (iteratorSizesIdx == 2)
				{
					iteratorSizes[iteratorSizesIdx] = 1;
					remainingSize--;
				}
				iteratorSizesIdx--;
			}
			else
			{
#ifdef MAX_FIRST_SIZE
				if (iteratorSizesIdx == 0 && iteratorSizes[0] >= MAX_FIRST_SIZE) return false;
#endif
				iteratorSizes[iteratorSizesIdx]++;
				remainingSize--;
				if (remainingSize == 0) goto restart;

				iteratorSizes[3] = remainingSize;
				if (--count == 0) return true;
				else goto restart;
			}
		}
		return false;
	}

private:
	uint8_t data[data_size + 2 * cache_data_size];
	uint_fast32_t dataIdx;
	
public:
	bool Execute(const char* initialData, const uint_fast32_t initialDataSize)
	{
		auto& dataFirst = iterators[0].Data();
		auto& dataLeft = iterators[1].Data();
		auto& dataInner = iterators[2].Data();
		auto& dataRight = iterators[3].Data();
		constexpr uint_fast32_t dataZeroIdx = cache_data_size / 2;
		
		// TODO: see if bounds is faster
		dataIdx = data_size / 2 + cache_data_size;
		memset(data, 0, data_size + 2 * cache_data_size);
		memcpy(data + dataIdx, initialData, initialDataSize);

		for (int_fast32_t i = dataFirst.start; i < dataFirst.end; i++)
		{
			data[dataIdx + i] += dataFirst.data[dataZeroIdx + i];
		}
		// Don't check result in trivial case
		if (data[dataIdx] == 0) return false;

#ifdef ONLY_UU
		if (dataLeft.idx + dataRight.idx == 0 || dataInner.idx == 0) return false;
		return Execute_UO_UI();
#else
		if (dataInner.idx == 0)
		{
			if (dataLeft.idx + dataRight.idx == 0) return Execute_BO_BI();
			else                                   return Execute_UO_BI();
		}
		else 
		{
			return Execute_UI();
		}
#endif
	}

private:
	bool Execute_UI()
	{
		auto& dataLeft = iterators[1].Data();
		auto& dataInner = iterators[2].Data();
		auto& dataRight = iterators[3].Data();
		constexpr uint_fast32_t dataZeroIdx = cache_data_size / 2;

		uint_fast32_t iters = MAX_ITERS;
		while (iters--)
		{
			if (data[dataIdx] == 0) return true;

			for (int_fast32_t i = dataLeft.start; i < dataLeft.end; i++)
			{
				data[dataIdx + i] += dataLeft.data[dataZeroIdx + i];
			}
			dataIdx += dataLeft.idx;
#ifdef NO_LEFT_DATA
			if (dataIdx < data_size / 2 + cache_data_size || dataIdx >= data_size + cache_data_size) return false;
#else
			if (dataIdx < cache_data_size || dataIdx >= data_size + cache_data_size) return false;
#endif
			while (data[dataIdx] != 0)
			{
				for (int_fast32_t i = dataInner.start; i < dataInner.end; i++)
				{
					data[dataIdx + i] += dataInner.data[dataZeroIdx + i];
				}	
				dataIdx += dataInner.idx;
#ifdef NO_LEFT_DATA
				if (dataIdx < data_size / 2 + cache_data_size || dataIdx >= data_size + cache_data_size) return false;
#else
				if (dataIdx < cache_data_size || dataIdx >= data_size + cache_data_size) return false;
#endif
			}

			for (int_fast32_t i = dataRight.start; i < dataRight.end; i++)
			{
				data[dataIdx + i] += dataRight.data[dataZeroIdx + i];
			}
			dataIdx += dataRight.idx;
#ifdef NO_LEFT_DATA
			if (dataIdx < data_size / 2 + cache_data_size || dataIdx >= data_size + cache_data_size) return false;
#else
			if (dataIdx < cache_data_size || dataIdx >= data_size + cache_data_size) return false;
#endif
		}
		return false;
	}

	// Balanced Outer Balanced Inner
	bool Execute_BO_BI()
	{
		auto& dataLeft = iterators[1].Data();
		auto& dataInner = iterators[2].Data();
		auto& dataRight = iterators[3].Data();
		constexpr uint_fast32_t dataZeroIdx = cache_data_size / 2;

		int_fast32_t k = dataLeft.idx;
		uint_fast32_t startIdx = Min(dataLeft.start, Min(dataInner.start + k, dataRight.start + k));
		uint_fast32_t endIdx = Max(dataLeft.end, Max(dataInner.end + k, dataRight.end + k));

#ifdef NO_LEFT_DATA
		if (startIdx + dataIdx < data_size / 2 + cache_data_size) return false;
#endif

		int q0 = divisionTable->Get(
			data[dataIdx + k] + dataLeft.data[dataZeroIdx + k], 
			dataInner.data[dataZeroIdx]);
		// Initial run of inner loop is infinite
		if (q0 == -1) return false; 

		for (int_fast32_t i = startIdx; i < endIdx; i++)
		{
			data[dataIdx + i] += 
				dataLeft.data[dataZeroIdx + i] + 
				dataRight.data[dataZeroIdx + i - k] + 
				static_cast<uint8_t>(q0) * dataInner.data[dataZeroIdx + i - k];
		}

		int q = divisionTable->Get(
			dataLeft.data[dataZeroIdx + k] + dataRight.data[dataZeroIdx], 
			dataInner.data[dataZeroIdx]);
		// Inner loop is infinite
		if (q == -1) return false;

		uint8_t denom = 
			dataLeft.data[dataZeroIdx] + 
			dataRight.data[dataZeroIdx - k] + 
			static_cast<uint8_t>(q) * dataInner.data[dataZeroIdx - k];
		int n = divisionTable->Get(data[dataIdx], denom);
		// Outer loop is infinite
		if (n == -1) return false;

		for (int_fast32_t i = startIdx; i < endIdx; i++)
		{
			uint8_t term = 
				dataLeft.data[dataZeroIdx + i] + 
				dataRight.data[dataZeroIdx + i - k] + 
				static_cast<uint8_t>(q) * dataInner.data[dataZeroIdx + i - k];
			data[dataIdx + i] += term * static_cast<uint8_t>(n);
		}
		return true;
	}

	// Balanced Outer Unbalanced Inner
	bool Execute_UO_BI()
	{
		auto& dataLeft = iterators[1].Data();
		auto& dataInner = iterators[2].Data();
		auto& dataRight = iterators[3].Data();
		constexpr uint_fast32_t dataZeroIdx = cache_data_size / 2;

		int_fast32_t k = dataLeft.idx;
		uint_fast32_t startIdx = Min(dataLeft.start, Min(dataInner.start + k, dataRight.start + k));
		uint_fast32_t endIdx = Max(dataLeft.end, Max(dataInner.end + k, dataRight.end + k));

		// Directly simulate, will either reach data boundary or complete
		while (data[dataIdx] != 0)
		{
#ifdef NO_LEFT_DATA
			if (dataIdx + startIdx < data_size / 2 + cache_data_size) return false;
#endif
			int q = divisionTable->Get(data[dataIdx + k], dataInner.data[dataZeroIdx]);
			// Inner loop is infinite
			if (q == -1) return false;
			for (int_fast32_t i = startIdx; i < endIdx; i++)
			{
				data[dataIdx + i] += 
					dataLeft.data[dataZeroIdx + i] + 
					static_cast<uint8_t>(q) * dataInner.data[dataZeroIdx + i - k] +
					dataRight.data[dataZeroIdx + i - k];
			}
			dataIdx += dataLeft.idx + dataRight.idx;
#ifdef NO_LEFT_DATA
			if (dataIdx < data_size / 2 + cache_data_size || dataIdx >= data_size + cache_data_size) return false;
#else
			if (dataIdx < cache_data_size || dataIdx >= data_size + cache_data_size) return false;
#endif
		}
		return true;
	}

public:
	// NOTE: destructive
	uint_fast32_t StringDistance(const char* target, uint_fast32_t targetSize, uint_fast32_t shortCircuit)
	{
		uint_fast32_t score = 0;
		uint_fast32_t curIdx = dataIdx;
		for (uint_fast32_t charIdx = 0; charIdx < targetSize; charIdx++)
		{
			char c = target[charIdx];
			uint_fast32_t bestIdx;
			uint_fast32_t bestCharScore = 1000000;
			for (uint_fast32_t i = cache_data_size; i <= data_size + cache_data_size; i++)
			{
				uint_fast32_t charScore = abs(static_cast<int_fast32_t>(curIdx - i)) + abs(static_cast<int8_t>(c - data[i]));
				if (charScore < bestCharScore)
				{
					bestCharScore = charScore;
					bestIdx = i;
				}
			}
			score += bestCharScore;
			if (score > shortCircuit) return score;
			data[bestIdx] = c;
			curIdx = bestIdx;
		}
		return score;
	}

	uint_fast32_t StringDistanceOutput(const char* target, uint_fast32_t targetSize, char* output)
	{
		uint_fast32_t score = 0;
		uint_fast32_t curIdx = dataIdx;
		for (uint_fast32_t charIdx = 0; charIdx < targetSize; charIdx++)
		{
			char c = target[charIdx];
			uint_fast32_t bestIdx;
			uint_fast32_t bestCharScore = 1000000;
			for (uint_fast32_t i = cache_data_size; i <= data_size + cache_data_size; i++)
			{
				uint_fast32_t charScore = abs(static_cast<int_fast32_t>(curIdx - i)) + abs(static_cast<int8_t>(c - data[i]));
				if (charScore < bestCharScore)
				{
					bestCharScore = charScore;
					bestIdx = i;
				}
			}

			if (bestIdx > curIdx)
			{
				for (uint_fast32_t i = 0; i < bestIdx - curIdx; i++) *(output++) = '>';
			}
			else
			{
				for (uint_fast32_t i = 0; i < curIdx - bestIdx; i++) *(output++) = '<';
			}
			char diff = c - data[bestIdx];
			if (diff > 0)
			{
				for (uint_fast32_t i = 0; i < diff; i++) *(output++) = '+';
			}
			else
			{
				for (uint_fast32_t i = 0; i < -diff; i++) *(output++) = '-';
			}
			*(output++) = '.';

			score += bestCharScore;
			data[bestIdx] = c;
			curIdx = bestIdx;
		}

		*output = '\0';
		return score;
	}

	inline bool StartsPlus()
	{
		auto& dataFirst = iterators[0].Data();
		auto& dataLeft = iterators[1].Data();
		auto& dataInner = iterators[2].Data();
		auto& dataRight = iterators[3].Data();
		constexpr uint_fast32_t dataZeroIdx = cache_data_size / 2;

		if (dataFirst.data[dataZeroIdx] == 0 || dataFirst.idx < 0) return false;
		if (dataFirst.idx == 0)
		{
			if (dataLeft.idx < 0) return false;
			if (dataLeft.idx == 0)
			{
				if (dataInner.idx < 0) return false;
				if (dataInner.idx == 0)
				{
					if (dataRight.idx < 0) return false; 
				}
			}
		}
		return true;
	}

	char currentProgram[256];

	char* GetProgram()
	{
		char* output = currentProgram;

		strcpy(output, iterators[0].GetProgram());
		output += strlen(output);
		output[0] = '['; output++;

		strcpy(output, iterators[1].GetProgram());
		output += strlen(output);
		output[0] = '['; output++;
		
		strcpy(output, iterators[2].GetProgram());
		output += strlen(output);
		output[0] = ']'; output++;
		
		strcpy(output, iterators[3].GetProgram());
		output += strlen(output);
		output[0] = ']';
		output[1] = '\0';

		return currentProgram;
	}

	void PrintData()
	{
		for (uint_fast32_t i = cache_data_size; i <= data_size + cache_data_size; i++)
			std::cout << std::setw(4) << (int)data[i];
		std::cout << std::endl << std::string((dataIdx - cache_data_size) * 4, ' ') << "  ^" << std::endl;
	}
};
