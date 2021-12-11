#pragma once

#include <assert.h>
#include <locale>
#include "LinearIterator.h"
#include "ModDivisionTable.h"

#define SINGLE_BRACKET_HIERARCHY
//#define MAX_BRACKET_DEPTH 2
#define MAX_JUMPS 20000
#define SHORT_CIRCUIT_LINEAR_SINGULAR
#define INITIAL_ZERO
// #define INITIAL_DATA_SYMMETRIC
// #define SINGLE_ITER_COUNT 5
#define NO_ENDING
#define MAX_FIRST_SIZE 1
// #define CASE_INSENSITIVE

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
		lastExecutionMaxProgramIdx = 0;
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
			lastExecutionMaxProgramIdx = iteratorCount - 1;
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
		if (iteratorCount == 1) return false;

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

		return (
			iteratorSizes[bracketIdx] > 0
			|| (bracketIdx > 0 && brackets[bracketIdx - 1].bracket == Bracket::LEFT)
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
		assert(bracketIdx > 0 && bracketIdx < max_program_size);
		brackets[bracketIdx].bracket = Bracket::RIGHT;
		brackets[bracketIdx].depth = brackets[bracketIdx - 1].depth - 1;

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
			else
			{
				uint_fast32_t lbracket = leftBracketStack[brackets[i - 1].depth];
				jumps[i].zero = jumps[lbracket].zero = i + 1;
				jumps[i].nonzero = jumps[lbracket].nonzero = lbracket + 1;
			}
		}
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

	bool lastExecutionSuccessful;
	uint_fast32_t lastExecutionMaxProgramIdx;

