#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <assert.h>
#include <chrono>
#include "LinearIterator.h"
#include "ModDivisionTable.h"

#define THREAD_COUNT 16
#define SIZE_START 24
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
		: inputs(inputs), input_sizes(input_sizes), outputs(outputs), output_sizes(output_sizes), count(0), sizeCount(0), programResult(""), printProgress(true)
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
	uint_fast64_t programSizeCount;
	std::chrono::time_point<std::chrono::system_clock> sizeStart;

	std::vector<const char*> inputs;
	std::vector<uint_fast32_t> input_sizes;
	std::vector<const char*> outputs;
	std::vector<uint_fast32_t> output_sizes;

	uint_fast64_t count;
	uint_fast64_t sizeCount;
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
		static const uint_fast32_t countUpdate = 10000000;

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
				this->sizeCount += countUpdate;
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

		std::cout << " Calculating program count for size " << programSize << "...\r" << std::flush;
		programSizeCount = iterators[0].ProgramCount(programSize);
		sizeCount = 0;

		sizeStart = std::chrono::system_clock::now();

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

			if (++threadCount % countUpdate == 0)
			{
				lock.lock();
				this->count += countUpdate;
				this->sizeCount += countUpdate;
				if (printProgress)
				{
					// std::ofstream file("progress.txt", std::ofstream::app);
					// file 
					// 	<< std::right << std::setw(15) << this->count
					// 	<< " " << programSize 
					// 	<< " " << iterator.GetProgram() << std::endl;
					// file.close();

					double proportion = static_cast<double>(this->sizeCount) / static_cast<double>(programSizeCount);
					auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - sizeStart).count();
					uint_fast64_t remaining = static_cast<double>(elapsed) * (1-proportion) / proportion;

					auto seconds = remaining % 60;
					auto minutes = remaining / 60 % 60;
					auto hours = remaining / 3600 % 24;
					auto days = remaining / 86400;
					
					std::cout 
						<< std::right
						<< " " << programSize
						<< " " << std::setw(10) << std::setprecision(6) << std::fixed << proportion * 100 << "%"
						<< " " << iterator.GetProgram()
						<< " Estimated remaining " << std::setw(3) << days << ":" << std::setfill('0') << std::setw(2) << hours << ":" << std::setw(2) << minutes << ":" << std::setw(2) << seconds << std::setfill(' ')
						<< "\r" << std::flush;
				}
				threadBestStringScore = this->bestStringScore;
				lock.unlock();
			}

#ifdef INITIAL_ZERO
			// if (!iterator.StartsPlus()) goto fail;
			if (!iterator.StartsPlusOrMinus()) goto fail;
			if (!iterator.IsFirstDataDeltaRight()) goto fail;
#endif
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
				// show all programs 80 characters or lower
				if (this->bestStringScore < 85-13) {
					this->bestStringScore = 85-13;
				} 

				std::cout 
					<< std::right << std::setw(3) << std::setfill(' ') << stringDist + programSize + output_sizes[0] 
					<< " " << programResult << std::endl;

				std::ofstream file("output.txt", std::ofstream::app);
				file
					<< std::right << std::setw(3) << std::setfill(' ') << stringDist + programSize + output_sizes[0] 
					<< " " << programResult << std::endl;
				file.close();
			}
			threadBestStringScore = this->bestStringScore;
			lock.unlock();

		fail:;
		}
	}
};

template<typename PIteratorT, typename CacheT>
std::mutex ProgramSearch<PIteratorT, CacheT>::lock;
