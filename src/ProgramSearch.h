#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <assert.h>
#include "LinearIterator.h"
#include "ModDivisionTable.h"

#define THREAD_COUNT 8
#define SIZE_START 27
#define INITIAL_ZERO

template<typename PIteratorT, typename CacheT>
class ProgramSearch
{
public:
	ProgramSearch(
		std::vector<const char*> inputs,
		std::vector<const char*> outputs)
		: ProgramSearch(inputs, GetStringSizes(inputs), outputs, GetStringSizes(outputs))
	{
	}
	ProgramSearch(
		std::vector<const char*> inputs, 
		std::vector<uint_fast32_t> input_sizes, 
		std::vector<const char*> outputs)
		: ProgramSearch(inputs, input_sizes, outputs, GetStringSizes(outputs))
	{
	}
	ProgramSearch(
		std::vector<const char*> inputs, 
		std::vector<const char*> outputs, 
		std::vector<uint_fast32_t> output_sizes)
		: ProgramSearch(inputs, GetStringSizes(inputs), outputs, output_sizes)
	{
	}

	ProgramSearch(
		std::vector<const char*> inputs, 
		std::vector<uint_fast32_t> input_sizes, 
		std::vector<const char*> outputs, 
		std::vector<uint_fast32_t> output_sizes)
		: inputs(inputs), input_sizes(input_sizes), outputs(outputs), output_sizes(output_sizes), count(0), programResult(""), printProgress(true)
	{
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			threads[i] = nullptr;
		}
		cache.Create();
		Setup();
	}

	~ProgramSearch()
	{
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
			if (threads[i] != nullptr)
				delete threads[i];
	}

	static std::vector<uint_fast32_t> GetStringSizes(std::vector<const char*> strs)
	{
		std::vector<uint_fast32_t> result;
		for (uint_fast32_t i = 0; i < strs.size(); i++)
			result.push_back(strlen(strs[i]));
		return result;
	}

	void Setup()
	{
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			if (threads[i]) delete threads[i];
			threads[i] = nullptr;

			iterators[i] = PIteratorT();
			iterators[i].SetCache(&cache);
			iterators[i].SetDivisionTable(&divisionTable);
		}
	}

	bool printProgress;

private:
	CacheT cache;
	ModDivisionTable divisionTable;
	PIteratorT iterators[THREAD_COUNT];

	std::thread* threads[THREAD_COUNT];
	std::string programResult;
	bool foundProgramResult = false;

	uint_fast32_t programSize;

	std::vector<const char*> inputs;
	std::vector<uint_fast32_t> input_sizes;
	std::vector<const char*> outputs;
	std::vector<uint_fast32_t> output_sizes;

	uint_fast64_t count;
	static std::mutex lock;

public:
	std::string Find()
	{
		uint_fast32_t programSize = SIZE_START;
		while (!FindSize(programSize++))
		{
			Setup();
		}
		return programResult;
	}

	bool FindSize(uint_fast32_t programSize)
	{
		this->programSize = programSize;
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			if (threads[i]) delete threads[i];
			threads[i] = new std::thread(&ProgramSearch::FindThread, this, i);
		}
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			threads[i]->join();
		}
		return foundProgramResult;
	}

	inline std::string Result()
	{
		return programResult;
	}

private:
	void FindThread(uint_fast32_t threadIdx)
	{
		static const uint_fast32_t countUpdate = 1000000;

		auto& iterator = iterators[threadIdx];
		iterator.Start(programSize, threadIdx, THREAD_COUNT);

		uint_fast32_t threadCount = 0;
		while (iterator.Next())
		{
			if (++threadCount % countUpdate == 0)
			{
				lock.lock();
				if (foundProgramResult)
				{
					lock.unlock();
					break;
				}
				this->count += countUpdate;
				if (printProgress) std::cout << std::setw(10) << this->count << " " << iterator.GetProgram() << std::endl;
				lock.unlock();
			}

			for (uint_fast32_t i = 0; i < inputs.size(); i++)
			{
				if (!iterator.Execute(inputs[i], input_sizes[i]))
				{
					goto fail;
				}
				if (!iterator.DataEqual(outputs[i], output_sizes[i]))
				{
					goto fail;
				}
			}

			lock.lock();
			this->foundProgramResult = true;
			this->programResult = std::string(iterator.GetProgram());
			lock.unlock();
			break;

		fail:;
		}
	}

public:
	void FindString()
	{
		bestStringScore = 10000000;
		uint_fast32_t programSize = SIZE_START;
		programResult = std::string();
		while (true)
		{
			FindStringSize(programSize++);
			Setup();
		}
	}

	void FindStringSize(uint_fast32_t programSize)
	{
		this->programSize = programSize;
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			if (threads[i]) delete threads[i];
			threads[i] = new std::thread(&ProgramSearch::FindStringThread, this, i);
		}
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			threads[i]->join();
		}
	}

private:
	uint_fast32_t bestStringScore;

	void FindStringThread(uint_fast32_t threadIdx)
	{
		static const uint_fast32_t countUpdate = 1000000;

		auto& iterator = iterators[threadIdx];
		iterator.Start(programSize, threadIdx, THREAD_COUNT);

		lock.lock();
		uint_fast32_t threadBestStringScore = this->bestStringScore;
		lock.unlock();
		uint_fast32_t threadCount = 0;
		while (iterator.Next())
		{
			uint_fast32_t stringDist;
#ifdef INITIAL_ZERO
			if (!iterator.StartsPlus()) goto fail;
#endif
			if (++threadCount % countUpdate == 0)
			{
				lock.lock();
				this->count += countUpdate;
				if (printProgress) std::cout 
					<< std::right << std::setw(15) << this->count
					<< " " << programSize 
					<< " " << iterator.GetProgram() << "\r" << std::flush;
				threadBestStringScore = this->bestStringScore;
				lock.unlock();
			}
			
			if (strcmp(iterator.GetProgram(), "+[[>++++++>+<<]>++]") == 0)
			{
				volatile int x = 0;
			}

			if (!iterator.Execute(inputs[0], input_sizes[0]))
				goto fail;

			stringDist = iterator.StringDistance(outputs[0], output_sizes[0], threadBestStringScore - programSize);
			if (stringDist + programSize > threadBestStringScore) goto fail;

			lock.lock();
			if (stringDist + programSize <= this->bestStringScore)
			{
				iterator.Execute(inputs[0], input_sizes[0]);
				char postProgram[500];
				iterator.StringDistanceOutput(outputs[0], output_sizes[0], postProgram);

				programResult = std::string(iterator.GetProgram()) + std::string(postProgram);
				this->bestStringScore = stringDist + programSize;

				std::cout
					<< std::setw(3) << this->bestStringScore + output_sizes[0] 
					<< " " << programResult << std::endl;
			}
			threadBestStringScore = this->bestStringScore;
			lock.unlock();

		fail:;
		}
	}
};

template<typename PIteratorT, typename CacheT>
std::mutex ProgramSearch<PIteratorT, CacheT>::lock;
