#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <assert.h>
#include <chrono>
#include "LinearIterator.h"
#include "ModDivisionTable.h"

#define THREAD_COUNT 16
#define SIZE_START 10

// #define ONLY_CHECK_ZERO_NONZERO
// #define DATA_ARBITRARY_MULTIPLE

template<typename Clock, typename Duration>
std::ostream &operator<<(std::ostream &stream,
  const std::chrono::time_point<Clock, Duration> &time_point) {
  const time_t time = Clock::to_time_t(time_point);
#if __GNUC__ > 4 || \
    ((__GNUC__ == 4) && __GNUC_MINOR__ > 8 && __GNUC_REVISION__ > 1)
  // Maybe the put_time will be implemented later?
  struct tm tm;
  localtime_r(&time, &tm);
  return stream << std::put_time(&tm, "%c"); // Print standard date&time
#else
  char buffer[26];
  ctime_r(&time, buffer);
  buffer[24] = '\0';  // Removes the newline that is added
  return stream << buffer;
#endif
}

template<typename PIteratorT, typename CacheT>
class DataProgramSearch
{
public:
	DataProgramSearch(
		std::vector<const char*> inputs,
		std::vector<uint_fast32_t> input_sizes,
		std::vector<int_fast32_t> input_offsets,
		std::vector<const char*> outputs,
		std::vector<uint_fast32_t> output_sizes,
		std::vector<int_fast32_t> output_offsets
	)
		: inputs(inputs), input_sizes(input_sizes), outputs(outputs), output_sizes(output_sizes), input_offsets(input_offsets), output_offsets(output_offsets), count(0), printProgress(true)
	{
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			threads[i] = nullptr;
		}
		cache.Create();
		Setup();
	}

	~DataProgramSearch()
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
	uint_fast32_t bestFoundOutputLength = 1;
	uint_fast32_t bestFoundProgramSize = 21;

	uint_fast32_t programSize;
	uint_fast64_t programSizeCount;
	std::chrono::time_point<std::chrono::system_clock> sizeStartTime;

	std::vector<const char*> inputs;
	std::vector<uint_fast32_t> input_sizes;
	std::vector<int_fast32_t> input_offsets;
	std::vector<const char*> outputs;
	std::vector<uint_fast32_t> output_sizes;
	std::vector<int_fast32_t> output_offsets;

	uint_fast64_t count;
	static std::mutex lock;

public:
	void Find()
	{
		uint_fast32_t programSize = SIZE_START;
		while (true)
		{
			FindSize(programSize++);
			Setup();
		}
	}

	void FindSize(uint_fast32_t programSize)
	{
		this->programSize = programSize;
		std::cout << " Calculating program count for size " << programSize << "..." << std::endl;
		programSizeCount = iterators[0].TotalCount(programSize);
		sizeStartTime = std::chrono::system_clock::now();

		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			if (threads[i]) delete threads[i];
			threads[i] = new std::thread(&DataProgramSearch::FindThread, this, i);
		}
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			threads[i]->join();
		}
	}

