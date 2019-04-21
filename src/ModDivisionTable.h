#pragma once

#include <cstdint>

class ModDivisionTable
{
public:
	ModDivisionTable();
	~ModDivisionTable();

	inline int Get(uint8_t d, uint8_t k)
	{
		return table[d][k];
	}

private:
	int table[256][256];
};
