#pragma once

#include <assert.h>
#include <emmintrin.h>
#include "AlignedData.h"

template<typename T>
T Min(const T& left, const T& right)
{
	return left < right ? left : right;
}
template<typename T>
T Max(const T& left, const T& right)
{
	return left > right ? left : right;
}

template<uint_fast32_t data_size>
bool AddData(AlignedData<data_size>& dest, const AlignedData<data_size>& srcFirst, const AlignedData<data_size>& srcSecond)
{
	constexpr int_fast32_t data_size_s = static_cast<int_fast32_t>(data_size);
	constexpr int_fast32_t lowDataIdx = -data_size_s / 2;
	constexpr int_fast32_t highDataIdx = data_size_s - data_size_s / 2;

	int_fast32_t newDataIndex = srcFirst.idx + srcSecond.idx;
	if (newDataIndex < lowDataIdx || newDataIndex >= highDataIdx)
		return false;
	dest.idx = newDataIndex;

	int_fast32_t secondStart = Max(srcSecond.start + srcFirst.idx, lowDataIdx);
	int_fast32_t secondEnd = Min(srcSecond.end + srcFirst.idx, highDataIdx);
	dest.start = Min(srcFirst.start, secondStart);
	dest.end = Max(srcFirst.end, secondEnd);

	memset(dest.data, 0, data_size);
	if (srcFirst.end > srcFirst.start)
	{
		memcpy(
			&dest.data[srcFirst.start + data_size / 2], 
			&srcFirst.data[srcFirst.start + data_size / 2], 
			srcFirst.end - srcFirst.start
		);
	}
	
	for (int_fast32_t i = secondStart; i < secondEnd; i++)
	{
		dest.data[i + data_size / 2] += srcSecond.data[i - srcFirst.idx + data_size / 2];
	}
	
	return true;
}

template<uint_fast32_t data_size>
void PrintData(uint8_t* data)
{
	for (uint_fast32_t i = 1; i < data_size + 1; i++)
		std::cout << std::setw(4) << (int)data[i];
	std::cout << std::endl << std::string(data[0] * 4, ' ') << "  ^" << std::endl;
}
