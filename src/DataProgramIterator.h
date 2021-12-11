#pragma once

#include <assert.h>
#include "LinearIterator.h"
#include "ModDivisionTable.h"

// #define SINGLE_BRACKET_HIERARCHY
// #define MAX_BRACKET_DEPTH 2
#define MAX_JUMPS 20000
#define SHORT_CIRCUIT_LINEAR_SINGULAR
#define INITIAL_ZERO
// #define INITIAL_DATA_SYMMETRIC
// #define NO_TRAILING_LINEAR_PROGRAM
// #define MAX_FIRST_SIZE 2

template<uint_fast32_t data_size, uint_fast32_t cache_data_size, uint_fast32_t cache_size, uint_fast32_t max_program_size = 32>
class DataProgramIterator
{
public:
	DataProgramIterator()
	{
	}
	~DataProgramIterator()
	{
	}

	void SetCache(DataCache<cache_data_size, cache_size>* cache)
	{
		this->cache = cache;
		for (uint_fast32_t i = 0; i < max_program_size; i++)
		{
			iterators[i].SetCache(cache);
		}
	}

	void SetDivisionTable(ModDivisionTable* divisionTable)
	{
		this->divisionTable = divisionTable;
	}

private:
	DataCache<cache_data_size, cache_size>* cache;

	uint_fast32_t threadOffset, threadDelta;
	uint_fast32_t programSize;

	uint_fast64_t currentBracketCount;
	uint_fast64_t currentCount;

	LinearIterator<cache_data_size, cache_size, max_program_size> iterators[max_program_size];
	uint_fast32_t iteratorCount;
	
	int_fast32_t iteratorSizes[max_program_size];
	int_fast32_t remainingSize;
	
	enum class Bracket
	{
		EMPTY,
		LEFT,
		RIGHT
	};
	struct { Bracket bracket; uint_fast32_t depth; } brackets[max_program_size];
	struct { uint_fast32_t zero; uint_fast32_t nonzero; } jumps[max_program_size];
	uint_fast32_t leftBracketStack[max_program_size];
	int_fast32_t bracketIdx;
	
	int_fast32_t iteratorIdx;

	int_fast32_t firstIteratorWithNonZeroDataDelta;

public:
	// Estimate
	uint_fast64_t TotalCount(uint_fast32_t programSize)
	{
		uint_fast64_t result = 0;
		
		Start(programSize, 0, 1);
		while (true)
		{
			uint_fast64_t iteratorsResult = 1;
			for (uint_fast32_t i = 0; i < iteratorCount; i++)
			{
				iteratorsResult *= iterators[0].SizeCount(iteratorSizes[i]);
			}
			result += iteratorsResult;

			while (!NextBrackets())
			{
				if (!NextValidIteratorSizes(threadDelta))
				{
					return result;
				}
				bracketIdx = 0;
			}
		}
	}
	inline uint_fast64_t CurrentCount()
	{
		return currentCount;
	}

	void Start(uint_fast32_t programSize, uint_fast32_t threadOffset, uint_fast32_t threadDelta)
	{
		this->programSize = programSize;
		this->currentCount = 0;
		this->threadOffset = threadOffset;
		this->threadDelta = threadDelta;

		memset(iteratorSizes, 0, max_program_size * sizeof(int_fast32_t));

		iteratorCount = 1;
		iteratorSizes[0] = programSize;
		firstIteratorWithNonZeroDataDelta = 0;
		bool found = NextValidIteratorSizes(threadOffset);
		assert(found);

		bracketIdx = 0;
		brackets[0].bracket = Bracket::EMPTY;
		brackets[0].depth = 0;
		jumps[0].zero = jumps[0].nonzero = 1;
		
		while (!NextBrackets())
		{
			bool found = NextValidIteratorSizes(threadDelta);
			assert(found);
			bracketIdx = 0;
		}

		iteratorIdx = 0;
		iterators[0].Start(iteratorSizes[0]);
	}

	bool Next()
	{
		while (!NextIterators())
		{
			int_fast64_t iteratorsCount = 1;
			for (uint_fast32_t i = 0; i < iteratorCount; i++)
			{
				iteratorsCount *= iterators[0].SizeCount(iteratorSizes[i]);
			}
			currentCount += iteratorsCount;

			while (!NextBrackets())
			{
				if (!NextValidIteratorSizes(threadDelta))
				{
					return false;
				}
				bracketIdx = 0;
			}
			iteratorIdx = 0;
			iterators[0].Start(iteratorSizes[0]);
		}
		return true;
	}

private:
	bool NextValidIteratorSizes(uint_fast32_t count)
	{
		for (int_fast32_t c = count; c > 0; c--)
		{
			while (!NextIteratorSizes())
			{
				iteratorCount += 2;

				remainingSize = programSize - iteratorCount + 1;
				if (remainingSize <= 0) return false;

				for (uint_fast32_t i = 0; i < iteratorCount - 2; i++)
				{
					assert(i >= 0 && i < max_program_size);
					iteratorSizes[i] = 0;
				}
#ifdef INITIAL_ZERO
				iteratorSizes[0] = 1;
				remainingSize--;
#endif
				assert(iteratorCount - 2 >= 0 && iteratorCount - 2 < max_program_size);
				// Set to -1 so that after the first call to NextIteratorSizes it will be 0
				iteratorSizes[iteratorCount - 2] = -1;
				remainingSize++;
				firstIteratorWithNonZeroDataDelta = 0;
			}
		}
		return true;
	}