private:
	void FindThread(uint_fast32_t threadIdx)
	{
		static const uint_fast32_t countUpdate = 1000000;

		auto& iterator = iterators[threadIdx];
		iterator.Start(programSize, threadIdx, THREAD_COUNT);

		uint_fast64_t threadCount = 0;
		while (iterator.Next())
		{
			if (++threadCount % countUpdate == 0)
			{
				lock.lock();
				
				this->count += countUpdate;
				uint_fast64_t currentCount = 0;
				for (int i = 0; i < THREAD_COUNT; i++)
				{
					currentCount += iterators[i].CurrentCount();
				}

				double completion = static_cast<double>(currentCount) / static_cast<double>(programSizeCount);
				double elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - sizeStartTime).count();

				uint_fast64_t remaining = elapsed * (1-completion) / completion;
				auto seconds = remaining % 60;
				auto minutes = remaining / 60 % 60;
				auto hours = remaining / 3600 % 24;
				auto days = remaining / 86400;

				auto eta = std::chrono::system_clock::now() + std::chrono::seconds(remaining);

				if (printProgress) {
					std::cout
						<< std::right << std::setw(3) << std::setfill(' ') << programSize
						<< " " << std::setw(10) << std::setprecision(6) << std::fixed << completion * 100 << "%"
						<< " " << iterator.GetProgram()
						<< " Rem " << std::setw(3) << days << ":" << std::setfill('0') << std::setw(2) << hours << ":" << std::setw(2) << minutes << ":" << std::setw(2) << seconds << std::setfill(' ')
						<< " - Eta " << eta
						<< '          \r' << std::flush
					;
				}

				lock.unlock();
			}

#ifdef DATA_ARBITRARY_MULTIPLE
			uint8_t multiple;
#endif

			for (uint_fast32_t i = 0; i < inputs.size(); i++)
			{
				if (!iterator.Execute(inputs[i], input_sizes[i], input_offsets[i]))
				{
					goto fail;
				}
#ifdef ONLY_CHECK_ZERO_NONZERO
				if (!iterator.DataEqualZeroOrNonzero(outputs[i], output_sizes[i], output_offsets[i]))
				{
					goto fail;
				}
#else
#ifdef DATA_ARBITRARY_MULTIPLE
				if (i == 0)
				{
					for (multiple = 1; multiple != 0; multiple++)
					{
						if (multiple == 128)
						{
							continue;
						}
						if (iterator.DataEqualMultiple(outputs[i], output_sizes[i], output_offsets[i], multiple))
						{
							goto found_multiple;
						}
					}
					goto fail;
				found_multiple:;
				}
				else if (!iterator.DataEqualMultiple(outputs[i], output_sizes[i], output_offsets[i], multiple))
				{
					goto fail;
				}
#else
				if (!iterator.DataEqual(outputs[i], output_sizes[i], output_offsets[i]))
				{
					goto fail;
				}
#endif
#endif
			}

			lock.lock();
			
			std::cout
				<< std::right << std::setw(3) << std::setfill(' ') << programSize
#ifdef DATA_ARBITRARY_MULTIPLE
				<< " " << std::setw(3) << std::setfill(' ') << static_cast<int>(multiple)
#endif
				<< " " << std::string(iterator.GetProgram())
				<< "      " << std::endl
			;

			// std::cin.ignore();
			lock.unlock();

			// uint_fast32_t output_idx = 0;
			// if (iterator.Execute(input, input_size, output, output_size, &output_idx))
			// {
			// 	if (
			// 		output_idx == bestFoundOutputLength && programSize <= bestFoundProgramSize
			// 		|| output_idx > bestFoundOutputLength
			// 		// for now who everything of at least size 2
			// 		|| output_idx >= 5
			// 	)
			// 	{
			// 		lock.lock();
					
			// 		bestFoundOutputLength = output_idx;
			// 		bestFoundProgramSize = programSize;

			// 		std::string result = std::string(iterator.GetProgram());
			// 		std::cout
			// 			<< std::right << std::setw(3) << std::setfill(' ') << programSize
			// 			<< " " << output_idx
			// 			<< " " << result << "      " << std::endl
			// 		;

			// 		// std::cin.ignore();

			// 		lock.unlock();
			// 	}
			// }

		fail:;
		}
	}

// DEPRACATED
// ================================================================================

// public:
// 	void FindString()
// 	{
// 		bestStringScore = 10000000;
// 		uint_fast32_t programSize = SIZE_START;
// 		programResult = std::string();
// 		while (true)
// 		{
// 			FindStringSize(programSize++);
// 			Setup();
// 		}
// 	}

// 	void FindStringSize(uint_fast32_t programSize)
// 	{
// 		this->programSize = programSize;

// 		std::cout << " Calculating program count for size " << programSize << "...\r" << std::flush;
// 		programSizeCount = iterators[0].ProgramCount(programSize);
// 		sizeCount = 0;

// 		sizeStart = std::chrono::system_clock::now();

// 		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
// 		{
// 			if (threads[i]) delete threads[i];
// 			threads[i] = new std::thread(&DataProgramSearch::FindStringThread, this, i);
// 		}
// 		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
// 		{
// 			threads[i]->join();
// 		}
// 	}

