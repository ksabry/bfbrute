#pragma once

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

// Slow
template<uint_fast32_t data_size, uint_fast32_t max_program_size = 32>
class RawSourceExecutor
{
public:
	RawSourceExecutor(std::string source)
		: source(source)
	{
		Parse();
	}

	std::string source;

	struct { uint_fast32_t zero; uint_fast32_t nonzero; } jumps[max_program_size];
	uint_fast32_t leftBracketStack[max_program_size];

	uint8_t data[data_size];
	int_fast32_t data_idx = data_size / 2;

	bool Execute()
	{
		uint_fast32_t program_idx = 0;
		data_idx = data_size / 2;
		memset(data, 0, data_size);

		while (program_idx < source.length())
		{
			switch (source[program_idx])
			{
				case '+':
					data[data_idx]++;
					program_idx++;
					break;
				case '-':
					data[data_idx]--;
					program_idx++;
					break;
				case '>':
					data_idx++;
					program_idx++;
					break;
				case '<':
					data_idx--;
					program_idx++;
					break;
				case '[':
				case ']':
					program_idx = data[data_idx] == 0 ? jumps[program_idx].zero : jumps[program_idx].nonzero;
					break;
			}
			if (data_idx < 0 || data_idx >= data_size)
			{
				return false;
			}
		}
		return true;
	}

	uint8_t* GetData()
	{
		return data;
	}

	uint_fast32_t GetDataIdx()
	{
		return static_cast<uint_fast32_t>(data_idx);
	}

	void Parse()
	{
		int_fast32_t depth = 0;
		for (int source_idx = 0; source_idx < source.length(); source_idx++)
		{
			switch (source[source_idx])
			{
				case '[':
					leftBracketStack[depth] = source_idx;
					depth++;
					break;
				case ']':
					uint_fast32_t lbracket = leftBracketStack[depth-1];
					jumps[source_idx].zero = jumps[lbracket].zero = source_idx + 1;
					jumps[source_idx].nonzero = jumps[lbracket].nonzero = lbracket + 1;
					depth--;
					break;
			}
		}
	}

	void PrintData()
	{
		uint_fast32_t data_bound_low = 0;
		uint_fast32_t data_bound_high = data_size - 1;
		while (data_bound_low < data_size) {
			if (data[data_bound_low] != 0)
			{
				break;
			}
			data_bound_low++;
		}
		while (data_bound_high > data_bound_low)
		{
			if (data[data_bound_high] != 0)
			{
				break;
			}
			data_bound_high--;
		}

		for (uint_fast32_t i = data_bound_low; i <= data_bound_high; i++)
		{
			std::cout << std::setw(4) << (int)data[i];
		}
		std::cout << std::endl << std::string((data_idx - data_bound_low) * 4, ' ') << "  ^" << std::endl;
	}
};
