#pragma once

#include <assert.h>
#include "LinearIterator.h"
#include "ModDivisionTable.h"

// #define SINGLE_BRACKET_HIERARCHY true
//#define MAX_BRACKET_DEPTH 2
#define MAX_JUMPS 5000
#define SHORT_CIRCUIT_LINEAR_SINGULAR
#define INITIAL_ZERO
// #define SINGLE_ITER_COUNT 5
#define NO_ENDING
#define MAX_FIRST_SIZE 2

template<uint_fast32_t data_size, uint_fast32_t cache_data_size, uint_fast32_t cache_size, uint_fast32_t max_program_size = 32>
class ProgramIterator
{
public:
	ProgramIterator()
	{
	}
	~ProgramIterator()
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

public:
	void Start(uint_fast32_t programSize, uint_fast32_t threadOffset, uint_fast32_t threadDelta)
	{
		this->programSize = programSize;
		this->threadOffset = threadOffset;
		this->threadDelta = threadDelta;

		memset(iteratorSizes, 0, max_program_size * sizeof(int_fast32_t));

		iteratorCount = 1;
		iteratorSizes[0] = programSize;
		bool found = NextValidIteratorSizes(threadOffset);
		assert(found);

		bracketIdx = 1;
		brackets[0].bracket = Bracket::LEFT;
		brackets[0].depth = 1;
		leftBracketStack[1] = 0;
		jumps[0].zero = jumps[0].nonzero = 1;
		
		while (!NextBrackets())
		{
			bool found = NextValidIteratorSizes(threadDelta);
			assert(found);
			bracketIdx = 1;
			leftBracketStack[1] = 0;
		}

		iteratorIdx = 0;
		iterators[0].Start(iteratorSizes[0]);
	}

	bool Next()
	{
		while (!NextIterators())
		{
			while (!NextBrackets())
			{
				if (!NextValidIteratorSizes(threadDelta)) return false;
				bracketIdx = 1;
				leftBracketStack[1] = 0;
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
#ifdef SINGLE_ITER_COUNT
				if (iteratorCount == 1) iteratorCount = SINGLE_ITER_COUNT;
				else return false;
#else
				iteratorCount += 2;
#endif
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
				iteratorSizes[iteratorCount - 2] = -1;
				remainingSize++;
			}
		}
		return true;
	}