public:
	bool Execute(const char* initialData, const uint_fast32_t initialDataSize)
	{
		// TODO: short-circuit unbalanced linear loop if beyond bounds and ends on nonzero

		lastExecutionSuccessful = false;
		lastExecutionMaxProgramIdx = 0;

		uint_fast32_t programIdx = 0;
		dataIdx = data_size / 2 + cache_data_size;
		//dataBoundLow = dataIdx;
		//dataBoundHigh = dataIdx + 1;
		
		memset(data, 0, data_size + 2 * cache_data_size);
		memcpy(data + dataIdx, initialData, initialDataSize);

		uint_fast32_t remainingJumps = MAX_JUMPS;
		while (remainingJumps--)
		{
			assert(programIdx < iteratorCount);
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
				else ApplyDataMult(iteratorData, cnt);
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
			if (programIdx == iteratorCount - 1)
			{
				lastExecutionSuccessful = true;
				return true;
			}

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
	uint_fast32_t StringDistance(const char* target, uint_fast32_t targetSize, uint_fast32_t shortCircuit)
	{
		uint8_t currentData[data_size];
		memcpy(currentData, data + cache_data_size, data_size);
		return StringDistanceRecursive(target, targetSize, shortCircuit, currentData, dataIdx - cache_data_size);
	}

	uint_fast32_t StringDistanceRecursive(const char* target, uint_fast32_t targetSize, uint_fast32_t shortCircuit, uint8_t* currentData, uint_fast32_t currentDataIdx)
	{
		if (targetSize == 0)
		{
			return 0;
		}

		uint_fast32_t bestScore = 100000;
		char c = target[0];

		// Compute best scores for reaching each specific data index
		uint_fast32_t dataDeltaScores[data_size];
		for (uint_fast32_t idx = 0; idx < data_size; idx++)
		{
			dataDeltaScores[idx] = abs(static_cast<int_fast32_t>(currentDataIdx - idx));
		}
		for (int_fast32_t startDelta = -8; startDelta <= 8; startDelta++)
		{
			int_fast32_t startIdx = static_cast<int_fast32_t>(currentDataIdx) + startDelta;
			if (startIdx < 0 || startIdx >= data_size)
			{
				continue;
			}
			for (int_fast32_t innerDelta = -8; innerDelta <= 8; innerDelta++)
			{
				if (innerDelta == 0)
				{
					continue;
				}
				int_fast32_t innerIdx = startIdx;
				while (currentData[innerIdx])
				{
					innerIdx += innerDelta;
					if (innerIdx < 0 || innerIdx >= data_size)
					{
						goto loop_fail;
					}
				}
				for (int_fast32_t endDelta = -8; endDelta <= 8; endDelta++)
				{
					int_fast32_t endIdx = innerIdx + endDelta;
					if (endIdx < 0 || endIdx >= data_size)
					{
						continue;
					}
					uint_fast32_t dataDeltaScore = 2 + abs(startDelta) + abs(innerDelta) + abs(endDelta);
					if (dataDeltaScore < dataDeltaScores[endIdx])
					{
						dataDeltaScores[endIdx] = dataDeltaScore;
					}
				}
			loop_fail:;
			}
		}

		// Search through reaching each specific data index and directly modifying the data to match the next character
		for (uint_fast32_t newDataIdx = 0; newDataIdx < data_size; newDataIdx++)
		{
			uint_fast32_t charScore = 1 + dataDeltaScores[newDataIdx] + abs(static_cast<int8_t>(c - currentData[newDataIdx]));

			if (charScore > shortCircuit)
			{
				continue;
			}

			char oldData = currentData[newDataIdx];
			currentData[newDataIdx] = c;
			uint_fast32_t restScore = StringDistanceRecursive(target + 1, targetSize - 1, shortCircuit - charScore, currentData, newDataIdx);
			currentData[newDataIdx] = oldData;

			if (charScore + restScore < bestScore)
			{
				bestScore = charScore + restScore;
			}

#ifdef CASE_INSENSITIVE
			if ('a' <= c && c <= 'z')
			{
				char upperC = toupper(c);
				uint_fast32_t upperCharScore = 1 + dataDeltaScores[newDataIdx] + abs(static_cast<int8_t>(upperC - currentData[newDataIdx]));

				if (upperCharScore > shortCircuit)
				{
					continue;
				}

				char oldData = currentData[newDataIdx];
				currentData[newDataIdx] = upperC;
				uint_fast32_t restScore = StringDistanceRecursive(target + 1, targetSize - 1, shortCircuit - upperCharScore, currentData, newDataIdx);
				currentData[newDataIdx] = oldData;

				if (upperCharScore + restScore < bestScore)
				{
					bestScore = upperCharScore + restScore;
				}
			}
#endif
		}

		// TODO: Search for any multi-output loops ><[><+-.><+-]
		return bestScore;
	}

	std::string StringDistanceOutput(const char* target, uint_fast32_t targetSize, uint_fast32_t shortCircuit)
	{
		uint8_t currentData[data_size];
		memcpy(currentData, data + cache_data_size, data_size);
		return StringDistanceOutputRecursive(target, targetSize, shortCircuit, currentData, dataIdx - cache_data_size);
	}

	std::string StringDistanceOutputRecursive(const char* target, uint_fast32_t targetSize, uint_fast32_t shortCircuit, uint8_t* currentData, uint_fast32_t currentDataIdx)
	{
		if (targetSize == 0)
		{
			return std::string();
		}

		uint_fast32_t bestScore = 100000;
		std::string bestProgram = std::string(shortCircuit + 1, ' ');
		char c = target[0];

		// Compute best scores for reaching each specific data index
		uint_fast32_t dataDeltaScores[data_size];
		std::string dataDeltaPrograms[data_size];

		for (uint_fast32_t idx = 0; idx < data_size; idx++)
		{
			dataDeltaScores[idx] = abs(static_cast<int_fast32_t>(currentDataIdx - idx));
			dataDeltaPrograms[idx] = "";
			if (idx > currentDataIdx)
			{
				for (uint_fast32_t i = 0; i < idx - currentDataIdx; i++) dataDeltaPrograms[idx] += '>';
			}
			else
			{
				for (uint_fast32_t i = 0; i < currentDataIdx - idx; i++) dataDeltaPrograms[idx] += '<';
			}
		}
		for (int_fast32_t startDelta = -8; startDelta <= 8; startDelta++)
		{
			int_fast32_t startIdx = static_cast<int_fast32_t>(currentDataIdx) + startDelta;
			if (startIdx < 0 || startIdx >= data_size)
			{
				continue;
			}
			for (int_fast32_t innerDelta = -8; innerDelta <= 8; innerDelta++)
			{
				if (innerDelta == 0)
				{
					continue;
				}
				int_fast32_t innerIdx = startIdx;
				while (currentData[innerIdx])
				{
					innerIdx += innerDelta;
					if (innerIdx < 0 || innerIdx >= data_size)
					{
						goto loop_fail;
					}
				}
				for (int_fast32_t endDelta = -8; endDelta <= 8; endDelta++)
				{
					int_fast32_t endIdx = innerIdx + endDelta;
					if (endIdx < 0 || endIdx >= data_size)
					{
						continue;
					}
					uint_fast32_t dataDeltaScore = 2 + abs(startDelta) + abs(innerDelta) + abs(endDelta);
					if (dataDeltaScore < dataDeltaScores[endIdx])
					{
						dataDeltaScores[endIdx] = dataDeltaScore;
						dataDeltaPrograms[endIdx] = std::string();
						if (startDelta > 0)
						{
							for (uint_fast32_t i = 0; i < startDelta; i++) dataDeltaPrograms[endIdx] += '>';
						}
						else
						{
							for (uint_fast32_t i = 0; i < -startDelta; i++) dataDeltaPrograms[endIdx] += '<';
						}
						dataDeltaPrograms[endIdx] += '[';
						if (innerDelta > 0)
						{
							for (uint_fast32_t i = 0; i < innerDelta; i++) dataDeltaPrograms[endIdx] += '>';
						}
						else
						{
							for (uint_fast32_t i = 0; i < -innerDelta; i++) dataDeltaPrograms[endIdx] += '<';
						}
						dataDeltaPrograms[endIdx] += ']';
						if (endDelta > 0)
						{
							for (uint_fast32_t i = 0; i < endDelta; i++) dataDeltaPrograms[endIdx] += '>';
						}
						else
						{
							for (uint_fast32_t i = 0; i < -endDelta; i++) dataDeltaPrograms[endIdx] += '<';
						}
					}
				}
			loop_fail:;
			}
		}

		// Search through reaching each specific data index and directly modifying the data to match the next character
		for (uint_fast32_t newDataIdx = 0; newDataIdx < data_size; newDataIdx++)
		{
			uint_fast32_t charScore = 1 + dataDeltaScores[newDataIdx] + abs(static_cast<int8_t>(c - currentData[newDataIdx]));
			std::string charProgram = dataDeltaPrograms[newDataIdx];
			int8_t diff = static_cast<int8_t>(c - currentData[newDataIdx]);
			if (diff > 0)
			{
				for (uint_fast32_t i = 0; i < diff; i++) charProgram += '+';
			}
			else
			{
				for (uint_fast32_t i = 0; i < -diff; i++) charProgram += '-';
			}
			charProgram += '.';

			if (charScore > shortCircuit)
			{
				continue;
			}

			char oldData = currentData[newDataIdx];
			currentData[newDataIdx] = c;
			std::string restProgram = StringDistanceOutputRecursive(target + 1, targetSize - 1, shortCircuit - charScore, currentData, newDataIdx);
			uint_fast32_t restScore = restProgram.size();
			currentData[newDataIdx] = oldData;

			if (charScore + restScore < bestScore)
			{
				bestScore = charScore + restScore;
				bestProgram = charProgram + restProgram;
			}

#ifdef CASE_INSENSITIVE
			if ('a' <= c && c <= 'z')
			{
				char upperC = toupper(c);
				uint_fast32_t upperCharScore = 1 + dataDeltaScores[newDataIdx] + abs(static_cast<int8_t>(upperC - currentData[newDataIdx]));
				std::string upperCharProgram = dataDeltaPrograms[newDataIdx];
				int8_t diff = static_cast<int8_t>(upperC - currentData[newDataIdx]);
				if (diff > 0)
				{
					for (uint_fast32_t i = 0; i < diff; i++) upperCharProgram += '+';
				}
				else
				{
					for (uint_fast32_t i = 0; i < -diff; i++) upperCharProgram += '-';
				}
				upperCharProgram += '.';

				if (upperCharScore > shortCircuit)
				{
					continue;
				}

				char oldData = currentData[newDataIdx];
				currentData[newDataIdx] = upperC;
				std::string restProgram = StringDistanceOutputRecursive(target + 1, targetSize - 1, shortCircuit - upperCharScore, currentData, newDataIdx);
				uint_fast32_t restScore = restProgram.size();
				currentData[newDataIdx] = oldData;

				if (upperCharScore + restScore < bestScore)
				{
					bestScore = upperCharScore + restScore;
					bestProgram = upperCharProgram + restProgram;
				}
			}
#endif
		}
		return bestProgram;
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
