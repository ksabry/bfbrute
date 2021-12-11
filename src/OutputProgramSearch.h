#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <assert.h>
#include <chrono>
#include "LinearIterator.h"
#include "ModDivisionTable.h"

#define THREAD_COUNT 16
#define SIZE_START 23

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
class OutputProgramSearch
{
public:
	OutputProgramSearch(
		std::vector<const char*> inputs,
		std::vector<const char*> outputs
	)
		: OutputProgramSearch(inputs, GetStringSizes(inputs), outputs, GetStringSizes(outputs))
	{
	}
	OutputProgramSearch(
		std::vector<const char*> inputs,
		std::vector<uint_fast32_t> input_sizes,
		std::vector<const char*> outputs
	)
		: OutputProgramSearch(inputs, input_sizes, outputs, GetStringSizes(outputs))
	{
	}
	OutputProgramSearch(
		std::vector<const char*> inputs,
		std::vector<const char*> outputs,
		std::vector<uint_fast32_t> output_sizes
	)
		: OutputProgramSearch(inputs, GetStringSizes(inputs), outputs, output_sizes)
	{
	}

	OutputProgramSearch(
		std::vector<const char*> inputs,
		std::vector<uint_fast32_t> input_sizes,
		std::vector<const char*> outputs,
		std::vector<uint_fast32_t> output_sizes
	)
		: inputs(inputs), input_sizes(input_sizes), outputs(outputs), output_sizes(output_sizes), count(0), printProgress(true)
	{
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			threads[i] = nullptr;
		}
		cache.Create();
		Setup();
	}

	~OutputProgramSearch()
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
	uint_fast32_t bestFoundProgramSize = 10;

	uint_fast32_t programSize;
	uint_fast64_t programSizeCount;
	std::chrono::time_point<std::chrono::system_clock> sizeStartTime;

	std::vector<const char*> inputs;
	std::vector<uint_fast32_t> input_sizes;
	std::vector<const char*> outputs;
	std::vector<uint_fast32_t> output_sizes;

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
			threads[i] = new std::thread(&OutputProgramSearch::FindThread, this, i);
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
						<< '\r' << std::flush
					;
				}

				lock.unlock();
			}

			uint_fast32_t lowest_output_idx = 100000;
			for (uint_fast32_t i = 0; i < inputs.size(); i++)
			{
				uint_fast32_t output_idx = 0;
				if (!iterator.Execute(inputs[i], input_sizes[i], outputs[i], output_sizes[i], &output_idx))
				{
					goto fail;
				}
				if (
					(output_idx != bestFoundOutputLength || programSize > bestFoundProgramSize)
					&& output_idx <= bestFoundOutputLength
					// for now who everything of at least a certain size
					// && output_idx < 10
				)
				{
					goto fail;
				}
				if (output_idx < lowest_output_idx)
				{
					lowest_output_idx = output_idx;
				}
			}

			lock.lock();
			
			if (lowest_output_idx > bestFoundOutputLength)
			{
				bestFoundOutputLength = lowest_output_idx;
				bestFoundProgramSize = programSize;
			}

			std::cout
				<< std::right << std::setw(3) << std::setfill(' ') << programSize
				<< " " << lowest_output_idx
				<< " " << std::string(iterator.GetProgram()) << "      " << std::endl
			;

			{
				std::ofstream file("output.txt", std::ofstream::app);
				file
					<< std::right << std::setw(3) << std::setfill(' ') << programSize
					<< " " << lowest_output_idx
					<< " " << std::string(iterator.GetProgram())
					<< std::endl;
				file.close();
			}

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
};

template<typename PIteratorT, typename CacheT>
std::mutex OutputProgramSearch<PIteratorT, CacheT>::lock;
