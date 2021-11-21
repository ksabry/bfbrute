// Slow
template<uint_fast32_t data_size, uint_fast32_t max_program_size = 32>
class RawSourceExecutor
{
public:
	RawSourceExecutor(
		const char* source
	)
		: RawSourceExecutor(source, strlen(source))
	{
	}
	RawSourceExecutor(
		const char* source,
		uint_fast32_t source_size,
	)
		: source(source), source_size(source_size)
	{
		Parse();
	}

private:
	const char* source,
	uint_fast32_t source_size,

	struct { uint_fast32_t zero; uint_fast32_t nonzero; } jumps[max_program_size];
	uint_fast32_t leftBracketStack[max_program_size];

	bool Execute()
	{
		uint_fast32_t program_idx = 0;
		int_fast32_t data_idx = data_size / 2;
		uint8_t data[data_size];
		memset(data, 0, data_size);

		while (program_idx < source_size)
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

	void Parse()
	{
		int_fast32_t depth = 0
		for (int source_idx = 0; source_idx < source_size; source_idx++)
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
}
