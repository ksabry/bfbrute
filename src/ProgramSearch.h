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
#define SHOW_ALL_PROGRAMS_LENGTH 82

template<typename PIteratorT, typename CacheT>
class ProgramSearch
{
public:
	ProgramSearch(
		std::vector<const char*> inputs,
		std::vector<const char*> outputs
	)
		: ProgramSearch(inputs, GetStringSizes(inputs), outputs, GetStringSizes(outputs))
	{
	}
	ProgramSearch(
		std::vector<const char*> inputs,
		std::vector<uint_fast32_t> input_sizes,
		std::vector<const char*> outputs
	)
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
		programResult = std::string();
		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			if (threads[i]) delete threads[i];
			threads[i] = nullptr;

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
		if (!LoadFindStringProgress())
		{
			programSize = SIZE_START;
			SetupSize();
		}
		
		FindStringSize();

		while (true)
		{
			programSize++;
			SetupSize();
			FindStringSize();
		}
	}

	void CountSize()
	{
		PIteratorT iterator;
		iterator.SetCache(&cache);
		iterator.SetDivisionTable(&divisionTable);

		std::cout << " Calculating program count for size " << programSize << "...\r" << std::flush;
		programSizeCount = iterator.TotalCount(programSize);
		std::cout << std::endl << " Completed program count calculation for size " << programSize << std::endl;
	}

	void SetupSize()
	{
		std::cout << "Setting up size " << programSize << std::endl;

		sizeStart = std::chrono::system_clock::now();
		for (uint_fast32_t thread_idx = 0; thread_idx < THREAD_COUNT; thread_idx++)
		{
			auto& iterator = iterators[thread_idx];
			iterator.Start(programSize, thread_idx, THREAD_COUNT);
		}
	}

	void FindStringSize()
	{
		CountSize();

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
	void FindStringThread(uint_fast32_t threadIdx)
	{
		static const uint_fast32_t countUpdate = 1000000;
		static const uint_fast32_t countSave = 10000000;

		auto& iterator = iterators[threadIdx];

		uint_fast32_t threadCount = 0;
		while (iterator.Next())
		{
			uint_fast32_t stringDist;

			if (++threadCount % countUpdate == 0)
			{
				lock.lock();
				this->count += countUpdate;

				if (printProgress)
				{
					uint_fast64_t currentCount = 0;
					for (int i = 0; i < THREAD_COUNT; i++)
					{
						currentCount += iterators[i].currentCount;
					}

					double proportion = static_cast<double>(currentCount) / static_cast<double>(programSizeCount);
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
				lock.unlock();
			}

			if (threadCount % countSave == 0)
			{
				SaveFindStringProgress();
			}

			if (!iterator.Execute(inputs[0], input_sizes[0]))
			{
				continue;
			}

			stringDist = iterator.StringDistance(outputs[0], output_sizes[0], SHOW_ALL_PROGRAMS_LENGTH - programSize);
			if (stringDist + programSize > SHOW_ALL_PROGRAMS_LENGTH)
			{
				continue;
			}

			std::string postProgram = iterator.StringDistanceOutput(outputs[0], output_sizes[0], SHOW_ALL_PROGRAMS_LENGTH - programSize);
			programResult = std::string(iterator.GetProgram()) + postProgram;

			lock.lock();

			std::cout 
				<< std::right << std::setw(3) << std::setfill(' ') << stringDist + programSize
				<< " " << programResult << std::endl;

			std::ofstream file("output.txt", std::ofstream::app);
			file
				<< std::right << std::setw(3) << std::setfill(' ') << stringDist + programSize
				<< " " << programResult << std::endl;
			file.close();
			
			if (printProgress)
			{
				uint_fast64_t currentCount = 0;
				for (int i = 0; i < THREAD_COUNT; i++)
				{
					currentCount += iterators[i].currentCount;
				}

				double proportion = static_cast<double>(currentCount) / static_cast<double>(programSizeCount);
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

			lock.unlock();

			SaveFindStringProgress();
		}
	}

	std::string StringToHex(const std::string& input)
	{
		static const char hex_digits[] = "0123456789ABCDEF";

		std::string output;
		output.reserve(input.length() * 2);
		for (unsigned char c : input)
		{
			output.push_back(hex_digits[c >> 4]);
			output.push_back(hex_digits[c & 15]);
		}
		return output;
	}

	std::string Filename()
	{
		std::string filename = "/home/ksabry/dev/bfbrute/progress/program_search";

		for (std::string input : inputs)
		{
			filename += "_" + StringToHex(input);
		}
		filename += "_";
		for (std::string output : outputs)
		{
			filename += "_" + StringToHex(output);
		}

		filename += "__" + std::to_string(THREAD_COUNT);
		// SIZE_START is not necessary
		filename += "_" + std::to_string(SHOW_ALL_PROGRAMS_LENGTH);
		filename += "_" + std::to_string(MAX_JUMPS);

#ifdef SINGLE_BRACKET_HIERARCHY
		filename += "_T";
#else
		filename += "_F";
#endif

#ifdef SHORT_CIRCUIT_LINEAR_SINGULAR
		filename += "_T";
#else
		filename += "_F";
#endif

#ifdef INITIAL_ZERO
		filename += "_T";
#else
		filename += "_F";
#endif

#ifdef INITIAL_DATA_SYMMETRIC
		filename += "_T";
#else
		filename += "_F";
#endif

#ifdef NO_ENDING
		filename += "_T";
#else
		filename += "_F";
#endif

#ifdef CASE_INSENSITIVE
		filename += "_T";
#else
		filename += "_F";
#endif

#ifdef MAX_BRACKET_DEPTH
		filename += "_" + std::to_string(MAX_BRACKET_DEPTH);
#else
		filename += "_F";
#endif

#ifdef SINGLE_ITER_COUNT
		filename += "_" + std::to_string(SINGLE_ITER_COUNT);
#else
		filename += "_F";
#endif

#ifdef MAX_FIRST_SIZE
		filename += "_" + std::to_string(MAX_FIRST_SIZE);
#else
		filename += "_F";
#endif

		return filename;
	}

	bool LoadFindStringProgress()
	{
		std::cout << "Attempting to load progress file" << std::endl;

		lock.lock();

		std::string filename = Filename();
		std::ifstream file(filename);

		if (!file.good())
		{
			std::cout << "No progress file found; starting from size " << SIZE_START << std::endl;
			file.close();
			lock.unlock();
			return false;
		}

		int64_t elapsed;

		file >> programSize;
		file >> elapsed;
		sizeStart = std::chrono::system_clock::now() - std::chrono::seconds(elapsed);

		std::cout << "Progress file found; resuming size " << programSize << " (0/" << THREAD_COUNT << " loaded)\r" << std::flush;

		for (uint_fast32_t thread_idx = 0; thread_idx < THREAD_COUNT; thread_idx++)
		{
			if (!iterators[thread_idx].Deserialize(file))
			{
				std::cout << "Failed to load progress file; starting from size " << SIZE_START << std::endl;
				file.close();
				lock.unlock();
				return false;
			}
			std::cout << "Progress file found; resuming size " << programSize << " (" << thread_idx + 1 << "/" << THREAD_COUNT << " loaded)\r" << std::flush;
		}

		std::cout << std::endl;
		file.close();
		lock.unlock();
		return true;
	}

	void SaveFindStringProgress()
	{
		lock.lock();

		std::string filename = Filename();
		std::ofstream file(filename, std::ofstream::trunc);
		
		file << programSize << "\n";
		int64_t elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - sizeStart).count();
		file << elapsed << "\n\n";

		for (uint_fast32_t i = 0; i < THREAD_COUNT; i++)
		{
			iterators[i].Serialize(file);
			file << "\n\n";
		}
		file.close();

		lock.unlock();
	}
};

template<typename PIteratorT, typename CacheT>
std::mutex ProgramSearch<PIteratorT, CacheT>::lock;
