// #include "ProgramIterator.h"
// #include "ProgramSearch.h"

// #include "OutputProgramIterator.h"

// #include "RawSourceExecutor.h"

// void test()
// {
// 	RawSourceExecutor<400> executor("+[-[>+>]<<+<+<-<-<+<++]");
// 	executor.Execute();
// 	executor.PrintData();
// }

#include "ProgramIterator.h"
// #include "LLRR_ProgramIterator.h"
#include "ProgramSearch.h"

void search()
{
	constexpr uint_fast32_t DATA_SIZE = 400;
	constexpr uint_fast32_t CACHE_DATA_SIZE = 32;
	constexpr uint_fast32_t CACHE_SIZE = 16;
	
	std::vector<const char*> inputs = {
		""
	};
	std::vector<const char*> outputs = {
		"Hello, World!"
	};

	// ksabry.io
	// -[->--[>]+[+++<]>>++]>>>>--.<.>>>.+.<<<-.<.<+.>>>--.<---.

	ProgramSearch<
		ProgramIterator<DATA_SIZE, CACHE_DATA_SIZE, CACHE_SIZE>, 
		DataCache<CACHE_DATA_SIZE, CACHE_SIZE>> 
	search(inputs, outputs);

	//search.printProgress = false;
	//std::string result = search.Find();
	//std::cout << result << " " << result.size() << std::endl;
	search.FindString();
}

// #include "OutputProgramIterator.h"
// #include "OutputProgramSearch.h"

// void search()
// {
// 	constexpr uint_fast32_t DATA_SIZE = 400;
// 	constexpr uint_fast32_t CACHE_DATA_SIZE = 32;
// 	constexpr uint_fast32_t CACHE_SIZE = 12;
	
// 	std::vector<const char*> inputs = {
// 		"",
// 	};
// 	std::vector<const char*> outputs = {
// 		"*\n*\n*\n*\n*\n*\n*\n*\n*\n*\n",
// 	};

// 	OutputProgramSearch<
// 		OutputProgramIterator<DATA_SIZE, CACHE_DATA_SIZE, CACHE_SIZE>,
// 		DataCache<CACHE_DATA_SIZE, CACHE_SIZE>>
// 	search(inputs, outputs);

// 	//search.printProgress = false;
// 	//std::string result = search.Find();
// 	//std::cout << result << " " << result.size() << std::endl;
// 	search.Find();
// }

// #include "DataProgramIterator.h"
// #include "DataProgramSearch.h"

// void search()
// {
// 	constexpr uint_fast32_t DATA_SIZE = 400;
// 	constexpr uint_fast32_t CACHE_DATA_SIZE = 32;
// 	constexpr uint_fast32_t CACHE_SIZE = 12;

// 	// TODO: add constraint in DataProgramSearch to have the result data index be the same across inputs?

// 	std::vector<const char*> inputs = {
// 		""
// 	};
// 	std::vector<uint_fast32_t> input_sizes = {
// 		0
// 	};
// 	std::vector<int_fast32_t> input_offsets = {
// 		0
// 	};
	
// 	std::vector<const char*> outputs = {
// 		"\x0A\x14\x28"
// 	};
// 	std::vector<uint_fast32_t> output_sizes = {
// 		3
// 	};
// 	std::vector<int_fast32_t> output_offsets = {
// 		0
// 	};
// 	DataProgramSearch<
// 		DataProgramIterator<DATA_SIZE, CACHE_DATA_SIZE, CACHE_SIZE>,
// 		DataCache<CACHE_DATA_SIZE, CACHE_SIZE>>
// 	search(inputs, input_sizes, input_offsets, outputs, output_sizes, output_offsets);

// 	//search.printProgress = false;
// 	//std::string result = search.Find();
// 	//std::cout << result << " " << result.size() << std::endl;
// 	search.Find();
// }

// #include "OutputProgramIterator.h"
// #include "OutputProgramSearch.h"
// #include "RawSourceExecutor.h"

// void search()
// {
// 	constexpr uint_fast32_t DATA_SIZE = 400;
// 	constexpr uint_fast32_t CACHE_DATA_SIZE = 32;
// 	constexpr uint_fast32_t CACHE_SIZE = 14;
	
// 	std::vector<const char*> inputs = {
// 		"",
// 	};
// 	std::vector<const char*> outputs = {
// 		"Helo, W"
// 	};

// 	// std::vector<std::string> initial_programs = {
// 	// };

// 	OutputProgramSearch<
// 		OutputProgramIterator<DATA_SIZE, CACHE_DATA_SIZE, CACHE_SIZE>,
// 		DataCache<CACHE_DATA_SIZE, CACHE_SIZE>,
// 		RawSourceExecutor<DATA_SIZE + CACHE_DATA_SIZE*2>
// 	>
// 	search(inputs, outputs);

// 	// for (uint_fast32_t i = 0; i < initial_programs.size(); i++) {
// 	// 	std::cout << "Searching from " << initial_programs[i] << " (" << i + 1 << " / " << initial_programs.size() << ")       " << std::endl;
// 	// 	search.FindMultipleFromInitialProgram(initial_programs[i]);
// 	// }

// 	search.FindMultipleFromInitialProgramThreaded("");

// 	std::cout << "Done" << std::endl;
// }

int main()
{
	// double first = static_cast<double>(clock()) / CLOCKS_PER_SEC;
	
	search();

	// double second = static_cast<double>(clock()) / CLOCKS_PER_SEC;
	// std::cout << "Completed in " << second - first << " seconds";

	return 0;
}


