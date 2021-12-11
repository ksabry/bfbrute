// #include "ProgramIterator.h"
// #include "ProgramSearch.h"

// void testIter()
// {
// 	static const uint_fast32_t DATA_SIZE = 100;
// 	static const uint_fast32_t CACHE_DATA_SIZE = 20;
// 	static const uint_fast32_t CACHE_SIZE = 10;
// 	static const uint_fast32_t PROGRAM_SIZE = 14;

// 	uint_fast32_t target = 69;

// 	DataCache<CACHE_DATA_SIZE, CACHE_SIZE> cache;
// 	cache.Create();
// 	ProgramIterator<DATA_SIZE, CACHE_DATA_SIZE, CACHE_SIZE> iterator;
// 	iterator.SetCache(&cache);
// 	iterator.Start(PROGRAM_SIZE, 0, 1);
	
// 	std::cout << "Iterator started" << std::endl;
	
// 	uint_fast32_t count = 0;
// 	while (iterator.Next())
// 	{
// 		if (++count % 1000000 == 0)
// 		{
// 			std::cout << std::setw(10) << count << " " << iterator.GetProgram() << std::endl;
// 			//std::cin.ignore();
// 		}

// 		//iterator.Execute();
// 		if (!iterator.Execute("", 0)) continue;

// 		uint8_t* data = iterator.Data(); 
// 		uint_fast32_t dataIdx = iterator.DataIdx();
// 		if (data[dataIdx] == target)
// 		{
// 			std::cout << count << std::endl;
// 			std::cout << iterator.GetProgram() << std::endl;
// 			iterator.PrintData();
// 			std::cin.ignore();
// 		}
// 	}
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
	
// 	// std::vector<const char*> inputs = {
// 	// 	"A\n",
// 	// };
// 	// std::vector<const char*> outputs = {
// 	// 	"AB\nBC\nCD\nDE\nEF\nFG\nGH\nHI\nIJ\nJK\nKL\nLM\nMN\nNO\nOP\nPQ\nQR\nRS\nST\nTU\nUV\nVW\nWX\nXY\nYZ\nZA",
// 	// };

// 	std::vector<const char*> inputs = {
// 		"",
// 	};
// 	std::vector<const char*> outputs = {
// 		"A\n B\n  C\n   D\n    E\n     F",
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

int main()
{
	//uint_fast32_t count = 1;

	//double first = static_cast<double>(clock()) / CLOCKS_PER_SEC;
	//for (uint_fast32_t i = 0; i < count; i++) testIter();
	//double second = static_cast<double>(clock()) / CLOCKS_PER_SEC;

	//std::cout << "Completed " << count << " tests in " << second - first << " seconds, average " << (second - first) / count;
	
	// DataCache<20, 8> cache;
	// cache.Create();
	// cache.PrintAll();
	
	// ModDivisionTable table;
	
	// LLRR_ProgramIterator<100, 20, 8> iter;
	// iter.SetCache(&cache);
	// iter.SetDivisionTable(&table);

	// iter.Start(17, 0, 1);
	// uint_fast32_t cnt = 0;
	// while (iter.Next())
	// {
	// 	if (++cnt % 1000000 == 0)
	// 	{
	// 		std::cout << cnt << " " << iter.GetProgram() << std::endl;
	// 	}
	// 	bool result = iter.Execute("", 0);
	// }

	// double first = static_cast<double>(clock()) / CLOCKS_PER_SEC;
	
	search();

	// double second = static_cast<double>(clock()) / CLOCKS_PER_SEC;
	// std::cout << "Completed in " << second - first << " seconds";

	std::cin.ignore();
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