	bool NextIterators()
	{
		if (iteratorIdx == iteratorCount - 1 && !lastExecutionSuccessful)
		{
			iteratorIdx = lastExecutionMaxProgramIdx;
		}

		while (iteratorIdx >= 0)
		{
			assert(iteratorIdx < max_program_size);
			if (NextSingleIterator())
			{
				if (iteratorIdx == iteratorCount - 1)
				{
					return true;
				}
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

	bool NextSingleIterator()
	{
		while (iterators[iteratorIdx].Next())
		{
#if defined INITIAL_ZERO || defined INITIAL_DATA_SYMMETRIC
			// Check that the first iterator has a right delta
			if (iteratorIdx <= firstIteratorWithNonZeroDataDelta)
			{
				int_fast32_t dataDelta = iterators[iteratorIdx].DataDelta();
				if (dataDelta < 0)
				{
					continue;
				}
				else if (dataDelta > 0)
				{
					firstIteratorWithNonZeroDataDelta = iteratorIdx;
				}
				else
				{
					firstIteratorWithNonZeroDataDelta = iteratorIdx + 1;
				}
			}
#endif

#ifdef INITIAL_ZERO
			// Do not permit guaranteed 0 after initial iterator or meaningless movements at the beginning
			auto& data = iterators[0].Data();
			if (iteratorIdx == 0 && (data.data[data.idx + cache_data_size / 2] == 0 || data.data[cache_data_size / 2] == 0))
			{
				continue;
			}
#endif

			// Do not permit [-] (since [+] is equivalent)
			if (
				iteratorSizes[iteratorIdx] == 1
				&& iteratorIdx > 0
				&& brackets[iteratorIdx - 1].bracket == Bracket::LEFT
				&& brackets[iteratorIdx].bracket == Bracket::RIGHT
				&& iterators[iteratorIdx].Data().data[cache_data_size / 2] == 255
			)
			{
				continue;
			}

			return true;
		}
		return false;
	}

	bool NextBrackets()
	{
		if (iteratorCount == 1)
		{
			return false;
		}

		while (bracketIdx >= 0)
		{
			uint_fast32_t remainingBrackets = iteratorCount - bracketIdx - 1;
			bool left_valid = bracketIdx == 0 ? true : remainingBrackets >= brackets[bracketIdx - 1].depth + 2;
			bool right_valid = bracketIdx == 0 ? false : brackets[bracketIdx - 1].depth > 0;

			// EMPTY -> LEFT -> RIGHT -> EMPTY
			assert(bracketIdx >= 0 && bracketIdx < max_program_size);
			if (brackets[bracketIdx].bracket == Bracket::EMPTY)
			{
				if (left_valid)
				{
					if (!SetBracketLeft()) continue;
					bracketIdx++;
				}
				else if (right_valid)
				{
					if (!SetBracketRight()) continue;
					if (bracketIdx == iteratorCount - 2) goto success;
					bracketIdx++;
				}
				else
				{
					bracketIdx--;
				}
			}
			else if (brackets[bracketIdx].bracket == Bracket::LEFT)
			{
				if (right_valid)
				{
					if (!SetBracketRight()) continue;
					if (bracketIdx == iteratorCount - 2) goto success;
					bracketIdx++;
				}
				else
				{
					brackets[bracketIdx].bracket = Bracket::EMPTY;
					bracketIdx--;
				}
			}
			else
			{
				brackets[bracketIdx].bracket = Bracket::EMPTY;
				bracketIdx--;
			}
		}
		return false;
	
	success:;

		SetJumps();
		return true;
	}

	inline bool SetBracketLeft()
	{
		assert(bracketIdx >= 0 && bracketIdx < max_program_size);
		brackets[bracketIdx].bracket = Bracket::LEFT;
		brackets[bracketIdx].depth = bracketIdx == 0 ? 1 : brackets[bracketIdx - 1].depth + 1;

		// allow .[ [[
		// disallow ][
		return (
			iteratorSizes[bracketIdx] > 0
			|| bracketIdx == 0 // INITIAL_ZERO will stop this from happening if necessary in another location
			|| brackets[bracketIdx - 1].bracket != Bracket::RIGHT
		)
#ifdef MAX_BRACKET_DEPTH
		&& brackets[bracketIdx].depth <= MAX_BRACKET_DEPTH
#endif
#ifdef SINGLE_BRACKET_HIERARCHY
		&& (bracketIdx == 0 || brackets[bracketIdx - 1].depth > 0)
#endif 
		;
	}

	inline bool SetBracketRight()
	{
		brackets[bracketIdx].bracket = Bracket::RIGHT;
		brackets[bracketIdx].depth = brackets[bracketIdx - 1].depth - 1;

		// disallow .] [] ]]
		return iteratorSizes[bracketIdx] > 0;
	}

	void SetJumps()
	{
		for (uint_fast32_t i = 0; i < iteratorCount - 1; i++)
		{
			if (brackets[i].bracket == Bracket::LEFT)
			{
				leftBracketStack[brackets[i].depth] = i;
			}
			else if (brackets[i].bracket == Bracket::RIGHT)
			{
				uint_fast32_t lbracket = leftBracketStack[brackets[i - 1].depth];
				jumps[i].zero = jumps[lbracket].zero = i + 1;
				jumps[i].nonzero = jumps[lbracket].nonzero = lbracket + 1;
			}
			else
			{
				jumps[i].zero = jumps[i].nonzero = i + 1;
			}
		}
		jumps[iteratorCount - 1].zero = jumps[iteratorCount - 1].nonzero = iteratorCount;
	}

	bool NextIteratorSizes()
	{
		int_fast32_t iteratorSizesIdx;
	restart:;
		iteratorSizesIdx = iteratorCount - 2;
		while (iteratorSizesIdx >= 0)
		{
			if (remainingSize == 0)
			{
				remainingSize += iteratorSizes[iteratorSizesIdx];
				assert(iteratorSizesIdx >= 0 && iteratorSizesIdx < max_program_size);
				iteratorSizes[iteratorSizesIdx] = 0;
				iteratorSizesIdx--;
			}
			else
			{
				assert(iteratorSizesIdx >= 0 && iteratorSizesIdx < max_program_size);
#ifdef MAX_FIRST_SIZE
				if (iteratorSizesIdx == 0 && iteratorSizes[0] >= MAX_FIRST_SIZE)
					return false;
#endif
				iteratorSizes[iteratorSizesIdx]++;
				assert(iteratorCount - 1 >= 0 && iteratorCount - 1 < max_program_size);
				remainingSize--;
#ifdef NO_TRAILING_LINEAR_PROGRAM
				if (remainingSize != 0) goto restart;
#endif
				iteratorSizes[iteratorCount - 1] = remainingSize;
				return true;
			}
		}
		return false;
	}

private:
	uint8_t data[data_size + 2 * cache_data_size];
	uint_fast32_t dataIdx;
	uint_fast32_t dataBoundLow;
	uint_fast32_t dataBoundHigh;
	
	ModDivisionTable* divisionTable;

	bool lastExecutionSuccessful;
	uint_fast32_t lastExecutionMaxProgramIdx;

public:
	bool Execute(const char* initialData, const uint_fast32_t initialDataSize, const int_fast32_t initialDataOffset)
	{
		// TODO: short-circuit unbalanced linear loop if beyond bounds and ends on nonzero

		lastExecutionSuccessful = false;
		lastExecutionMaxProgramIdx = 0;

		uint_fast32_t programIdx = 0;
		dataIdx = data_size / 2 + cache_data_size;
		//dataBoundLow = dataIdx;
		//dataBoundHigh = dataIdx + 1;
		
		memset(data, 0, data_size + 2 * cache_data_size);
		memcpy(data + dataIdx + initialDataOffset, initialData, initialDataSize);

		uint_fast32_t remainingJumps = MAX_JUMPS;
		while (remainingJumps--)
		{
			if (programIdx >= iteratorCount)
			{
				lastExecutionSuccessful = true;
				return true;
			}

			auto iteratorData = iterators[programIdx].Data();
			// is of the form '[*]' where * is only <>+-
			bool isLinear = jumps[programIdx].nonzero == programIdx;

			if (isLinear && iterators[programIdx].IsBalanced())
			{
				uint8_t centerCell = iteratorData.data[iteratorData.idx + cache_data_size / 2];
				int cnt = divisionTable->Get(data[dataIdx], centerCell);
				if (cnt == -1)
				{
					return false;
				}

#ifdef SHORT_CIRCUIT_LINEAR_SINGULAR
				auto programSize = iteratorSizes[programIdx];
				if (programSize == centerCell || programSize == 256 - centerCell)
				{
					data[dataIdx] = 0; 
				}
				else
				{
					ApplyDataMult(iteratorData, cnt);
				}
#else
				ApplyDataMult(iteratorData.data, cnt);
#endif
				programIdx = jumps[programIdx].zero;
				continue;
			}

			uint_fast32_t newDataIdx = dataIdx + iteratorData.idx;
			if (newDataIdx < cache_data_size || newDataIdx >= data_size + cache_data_size)
			{
				return false;
			}
			
			ApplyData(iteratorData);
			dataIdx = newDataIdx;

			// TODO: test zero, nonzero in array
			programIdx = (data[dataIdx] == 0) ? jumps[programIdx].zero : jumps[programIdx].nonzero;
			if (programIdx > lastExecutionMaxProgramIdx)
			{
				lastExecutionMaxProgramIdx = programIdx;
			}
		}
	
		return false;
	}

	inline uint8_t* Data()
	{
		return data;
	}
	
	inline uint_fast32_t DataIdx()
	{
		return dataIdx;
	}

	inline bool DataEqual(const uint8_t* otherData, uint_fast32_t count, int_fast32_t offset)
	{
		for (uint_fast32_t i = 0; i < count; i++)
		{
			if (data[i + dataIdx + offset] != otherData[i]) return false;
		}
		return true;
	}

	inline bool DataEqual(const char* otherData, uint_fast32_t count, int_fast32_t offset)
	{
		for (uint_fast32_t i = 0; i < count; i++)
		{
			if (data[i + dataIdx + offset] != static_cast<uint8_t>(otherData[i])) return false;
		}
		return true;
	}

	inline bool DataEqualZeroOrNonzero(const uint8_t* otherData, uint_fast32_t count, int_fast32_t offset)
	{
		for (uint_fast32_t i = 0; i < count; i++)
		{
			if ((data[i + dataIdx + offset] == 0) != (otherData[i] == 0))
			{
				return false;
			}
		}
		return true;
	}

	inline bool DataEqualZeroOrNonzero(const char* otherData, uint_fast32_t count, int_fast32_t offset)
	{
		for (uint_fast32_t i = 0; i < count; i++)
		{
			if ((data[i + dataIdx + offset] == 0) != (otherData[i] == 0))
			{
				return false;
			}
		}
		return true;
	}

	inline bool DataEqualMultiple(const uint8_t* otherData, uint_fast32_t count, int_fast32_t offset, uint8_t multiple)
	{
		for (uint_fast32_t i = 0; i < count; i++)
		{
			if (data[i + dataIdx + offset] != static_cast<uint8_t>(otherData[i] * multiple))
			{
				return false;
			}
		}
		return true;
	}

	inline bool DataEqualMultiple(const char* otherData, uint_fast32_t count, int_fast32_t offset, uint8_t multiple)
	{
		for (uint_fast32_t i = 0; i < count; i++)
		{
			if (data[i + dataIdx + offset] != static_cast<uint8_t>(otherData[i] * multiple))
			{
				return false;
			}
		}
		return true;
	}

private:
	inline void ApplyData(AlignedData<cache_data_size>& argData)
	{
		//uint_fast32_t sseLow = argData.start / 16;
		//uint_fast32_t sseHigh = argData.end / 16;
		//
		//__m128i sseArgData = reinterpret_cast<__m128i*> argData.data;
		//__m123i sseData = reinterpret_cast<__m128i*> data + dataIdx;

		for (int_fast32_t i = argData.start; i < argData.end; i++)
		{
			data[dataIdx + i] += argData.data[i + cache_data_size / 2];
		}
	}

	inline void ApplyDataMult(AlignedData<cache_data_size>& argData, uint8_t m)
	{
		for (int_fast32_t i = argData.start; i < argData.end; i++)
		{
			data[dataIdx + i] += argData.data[i + cache_data_size / 2] * m;
		}
	}

public:
	char currentProgram[256];

	char* GetProgram()
	{
		char* output = currentProgram;
		for (uint32_t i = 0; i < iteratorCount; i++)
		{
			char* iterProg = iterators[i].GetProgram();
			strcpy(output, iterProg); 
			output += strlen(output);
			if (i < iteratorCount - 1) output[0] = brackets[i].bracket == Bracket::LEFT ? '[' : brackets[i].bracket == Bracket::RIGHT ? ']' : '.';
			output++;
			assert(output <= currentProgram + sizeof(currentProgram));
		}
		output[0] = '\0';
		return currentProgram;
	}

	char FirstChar()
	{
		return iterators[0].FirstChar();
	}

	void PrintData()
	{
		for (uint_fast32_t i = dataBoundLow; i <= dataBoundHigh; i++)
			std::cout << std::setw(4) << (int)data[i];
		std::cout << std::endl << std::string((dataIdx - dataBoundLow) * 4, ' ') << "  ^" << std::endl;
	}
};