// IDEAS
// If iterators[1] is skipped in all executions omit further programs in the family?

// Single digit increment

// base-2
// 18 ,>++[->++[<]>-]>-.

// base-3




// Hello, World!

// 72, 101, 108, 108, 111, 44, 32, 87, 111, 114, 108, 100, 33

// H 
// 15 ++[+[>]<-<++]>.
// optimal

// He
// 21 -[-->+++[+>]<<<-]>.>.
// likely optimal

// 22 -[-->++[+>]<<+<-]>-.>.
// 22 -[-->++[+>]<<+<-]>-.>.
// 22 -[-->+++[+>]<<<-]+>.>.
// 22 -[-->+++[+>]<<<-]->.>.
// 22 -[-->+++[+>]<<<-]>.+>.
// 22 -[-->+++[+>]<<<-]>.->.
// 22 -[-->+++[+>]<<<-]>[.>]

// 23 +[[--->]<<<-<--]>->.<-.
// 23 -[[--->]<<<-<--]>->.<-.
// 23 -[[+>]<<+++<----]>+.>+.
// 23 +[[--->]<<<-<--]>-->.<.
// 23 -[[--->]<<<-<--]>-->.<.
// 23 +[[--->]<<<-<--]>>.<--.
// 23 -[[--->]<<<-<--]>>.<--.
// 23 -[++[>]<-<-<+++]>->+.<.
// 23 -[++[>]<-<-<+++]>>+.<-.
// 23 +[>+[++<]>+++>+]<-.>>-.
// 23 -[-->+++[+>]<<<-]++>.>.
// 23 -[-->+++[+>]<<<-]-->.>.
// 23 -[-->++[+>]<<+<-]>-.+>.
// 23 -[-->++[+>]<<+<-]>-.->.
// 23 -[-->+[+>]<<++<-]>--.>.
// 23 -[+<+>>+++[>]<-<<]>.>-.
// 23 -[-->++[+>]<<+<-]+>-.>.
// 23 -[-->++[+>]<<+<-]->-.>.
// 23 -[-->+++[+>]<<<-]+>.+>.
// 23 -[-->+++[+>]<<<-]+>.->.
// 23 -[-->+++[+>]<<<-]->.+>.
// 23 -[-->+++[+>]<<<-]->.->.
// 23 +[+>+>->+[<]>+]<<<<.<+.
// 23 -[+<+>>-->[->>]<<<]<.<.
// 23 ++[[>->]<<---<<--]>>.>.
// 23 -[-->+++[+>]<<<-]>.++>.
// 23 -[-->+++[+>]<<<-]>.-->.
// 23 -[->++>+>+[<]>-]>+.>>-.
// 23 ++[>->+[+++<]>>]>.>---.
// 23 --[>>----[<<]>>+>-]>.>.
// 23 +[[++++>+[<]>+++]>-.>-]
// 23 -[[+>]<<+++<----]>[+.>]
// 23 +[>+[++<]>+++>+]<[-.>>]
// 23 -[-->++[+>]<<+<-]>-[.>]
// 23 -[-->+++[+>]<<<-]>[.+>]
// 23 -[-->+++[+>]<<<-]>[.->]
// 23 -[-->+++[+>]<<<-]+>[.>]
// 23 -[-->+++[+>]<<<-]->[.>]
// incomplete

// Interesting 23
// +[[--->]<<<-<--]>->.<-.
// -[[+>]<<+++<----]>+.>+.
// -[++[>]<-<-<+++]>->+.<.
// +[>+[++<]>+++>+]<-.>>-.
// -[-->+[+>]<<++<-]>--.>.
// -[+<+>>+++[>]<-<<]>.>-.
// +[+>+>->+[<]>+]<<<<.<+.
// -[+<+>>-->[->>]<<<]<.<.
// ++[[>->]<<---<<--]>>.>.
// -[->++>+>+[<]>-]>+.>>-.
// ++[>->+[+++<]>>]>.>---.
// --[>>----[<<]>>+>-]>.>.
// +[[++++>+[<]>+++]>-.>-]

// Hel 
// 27 +[++[<+<<+>>>>]<-<]<-<<[.>]
// No solutions <= 21 (OPS w/ MAX_JUMPS 25000)
// No solutions <= 22 (OPS w/ MAX_JUMPS 10000 MAX_FIRST_SIZE 2 MAXIMUM_OUTPUT_COUNT 5 NO_REPEATED_OUTPUT)

// Hell
// 29 +[[>]-<+<<[--<]>-]>.>>>>-.>..
// 29 +[->[>]+<<+[--<]>]>-.>>>-.>..
// 29 +[++[<+<<+>>>>]<-<]<<<.>.>-..
// 29 +[++[<+<<+>>>>]<-<]<-<<[.>]<.
// 29 +[----[->]<<<--<--]>.>--.>+..
// 29 -[----[->]<<<--<--]>.>--.>+..
// 29 -[-[>]>++>+[<]<+<-]>.>>>-.<..

// Hello
// 33 +[[>]-<+<<[--<]>-]>.>>>>-.>..+++.

// -[>--[>]<<+]>.
// +++++++..+++.
// +[>+[+<]>]>.

// 72, 101, 108, 108, 111, 44, 32, 87, 111, 114, 108, 100, 33
// 72, 101, 108, 44, 32, 87
// ^