// private:
// 	uint_fast32_t bestStringScore;

// 	void FindStringThread(uint_fast32_t threadIdx)
// 	{
// 		static const uint_fast32_t countUpdate = 1000000;

// 		auto& iterator = iterators[threadIdx];
// 		iterator.Start(programSize, threadIdx, THREAD_COUNT);

// 		lock.lock();
// 		uint_fast32_t threadBestStringScore = this->bestStringScore;
// 		lock.unlock();
// 		uint_fast32_t threadCount = 0;
// 		while (iterator.Next())
// 		{
// 			uint_fast32_t stringDist;

// 			if (++threadCount % countUpdate == 0)
// 			{
// 				lock.lock();
// 				this->count += countUpdate;
// 				this->sizeCount += countUpdate;
// 				if (printProgress)
// 				{
// 					// std::ofstream file("progress.txt", std::ofstream::app);
// 					// file 
// 					// 	<< std::right << std::setw(15) << this->count
// 					// 	<< " " << programSize 
// 					// 	<< " " << iterator.GetProgram() << std::endl;
// 					// file.close();

// 					double proportion = static_cast<double>(this->sizeCount) / static_cast<double>(programSizeCount);
// 					auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - sizeStart).count();
// 					uint_fast64_t remaining = static_cast<double>(elapsed) * (1-proportion) / proportion;

// 					auto seconds = remaining % 60;
// 					auto minutes = remaining / 60 % 60;
// 					auto hours = remaining / 3600 % 24;
// 					auto days = remaining / 86400;
					
// 					std::cout 
// 						<< std::right
// 						<< " " << programSize
// 						<< " " << std::setw(10) << std::setprecision(6) << std::fixed << proportion * 100 << "%"
// 						<< " " << iterator.GetProgram()
// 						<< " Estimated remaining " << std::setw(3) << days << ":" << std::setfill('0') << std::setw(2) << hours << ":" << std::setw(2) << minutes << ":" << std::setw(2) << seconds << std::setfill(' ')
// 						<< "\r" << std::flush;
// 				}
// 				threadBestStringScore = this->bestStringScore;
// 				lock.unlock();
// 			}

// #ifdef INITIAL_ZERO
// 			// if (!iterator.StartsPlus()) goto fail;
// 			if (!iterator.StartsPlusOrMinus()) goto fail;
// 			if (!iterator.IsFirstDataDeltaRight()) goto fail;
// #endif
// 			if (!iterator.Execute(inputs[0], input_sizes[0]))
// 				goto fail;

// 			stringDist = iterator.StringDistance(outputs[0], output_sizes[0], threadBestStringScore - programSize);
// 			if (stringDist + programSize > threadBestStringScore) goto fail;

// 			lock.lock();
// 			if (stringDist + programSize <= this->bestStringScore)
// 			{
// 				iterator.Execute(inputs[0], input_sizes[0]);
// 				char postProgram[500];
// 				iterator.StringDistanceOutput(outputs[0], output_sizes[0], postProgram);

// 				programResult = std::string(iterator.GetProgram()) + std::string(postProgram);
				
// 				this->bestStringScore = stringDist + programSize;
// 				// show all programs 80 characters or lower
// 				if (this->bestStringScore < 85-13) {
// 					this->bestStringScore = 85-13;
// 				} 

// 				std::cout 
// 					<< std::right << std::setw(3) << std::setfill(' ') << stringDist + programSize + output_sizes[0] 
// 					<< " " << programResult << std::endl;

// 				std::ofstream file("output.txt", std::ofstream::app);
// 				file
// 					<< std::right << std::setw(3) << std::setfill(' ') << stringDist + programSize + output_sizes[0] 
// 					<< " " << programResult << std::endl;
// 				file.close();
// 			}
// 			threadBestStringScore = this->bestStringScore;
// 			lock.unlock();

// 		fail:;
// 		}
// 	}
};

template<typename PIteratorT, typename CacheT>
std::mutex DataProgramSearch<PIteratorT, CacheT>::lock;
