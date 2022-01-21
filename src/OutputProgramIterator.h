#pragma once

#include <assert.h>
#include <locale>
#include "LinearIterator.h"
#include "ModDivisionTable.h"

// #define SINGLE_BRACKET_HIERARCHY
// #define MAX_BRACKET_DEPTH 1
#define MAX_JUMPS 25000
#define SHORT_CIRCUIT_LINEAR_SINGULAR
// #define INITIAL_DATA_SYMMETRIC
#define NO_TRAILING_LINEAR_PROGRAM
#define AFTER_OUTPUT_IRRELEVANT
// #define MAX_FIRST_SIZE 2
#define MINIMUM_OUTPUT_COUNT 1
// #define MAXIMUM_OUTPUT_COUNT 1
#define NO_REPEATED_OUTPUT
// #define CASE_INSENSITIVE
#define MAXIMUM_ZERO_DEPTH_OUTPUT_COUNT 1
#define MAXIMUM_NONZERO_DEPTH_OUTPUT_COUNT 1
#define MINIMUM_NONZERO_DEPTH_OUTPUT_EXECUTION_COUNT 2

template<uint_fast32_t data_size, uint_fast32_t cache_data_size, uint_fast32_t cache_size, uint_fast32_t max_program_size = 32>
class OutputProgramIterator
{
public:
	OutputProgramIterator()
	{
	}
	~OutputProgramIterator()
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

	uint_fast32_t programSize;

private:
	DataCache<cache_data_size, cache_size>* cache;

	uint_fast32_t threadOffset, threadDelta;
	bool initialZero;

	uint_fast64_t currentCount;

	LinearIterator<cache_data_size, cache_size, max_program_size> iterators[max_program_size];
	uint_fast32_t iteratorCount;
	
	int_fast32_t iteratorSizes[max_program_size];
	int_fast32_t remainingSize;
	
	enum class Bracket
	{
		EMPTY,
		LEFT,
		RIGHT,
		OUTPUT
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

	void Start(uint_fast32_t programSize, uint_fast32_t threadOffset, uint_fast32_t threadDelta, bool initialZero = false)
	{
		this->programSize = programSize;
		this->currentCount = 0;
		this->threadOffset = threadOffset;
		this->threadDelta = threadDelta;
		this->initialZero = initialZero;

		memset(iteratorSizes, 0, max_program_size * sizeof(int_fast32_t));

		iteratorCount = 1;
		iteratorSizes[0] = programSize;
		firstIteratorWithNonZeroDataDelta = 0;
		lastExecutionSuccessful = true;
		bracketIdx = 0;

		NextValidIteratorSizes(threadOffset);
		while (!NextBrackets())
		{
			if (!NextValidIteratorSizes(threadDelta))
			{
				return;
			}
			bracketIdx = 0;
		}
		lastExecutionMaxProgramIdx = iteratorCount - 1;

		// std::cout << threadOffset << ": ";
		// for (uint_fast32_t i = 0; i < iteratorCount; i++)
		// {
		// 	std::cout << iteratorSizes[i] << " ";
		// }
		// std::cout << std::endl;
		// std::cin.ignore();

		bracketIdx = 0;
		brackets[0].bracket = Bracket::EMPTY;
		brackets[0].depth = 0;
		jumps[0].zero = jumps[0].nonzero = 1;

#ifdef MINIMUM_OUTPUT_COUNT

		while (iteratorCount - 1 < MINIMUM_OUTPUT_COUNT)
		{
			if (!NextValidIteratorSizes(threadDelta))
			{
				return;
			}
		}
		bracketIdx = 0;
		while (!NextBrackets())
		{
			if (!NextValidIteratorSizes(threadDelta))
			{
				return;
			}
			bracketIdx = 0;
		}
		lastExecutionMaxProgramIdx = iteratorCount - 1;

#endif

		iteratorIdx = 0;
		iterators[0].Start(iteratorSizes[0]);
	}

