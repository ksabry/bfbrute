#pragma once

template<uint_fast32_t data_size>
struct AlignedData
{
	/*alignas(16)*/ uint8_t data[data_size];
	int_fast32_t idx;
	int_fast32_t start;
	int_fast32_t end;
};
