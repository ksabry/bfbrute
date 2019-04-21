#include "ModDivisionTable.h"

ModDivisionTable::ModDivisionTable()
{
	for (uint_fast32_t d = 0; d < 256; d++)
	{
		for (uint_fast32_t k = 0; k < 256; k++)
		{
			table[d][k] = -1;
		}
	}

	for (int k = 1; k < 256; k++)
	{
		table[0][k] = 0;
		
		uint8_t mult = k;
		int cnt = 1;
		while (mult)
		{
			table[mult][256 - k] = cnt;
			cnt++;
			mult += k;
		}
	}
}

ModDivisionTable::~ModDivisionTable()
{
}