	bool Next()
	{
		// if (lastExecutionMaxProgramIdx >= iteratorCount)
		// {
		// 	std::cout << std::endl << "Next(); " << lastExecutionMaxProgramIdx << " " << iteratorCount << std::endl;
		// 	std::cin.ignore();
		// }
		
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
				
				// for (uint_fast32_t i = 0; i < iteratorCount; i++)
				// {
				// 	std::cout << iteratorSizes[i] << " ";
				// }
				// std::cout << std::flush;
				// std::cin.ignore();

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
				iteratorCount += 1;

				remainingSize = programSize - iteratorCount + 1;
				if (remainingSize < 0) return false;

				for (uint_fast32_t i = 0; i < iteratorCount - 2; i++)
				{
					assert(i >= 0 && i < max_program_size);
					iteratorSizes[i] = 0;
				}

				if (this->initialZero)
				{
					iteratorSizes[0] = 1;
					remainingSize--;
					if (remainingSize < 0) return false;
				}

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
			if (this->initialZero)
			{
				if (iteratorIdx <= firstIteratorWithNonZeroDataDelta)
				{
					// Check that the first iterator has a right delta
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
				// Do not permit guaranteed 0 after initial iterator or meaningless movements at the beginning
				auto& data = iterators[0].Data();
				if (iteratorIdx == 0 && (data.data[data.idx + cache_data_size / 2] == 0 || data.data[cache_data_size / 2] == 0))
				{
					continue;
				}
			}

#if defined INITIAL_DATA_SYMMETRIC
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

			// Remove patterns equivalent to [+]
			// Note that while [+*] and [-*] are not necessarily equivalent in behavior to [+], the cases in which they are not equivalent are always infinite loops and so can be safely pruned
			if (
				iteratorIdx > 0
				&& brackets[iteratorIdx - 1].bracket == Bracket::LEFT
				&& brackets[iteratorIdx].bracket == Bracket::RIGHT
				&&
				(
					// Do not permit [-*] with any amount of minuses
					iterators[iteratorIdx].Data().data[cache_data_size / 2] == 256 - iteratorSizes[iteratorIdx]
					// Do not permit [+*] with more than 1 plus
					|| iteratorSizes[iteratorIdx] > 1 && iterators[iteratorIdx].Data().data[cache_data_size / 2] == iteratorSizes[iteratorIdx]
				)
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
	restart:;
		while (bracketIdx >= 0)
		{
			uint_fast32_t remainingBrackets = iteratorCount - bracketIdx - 1;
			bool left_valid = bracketIdx == 0 ? remainingBrackets >= 2 : remainingBrackets >= brackets[bracketIdx - 1].depth + 2;
			bool right_valid = bracketIdx == 0 ? false : brackets[bracketIdx - 1].depth > 0;
			bool output_valid = bracketIdx == 0 ? remainingBrackets >= 1 : remainingBrackets >= brackets[bracketIdx - 1].depth + 1;

			// std::cout << "iteratorCount " << iteratorCount << std::endl;
			// std::cout << "bracketIdx " << bracketIdx << std::endl;
			// std::cout << "remainingBrackets " << remainingBrackets << std::endl;
			// std::cout << "brackets valid " << left_valid << " " << right_valid << " " << output_valid << std::endl;
			// std::cin.ignore();

			// EMPTY -> LEFT -> RIGHT -> OUTPUT -> EMPTY
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
				else if (output_valid)
				{
					if (!SetBracketOutput()) continue;
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
				else if (output_valid)
				{
					if (!SetBracketOutput()) continue;
					if (bracketIdx == iteratorCount - 2) goto success;
					bracketIdx++;
				}
				else
				{
					brackets[bracketIdx].bracket = Bracket::EMPTY;
					bracketIdx--;
				}
			}
			else if (brackets[bracketIdx].bracket == Bracket::RIGHT)
			{
				if (output_valid)
				{
					if (!SetBracketOutput()) continue;
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

#if defined MINIMUM_OUTPUT_COUNT || defined MAXIMUM_OUTPUT_COUNT || defined MAXIMUM_ZERO_DEPTH_OUTPUT_COUNT || defined MAXIMUM_NONZERO_DEPTH_OUTPUT_COUNT
		uint_fast32_t zero_depth_output_count = 0;
		uint_fast32_t output_count = 0;
		for (int_fast32_t i = 0; i < iteratorCount - 1; i++)
		{
			if (brackets[i].bracket == Bracket::OUTPUT)
			{
				output_count++;
				if (brackets[i].depth == 0)
				{
					zero_depth_output_count++;
				}
			}
		}

#if defined MINIMUM_OUTPUT_COUNT
		if (output_count < MINIMUM_OUTPUT_COUNT) goto restart;
#endif
#if defined MAXIMUM_OUTPUT_COUNT
		if (output_count > MAXIMUM_OUTPUT_COUNT) goto restart;
#endif
#if defined MAXIMUM_ZERO_DEPTH_OUTPUT_COUNT
		if (zero_depth_output_count > MAXIMUM_ZERO_DEPTH_OUTPUT_COUNT) goto restart;
#endif
#if defined MAXIMUM_NONZERO_DEPTH_OUTPUT_COUNT
		if (output_count - zero_depth_output_count > MAXIMUM_NONZERO_DEPTH_OUTPUT_COUNT) goto restart;
#endif

#endif

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
			|| (bracketIdx > 0 && brackets[bracketIdx - 1].bracket != Bracket::RIGHT)
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

	inline bool SetBracketOutput()
	{
		brackets[bracketIdx].bracket = Bracket::OUTPUT;
		brackets[bracketIdx].depth = bracketIdx == 0 ? 0 : brackets[bracketIdx - 1].depth;

		// allow .. [.
		// disallow ].
		// if NO_REPEATED_OUTPUT disallow ..
		return (
			bracketIdx == 0
			|| iteratorSizes[bracketIdx] > 0
			||
			(
				bracketIdx > 0
				&& brackets[bracketIdx - 1].bracket != Bracket::RIGHT
#ifdef NO_REPEATED_OUTPUT
				&& brackets[bracketIdx - 1].bracket != Bracket::OUTPUT
#endif
			)

		);
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
			else if (brackets[i].bracket == Bracket::OUTPUT)
			{
				jumps[i].zero = jumps[i].nonzero = i + 1;
			}
			// else
			// {
			// 	std::cout << "Empty bracket in SetJumps" << std::endl;
			// 	std::cout << GetProgram() << std::endl;
			// 	for (uint_fast32_t i = 0; i < iteratorCount - 1; i++)
			// 	{
			// 		std::cout
			// 			<< "    " << brackets[i].depth << " "
			// 			<< (
			// 				brackets[i].bracket == Bracket::LEFT ? "[" :
			// 				brackets[i].bracket == Bracket::RIGHT ? "]" :
			// 				brackets[i].bracket == Bracket::OUTPUT ? "." :
			// 				brackets[i].bracket == Bracket::EMPTY ? "_" :
			// 				"!"
			// 			)
			// 			<< std::endl;
			// 	}
			// 	std::cin.ignore();
			// }
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
				remainingSize--;
#ifdef NO_TRAILING_LINEAR_PROGRAM
				if (remainingSize != 0)
					goto restart;
#endif
				assert(iteratorCount - 1 >= 0 && iteratorCount - 1 < max_program_size);
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
	bool Execute(const char* initialData, const uint_fast32_t initialDataSize, const char* targetOutput, const uint_fast32_t targetOutputSize, uint_fast32_t* targetOutputIdx)
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
			if (programIdx == iteratorCount - 1)
			{
				lastExecutionSuccessful = true;
				return true;
			}

			if (brackets[programIdx].bracket == Bracket::OUTPUT)
			{
				if (
					*targetOutputIdx == targetOutputSize ||
#ifdef CASE_INSENSITIVE
					tolower(data[dataIdx]) != tolower(targetOutput[*targetOutputIdx])
#else
					data[dataIdx] != targetOutput[*targetOutputIdx]
#endif
				)
				{
					return false;
				}
				(*targetOutputIdx)++;
#ifdef AFTER_OUTPUT_IRRELEVANT
				if (*targetOutputIdx == targetOutputSize)
				{
					lastExecutionSuccessful = true;
					return true;
				}
#endif
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

	bool ExecuteRawData(const uint8_t* initialData, const int_fast32_t initialDataIdx, const char* targetOutput, const uint_fast32_t targetOutputSize, uint_fast32_t* targetOutputIdx)
	{
		// TODO: short-circuit unbalanced linear loop if beyond bounds and ends on nonzero
		
		// for (uint_fast32_t i = 0; i < iteratorCount - 1; i++)
		// {
		// 	if (brackets[i].bracket == Bracket::EMPTY)
		// 	{
		// 		std::cout << "Empty bracket in execution" << std::endl;
		// 		std::cout << GetProgram() << std::endl;
		// 		for (uint_fast32_t i = 0; i < iteratorCount - 1; i++)
		// 		{
		// 			std::cout
		// 				<< "    " << brackets[i].depth << " "
		// 				<< (
		// 					brackets[i].bracket == Bracket::LEFT ? "[" :
		// 					brackets[i].bracket == Bracket::RIGHT ? "]" :
		// 					brackets[i].bracket == Bracket::OUTPUT ? "." :
		// 					brackets[i].bracket == Bracket::EMPTY ? "_" :
		// 					"!"
		// 				)
		// 				<< std::endl;
		// 		}
		// 		std::cin.ignore();
		// 		break;
		// 	}
		// }

		lastExecutionSuccessful = false;
		lastExecutionMaxProgramIdx = 0;

		uint_fast32_t programIdx = 0;
		dataIdx = initialDataIdx;
		memcpy(data, initialData, data_size + 2 * cache_data_size);

#ifdef MINIMUM_NONZERO_DEPTH_OUTPUT_EXECUTION_COUNT
		uint_fast32_t output_execution_counts[max_program_size];
		memset(output_execution_counts, 0, max_program_size * sizeof(uint_fast32_t));
#endif

		uint_fast32_t remainingJumps = MAX_JUMPS;
		while (remainingJumps--)
		{
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
			if (programIdx == iteratorCount - 1)
			{
				lastExecutionSuccessful = true;
#ifdef MINIMUM_NONZERO_DEPTH_OUTPUT_EXECUTION_COUNT
				for (uint_fast32_t idx = 0; idx < iteratorCount; idx++)
				{
					if (
						brackets[idx].bracket == Bracket::OUTPUT
						&& brackets[idx].depth > 0
						&& output_execution_counts[idx] <= MINIMUM_NONZERO_DEPTH_OUTPUT_EXECUTION_COUNT
					)
					{
						return false;
					}
				}
#endif
				return true;
			}

			if (brackets[programIdx].bracket == Bracket::OUTPUT)
			{
#ifdef MINIMUM_NONZERO_DEPTH_OUTPUT_EXECUTION_COUNT
				output_execution_counts[programIdx]++;
#endif

				if (
					*targetOutputIdx == targetOutputSize ||
#ifdef CASE_INSENSITIVE
					tolower(data[dataIdx]) != tolower(targetOutput[*targetOutputIdx])
#else
					data[dataIdx] != targetOutput[*targetOutputIdx]
#endif
				)
				{
					return false;
				}
				(*targetOutputIdx)++;
#ifdef AFTER_OUTPUT_IRRELEVANT
				if (*targetOutputIdx == targetOutputSize)
				{
					lastExecutionSuccessful = true;
					return true;
				}
#endif
			}
			// TODO: test zero, nonzero in array
			programIdx = (data[dataIdx] == 0) ? jumps[programIdx].zero : jumps[programIdx].nonzero;
			if (programIdx > lastExecutionMaxProgramIdx)
			{
				lastExecutionMaxProgramIdx = programIdx;

				// if (lastExecutionMaxProgramIdx >= iteratorCount)
				// {
				// 	std::cout << std::endl << "lastExecutionMaxProgramIdx = programIdx; " << lastExecutionMaxProgramIdx << " " << iteratorCount << std::endl;
				// 	std::cout << GetProgram() << std::endl;
				// 	for (uint_fast32_t i = 0; i < iteratorCount - 1; i++)
				// 	{
				// 		std::cout << i << " -> " << jumps[i].zero << " " << jumps[i].nonzero << std::endl;
				// 		std::cout
				// 			<< "    " << brackets[i].depth << " "
				// 			<< (
				// 				brackets[i].bracket == Bracket::LEFT ? "[" :
				// 				brackets[i].bracket == Bracket::RIGHT ? "]" :
				// 				brackets[i].bracket == Bracket::OUTPUT ? "." :
				// 				brackets[i].bracket == Bracket::EMPTY ? "_" :
				// 				"!"
				// 			)
				// 			<< std::endl;
				// 	}
				// 	std::cin.ignore();
				// }
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
	char currentProgram[256];

	char* GetProgram()
	{
		char* output = currentProgram;
		for (uint32_t i = 0; i < iteratorCount; i++)
		{
			char* iterProg = iterators[i].GetProgram();
			strcpy(output, iterProg); 
			output += strlen(output);
			if (i < iteratorCount - 1) output[0] = brackets[i].bracket == Bracket::LEFT ? '[' : brackets[i].bracket == Bracket::RIGHT ? ']' : brackets[i].bracket == Bracket::OUTPUT ? '.' : '_';
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