	bool NextIterators()
	{
		while (iteratorIdx >= 0)
		{
			assert(iteratorIdx < max_program_size);
			if (iterators[iteratorIdx].Next())
			{
				if (iteratorIdx == iteratorCount - 1) return true;
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

	bool NextBrackets()
	{
		if (iteratorCount == 1) return false;

		while (bracketIdx > 0)
		{
			assert(bracketIdx >= 1 && bracketIdx < max_program_size);
			if (brackets[bracketIdx].bracket == Bracket::EMPTY)
			{
				uint_fast32_t remainingBrackets = iteratorCount - bracketIdx - 1;
				assert(remainingBrackets >= brackets[bracketIdx - 1].depth);
				if (remainingBrackets == brackets[bracketIdx - 1].depth)
				{
					if (!SetBracketRight()) continue;
					if (bracketIdx == iteratorCount - 2) return true;
				}
				else
				{
					if (!SetBracketLeft()) continue;
				}
				bracketIdx++;
			}
			else if (brackets[bracketIdx].bracket == Bracket::LEFT && brackets[bracketIdx - 1].depth > 0)
			{
				if (!SetBracketRight()) continue;
				bracketIdx++;
			}
			else
			{
				brackets[bracketIdx].bracket = Bracket::EMPTY;
				bracketIdx--;
			}
		}
		return false;
	}

	inline bool SetBracketLeft()
	{
		assert(bracketIdx >= 0 && bracketIdx < max_program_size);
		brackets[bracketIdx].bracket = Bracket::LEFT;
		brackets[bracketIdx].depth = brackets[bracketIdx - 1].depth + 1;
		assert(brackets[bracketIdx].depth >= 0 && brackets[bracketIdx].depth < max_program_size);
		leftBracketStack[brackets[bracketIdx].depth] = bracketIdx;

		return (iteratorSizes[bracketIdx] > 0 || brackets[bracketIdx - 1].bracket == Bracket::LEFT)
#ifdef MAX_BRACKET_DEPTH
			   && brackets[bracketIdx].depth <= MAX_BRACKET_DEPTH
#endif
#ifdef SINGLE_BRACKET_HIERARCHY
			   && brackets[bracketIdx - 1].depth > 0
#endif 
			   ;
	}

	inline bool SetBracketRight()
	{
		assert(bracketIdx >= 0 && bracketIdx < max_program_size);
		uint_fast32_t lbracket = leftBracketStack[brackets[bracketIdx - 1].depth];
		assert(lbracket >= 0 && lbracket < max_program_size);
		jumps[bracketIdx].zero = jumps[lbracket].zero = bracketIdx + 1;
		jumps[bracketIdx].nonzero = jumps[lbracket].nonzero = lbracket + 1;

		brackets[bracketIdx].bracket = Bracket::RIGHT;
		brackets[bracketIdx].depth = brackets[bracketIdx - 1].depth - 1;

		return iteratorSizes[bracketIdx] > 0;
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
#ifdef NO_ENDING
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

public:
	bool Execute(const char* initialData, const uint_fast32_t initialDataSize)
	{
		// TODO: short-circuit unbalanced linear loop if beyond bounds and ends on nonzero

		uint_fast32_t programIdx = 0;
		dataIdx = data_size / 2 + cache_data_size;
		//dataBoundLow = dataIdx;
		//dataBoundHigh = dataIdx + 1;
		
		memset(data, 0, data_size + 2 * cache_data_size);
		memcpy(data + dataIdx, initialData, initialDataSize);

		uint_fast32_t remainingJumps = MAX_JUMPS;
		while (remainingJumps--)
		{
			auto iteratorData = iterators[programIdx].Data();
			bool isLinear = jumps[programIdx].nonzero == programIdx;

			// Technically may be incorrect if iterator size is greater than 256
			//if (isLinear && dataIdx + iteratorData.idx < dataBoundLow && 
			//	iteratorData.data[iteratorData.idx] != 0) return false;
			//if (isLinear && dataIdx + iteratorData.idx >= dataBoundHigh &&
			//	iteratorData.data[iteratorData.idx] != 0) return false;
			//dataBoundLow = Min(dataBoundLow, dataIdx + iteratorData.start);
			//dataBoundHigh = Max(dataBoundHigh, dataIdx + iteratorData.end);

			if (isLinear && iterators[programIdx].IsBalanced())
			{
				uint8_t centerCell = iteratorData.data[iteratorData.idx + cache_data_size / 2];
				int cnt = divisionTable->Get(data[dataIdx], centerCell);
				if (cnt == -1) return false;

#ifdef SHORT_CIRCUIT_LINEAR_SINGULAR
				auto programSize = iteratorSizes[programIdx];
				if (programSize == centerCell || programSize == 256 - centerCell)
				{
					data[dataIdx] = 0; 
				}
				else ApplyDataMult(iteratorData, cnt);
#else
				ApplyDataMult(iteratorData.data, cnt);
#endif
				programIdx = jumps[programIdx].zero;
				continue;
			}

			uint_fast32_t newDataIdx = dataIdx + iteratorData.idx;
			if (newDataIdx < cache_data_size || newDataIdx >= data_size + cache_data_size) return false;
			
			ApplyData(iteratorData);
		
			dataIdx += iteratorData.idx;
			if (programIdx == iteratorCount - 1) return true;
			// TODO: test zero, nonzero in array
			programIdx = (data[dataIdx] == 0) ? jumps[programIdx].zero : jumps[programIdx].nonzero;
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

	inline bool DataEqual(const uint8_t* otherData, uint_fast32_t count)
	{
		for (uint_fast32_t i = 0; i < count; i++)
		{
			if (data[i + dataIdx] != otherData[i]) return false;
		}
		return true;
	}

	inline bool DataEqual(const char* otherData, uint_fast32_t count)
	{
		for (uint_fast32_t i = 0; i < count; i++)
		{
			if (data[i + dataIdx] != static_cast<uint8_t>(otherData[i])) return false;
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
			if (c > 0)
			{
				for (uint_fast32_t i = 0; i < c; i++) *(output++) = '+';
			}
			else
			{
				for (uint_fast32_t i = 0; i < -c; i++) *(output++) = '-';
			}

			score += bestCharScore;
			data[bestIdx] = c;
			curIdx = bestIdx;
		}

		*output = '\0';
		return score;
	}

	char currentProgram[256];

	char* GetProgram()
	{
		char* output = currentProgram;
		for (uint32_t i = 0; i < iteratorCount; i++)
		{
			char* iterProg = iterators[i].GetProgram();
			strcpy(output, iterProg); 
			output += strlen(output);
			if (i < iteratorCount - 1) output[0] = brackets[i].bracket == Bracket::LEFT ? '[' : ']';
			output++;
			assert(output <= currentProgram + sizeof(currentProgram));
		}
		output[0] = '\0';
		return currentProgram;
	}

	inline bool StartsPlus()
	{
		return static_cast<int8_t>(iterators[0].Data().data[cache_data_size / 2]) > 0;
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
