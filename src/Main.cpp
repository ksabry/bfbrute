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

// #include "ProgramIterator.h"
// // #include "LLRR_ProgramIterator.h"
// #include "ProgramSearch.h"

// void search()
// {
// 	constexpr uint_fast32_t DATA_SIZE = 400;
// 	constexpr uint_fast32_t CACHE_DATA_SIZE = 32;
// 	constexpr uint_fast32_t CACHE_SIZE = 12;
	
// 	std::vector<const char*> inputs = {
// 		""
// 	};
// 	std::vector<const char*> outputs = {
// 		"("
// 	};

// 	ProgramSearch<
// 		ProgramIterator<DATA_SIZE, CACHE_DATA_SIZE, CACHE_SIZE>, 
// 		DataCache<CACHE_DATA_SIZE, CACHE_SIZE>> 
// 	search(inputs, outputs);

// 	//search.printProgress = false;
// 	//std::string result = search.Find();
// 	//std::cout << result << " " << result.size() << std::endl;
// 	search.Find();
// }

#include "OutputProgramIterator.h"
#include "OutputProgramSearch.h"

void search()
{
	constexpr uint_fast32_t DATA_SIZE = 400;
	constexpr uint_fast32_t CACHE_DATA_SIZE = 32;
	constexpr uint_fast32_t CACHE_SIZE = 12;
	
	// backwards
	// std::vector<const char*> inputs = {
	// 	"\0000", "\0001", "\0002", "\0008", "\0009", "\00001", "\00011"
	// };
	// std::vector<uint_fast32_t> input_sizes = {
	// 	2,       2,       2,       2,       2,       3,        3
	// };
	// std::vector<const char*> outputs = {
	// 	"1", "2", "3", "9", "10", "11", "12"
	// };

	// std::vector<const char*> inputs = {
	// 	"9", "0", "1", "2", "3", "4", "5", "6", "7", "8"
	// };
	// std::vector<const char*> outputs = {
	// 	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
	// };

	std::vector<const char*> inputs = {
		"",
	};
	std::vector<const char*> outputs = {
		"(",
	};

	OutputProgramSearch<
		OutputProgramIterator<DATA_SIZE, CACHE_DATA_SIZE, CACHE_SIZE>,
		DataCache<CACHE_DATA_SIZE, CACHE_SIZE>>
	search(inputs, outputs);

	// investigate why -[>+<-----]>--. wasn't found
	// +[[---->]<<-]
	// -[>+<-----]>--

	//search.printProgress = false;
	//std::string result = search.Find();
	//std::cout << result << " " << result.size() << std::endl;
	search.Find();
}

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