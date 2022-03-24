#include "RawSourceExecutor.h"

void ExecuteFromSource()
{
	RawSourceExecutor<400> executor("+[-[>+>]<<+<+<-<-<+<++]");
	executor.Execute();
	executor.PrintData();
}

// Brute force using the hunt and peck method; will save resumable progress files

// #include "ProgramIterator.h"
// #include "ProgramSearch.h"

// void FindString()
// {
// 	constexpr uint_fast32_t DATA_SIZE = 400;
// 	constexpr uint_fast32_t CACHE_DATA_SIZE = 32;
// 	constexpr uint_fast32_t CACHE_SIZE = 12;
	
// 	std::vector<const char*> inputs = {
// 		""
// 	};
// 	std::vector<const char*> outputs = {
// 		"Hello, World!"
// 	};

// 	ProgramSearch<
// 		ProgramIterator<DATA_SIZE, CACHE_DATA_SIZE, CACHE_SIZE>,
// 		DataCache<CACHE_DATA_SIZE, CACHE_SIZE>>
// 	search(inputs, outputs);

// 	search.FindString();
// }

// Brute force including the '.' character

// #include "OutputProgramIterator.h"
// #include "OutputProgramSearch.h"

// void FindStringOutput()
// {
// 	constexpr uint_fast32_t DATA_SIZE = 400;
// 	constexpr uint_fast32_t CACHE_DATA_SIZE = 32;
// 	constexpr uint_fast32_t CACHE_SIZE = 12;
	
// 	std::vector<const char*> inputs = {
// 		"",
// 	};
// 	std::vector<const char*> outputs = {
// 		"Hello",
// 	};

// 	OutputProgramSearch<
// 		OutputProgramIterator<DATA_SIZE, CACHE_DATA_SIZE, CACHE_SIZE>,
// 		DataCache<CACHE_DATA_SIZE, CACHE_SIZE>,
// 		RawSourceExecutor<DATA_SIZE>>
// 	search(inputs, outputs);

// 	search.Find();
// }

// Brute force given a target data on the tape (rather than actual output)

#include "DataProgramIterator.h"
#include "DataProgramSearch.h"

void FindData()
{
	constexpr uint_fast32_t DATA_SIZE = 400;
	constexpr uint_fast32_t CACHE_DATA_SIZE = 32;
	constexpr uint_fast32_t CACHE_SIZE = 12;

	// TODO: add constraint in DataProgramSearch to have the result data index be the same across inputs?

	// A*3 + B*2

	std::vector<const char*> inputs = {
		"\x01\x01", "\x02\x01", "\x03\x02", "\x06\x02", "\x05\x04"
	};
	std::vector<uint_fast32_t> input_sizes = {
		2, 2, 2, 2, 2
	};
	std::vector<int_fast32_t> input_offsets = {
		0, 0, 0, 0, 0
	};
	
	std::vector<const char*> outputs = {
		"\x05", "\x08", "\x0D", "\x16", "\x17"
	};
	std::vector<uint_fast32_t> output_sizes = {
		1, 1, 1, 1, 1
	};
	std::vector<int_fast32_t> output_offsets = {
		0, 0, 0, 0, 0
	};
	DataProgramSearch<
		DataProgramIterator<DATA_SIZE, CACHE_DATA_SIZE, CACHE_SIZE>,
		DataCache<CACHE_DATA_SIZE, CACHE_SIZE>>
	search(inputs, input_sizes, input_offsets, outputs, output_sizes, output_offsets);

	search.Find();
}

int main()
{
	// double first = static_cast<double>(clock()) / CLOCKS_PER_SEC;
	
	FindData();

	// double second = static_cast<double>(clock()) / CLOCKS_PER_SEC;
	// std::cout << "Completed in " << second - first << " seconds";

	return 0;
}
