#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <assert.h>
#include <chrono>
#include "LinearIterator.h"
#include "ModDivisionTable.h"

#define SIZE_START 1
#define THREAD_COUNT 16

#define MULTIPLE_MAX_TOTAL_LENGTH 70
#define MULTIPLE_MAX_SINGLE_LENGTH 10
#define MULTIPLE_MAX_FIRST_LENGTH 30
#define MULTIPLE_START_FIRST_LENGTH 15

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

template<typename PIteratorT, typename CacheT, typename RawSourceExecutorT>
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

	std::string findMultiplePrecedingProgram;
	PIteratorT findMultipleIterators[THREAD_COUNT * 20];
	uint_fast64_t findMultipleCounts[THREAD_COUNT];
	uint_fast32_t findMultipleBestScore;

	void FindMultipleFromInitialProgram(std::string initialProgram)
	{
		findMultipleCounts[0] = 0;
		findMultipleBestScore = MULTIPLE_MAX_TOTAL_LENGTH + 1;
		findMultiplePrecedingProgram = initialProgram;
		RawSourceExecutorT executor(initialProgram);
		if (!executor.Execute())
		{
			std::cout << "Initial program execution failed";
			return;
		}
		FindMultipleRecursive(0, executor.GetData(), executor.GetDataIdx(), 0, initialProgram.length());
	}

	void FindMultipleFromInitialData(uint8_t* initial_data, uint_fast32_t initial_data_idx, uint_fast32_t initial_length)
	{
		findMultipleCounts[0] = 0;
		findMultipleBestScore = MULTIPLE_MAX_TOTAL_LENGTH + 1;
		FindMultipleRecursive(0, initial_data, initial_data_idx, 0, initial_length);
	}

	void FindMultipleRecursive(uint_fast32_t iterator_idx, uint8_t* data, uint_fast32_t data_idx, uint_fast32_t start_output_idx, uint_fast32_t current_length)
	{
		PIteratorT& iterator = findMultipleIterators[iterator_idx];
		iterator.SetCache(&cache);
		iterator.SetDivisionTable(&divisionTable);

		uint_fast32_t limit = iterator_idx == 0 ? MULTIPLE_MAX_FIRST_LENGTH : MULTIPLE_MAX_SINGLE_LENGTH;

		for (uint_fast32_t program_size = 1; program_size <= limit; program_size++)
		{
			uint_fast32_t next_length = current_length + program_size;
			if (next_length >= findMultipleBestScore)
			{
				return;
			}

			iterator.Start(program_size, 0, 1);
			while (iterator.Next())
			{
				if (++findMultipleCounts[0] % 10000000 == 0)
				// if (++findMultipleCount % 1 == 0)
				{
					lock.lock();

					std::string program = GetFindMultipleProgram(0, iterator_idx, true);

					std::cout
						<< std::right << std::setfill(' ')
						<< std::setw(3) << findMultipleIterators[0].programSize << " "
						<< std::setw(12) << findMultipleCounts[0] << " "
						<< std::string(program) << "      "
						<< '\r' << std::flush
					;

					// if (iterator_idx > 0)
					// {
					// 	std::cin.ignore();
					// }

					lock.unlock();
				}

				uint_fast32_t output_idx = start_output_idx;
				if (!iterator.ExecuteRawData(data, data_idx, outputs[0], output_sizes[0], &output_idx))
				{
					continue;
				}
				// skip programs that produce no relevent output
				if (output_idx == start_output_idx)
				{
					continue;
				}

				// std::string program = GetFindMultipleProgram(0, iterator_idx, true);
				// std::cout
				// 	<< std::endl
				// 	<< std::string(program)
				// 	<< std::endl
				// ;
				// std::cin.ignore();

				if (output_idx >= output_sizes[0])
				{
					lock.lock();

					std::string program = GetFindMultipleProgram(0, iterator_idx);
					findMultipleBestScore = next_length;

					std::cout
						<< std::right << std::setw(3) << std::setfill(' ') << next_length
						<< " " << std::string(program) << "      " << std::endl
					;

					std::ofstream file("output.txt", std::ofstream::app);
					file
						<< std::right << std::setw(3) << std::setfill(' ') << next_length
						<< " " << std::string(program)
						<< std::endl;
					file.close();

					// std::cin.ignore();
					lock.unlock();
				}

				// Heuristic pruning

				int_fast32_t remaining_output = output_sizes[0] - output_idx;
				int_fast32_t remaining_length = findMultipleBestScore - next_length;

				// std::cout << GetFindMultipleProgram(0, iterator_idx, true) << std::endl;
				// std::cout << remaining_length << " " << remaining_output << " " << remaining_output * 3.5 << std::endl;
				// std::cin.ignore();

				// Potentially experiment with these values
				// if (remaining_length < (remaining_output - 1) * 3.5) // For "Hello, World!"
				// if (remaining_length < (remaining_output - 1) * 4)
				// {
				// 	continue;
				// }

				uint8_t* next_data = iterator.Data();
				uint_fast32_t next_data_idx = iterator.DataIdx();
				FindMultipleRecursive(iterator_idx + 1, next_data, next_data_idx, output_idx, next_length);
			}
		}
	}

	// Threaded

	void FindMultipleFromInitialProgramThreaded(std::string initialProgram)
	{
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			findMultipleCounts[i] = 0;
		}
		findMultipleBestScore = MULTIPLE_MAX_TOTAL_LENGTH + 1;
		findMultiplePrecedingProgram = initialProgram;
		RawSourceExecutorT executor(initialProgram);
		if (!executor.Execute())
		{
			std::cout << "Initial program execution failed";
			return;
		}

		// FindMultipleRecursiveThread(0, 0, executor.GetData(), executor.GetDataIdx(), 0, initialProgram.length());

		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			if (threads[i]) delete threads[i];
			threads[i] = new std::thread(&OutputProgramSearch::FindMultipleRecursiveThread, this, i, 0, executor.GetData(), executor.GetDataIdx(), 0, initialProgram.length());
		}
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			threads[i]->join();
		}
	}

	void FindMultipleRecursiveThread(uint_fast32_t thread_idx, uint_fast32_t iterator_idx, uint8_t* data, uint_fast32_t data_idx, uint_fast32_t start_output_idx, uint_fast32_t current_length)
	{
		// lock.lock();
		// std::cout << thread_idx << " " << iterator_idx << std::endl;
		// lock.unlock();

		PIteratorT& iterator = findMultipleIterators[20 * thread_idx + iterator_idx];
		iterator.SetCache(&cache);
		iterator.SetDivisionTable(&divisionTable);

		uint_fast32_t start_program_size = iterator_idx == 0 ? MULTIPLE_START_FIRST_LENGTH : 1;
		uint_fast32_t end_program_size = iterator_idx == 0 ? MULTIPLE_MAX_FIRST_LENGTH : MULTIPLE_MAX_SINGLE_LENGTH;

		for (uint_fast32_t program_size = start_program_size; program_size <= end_program_size; program_size++)
		{
			uint_fast32_t next_length = current_length + program_size;
			if (next_length >= findMultipleBestScore)
			{
				return;
			}

			if (iterator_idx == 0)
			{
				iterator.Start(program_size, thread_idx, THREAD_COUNT, true);
			}
			else
			{
				iterator.Start(program_size, 0, 1);
			}

			while (iterator.Next())
			{
				// std::cout << thread_idx << " " << iterator.GetProgram() << std::endl;
				// std::cin.ignore();
				
				if (++findMultipleCounts[thread_idx] % 10000000 == 0)
				{
					lock.lock();

					uint_fast64_t total_count = 0;
					for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
					{
						total_count += findMultipleCounts[i];
					}
					
					std::string program = GetFindMultipleProgram(thread_idx, iterator_idx, true);

					std::cout
						<< std::right << std::setfill(' ')
						<< std::setw(3) << findMultipleIterators[20 * thread_idx].programSize << " "
						<< std::left << std::setw(MULTIPLE_MAX_TOTAL_LENGTH + 15) << std::string(program) << " "
						<< std::right << std::setw(12) << total_count << " "
						// << std::setw(3) << thread_idx << " : "
					;

					// for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
					// {
					// 	std::cout <<
					// 		i << "|" << findMultipleCounts[i] << " ";
					// 	;
					// }

					std::cout
						<< '\r' << std::flush
					;

					lock.unlock();
				}

				uint_fast32_t output_idx = start_output_idx;
				if (!iterator.ExecuteRawData(data, data_idx, outputs[0], output_sizes[0], &output_idx))
				{
					continue;
				}
				// skip programs that produce no relevent output
				if (output_idx == start_output_idx)
				{
					continue;
				}

				// std::string program = GetFindMultipleProgram(thread_idx, iterator_idx, true);
				// std::cout
				// 	<< std::endl
				// 	<< std::string(program)
				// 	<< std::endl
				// ;
				// std::cin.ignore();

				if (output_idx >= output_sizes[0])
				{
					lock.lock();

					std::string program = GetFindMultipleProgram(thread_idx, iterator_idx);
					findMultipleBestScore = next_length;

					std::cout
						<< std::right << std::setw(3) << std::setfill(' ') << next_length
						<< " " << std::string(program) << "      " << std::endl
					;

					std::ofstream file("output.txt", std::ofstream::app);
					file
						<< std::right << std::setw(3) << std::setfill(' ') << next_length
						<< " " << std::string(program)
						<< std::endl;
					file.close();

					// std::cin.ignore();
					lock.unlock();
				}

				// Heuristic pruning

				// int_fast32_t remaining_output = output_sizes[0] - output_idx;
				// int_fast32_t remaining_length = findMultipleBestScore - next_length;

				// std::cout << GetFindMultipleProgram(thread_idx, iterator_idx, true) << std::endl;
				// std::cout << remaining_length << " " << remaining_output << " " << remaining_output * 3.5 << std::endl;
				// std::cin.ignore();

				// Potentially experiment with these values
				// if (remaining_length < (remaining_output - 1) * 3.5) // For "Hello, World!"
				// if (remaining_length < (remaining_output - 1) * 4)
				// {
				// 	continue;
				// }

				uint8_t* next_data = iterator.Data();
				uint_fast32_t next_data_idx = iterator.DataIdx();
				FindMultipleRecursiveThread(thread_idx, iterator_idx + 1, next_data, next_data_idx, output_idx, next_length);
			}
		}
	}

	std::string GetFindMultipleProgram(uint_fast32_t thread_idx, uint_fast32_t last_iterator_idx, bool separate = false)
	{
		std::string result = findMultiplePrecedingProgram;
		for (uint_fast32_t iterator_idx = thread_idx * 20; iterator_idx <= thread_idx * 20 + last_iterator_idx; iterator_idx++)
		{
			if (separate)
			{
				result += "|";
			}
			result += std::string(findMultipleIterators[iterator_idx].GetProgram());
		}
		return result;
	}
};

template<typename PIteratorT, typename CacheT, typename RawSourceExecutorT>
std::mutex OutputProgramSearch<PIteratorT, CacheT, RawSourceExecutorT>::lock;
