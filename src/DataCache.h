#pragma once

#include <cstring>
#include <vector>
#include <unordered_set>
#include <iostream>
#include <iomanip>
#include <string>

#include "LookupData.h"
#include "AlignedData.h"
#include "Util.h"

// NOTE: for efficiency data_size should be a multiple of 4 minus 1
template<uint_fast32_t data_size, uint_fast32_t max_cache_size>
class DataCache
{
public:
	DataCache()
		: dataBalancedCount(0), dataUnbalancedCount(0)
	{
	}
	~DataCache()
	{
	}

	uint_fast32_t zeroDataIdx = data_size / 2;
	int_fast32_t lowDataIdx = -static_cast<int_fast32_t>(data_size) / 2;
	int_fast32_t highDataIdx = static_cast<int_fast32_t>(data_size) + lowDataIdx;

	uint_fast32_t dataBalancedCount;
	uint_fast32_t dataUnbalancedCount;

	std::vector<AlignedData<data_size>> dataBalanced;
	std::vector<AlignedData<data_size>> dataUnbalanced;

	std::vector<char> programsBalanced;
	std::vector<char> programsUnbalanced;

	uint_fast32_t bordersBalanced[max_cache_size + 2];
	uint_fast32_t bordersUnbalanced[max_cache_size + 2];

	void Create()
	{
		CreateZero();
		CreateOne();
		for (uint_fast32_t size = 2; size <= max_cache_size; size++)
		{
			CreateSize(size);
			std::cout << "Created cache size " << size << "\r" << std::flush;
		}
		std::cout << std::endl;
	}

private:
	std::unordered_set<LookupData<data_size>> dataSet;
	
	uint8_t currentData[data_size];
	int_fast32_t currentDataIdx;
	int_fast32_t currentDataStart;
	int_fast32_t currentDataEnd;
	char currentProgram[max_cache_size];
	uint_fast32_t currentProgramSize;
	
	void AddData()
	{
		auto lookup = LookupData<data_size>(currentDataIdx, currentData);
		if (dataSet.count(lookup) > 0)
			return;
		dataSet.insert(lookup);

		bool balanced = currentDataIdx == 0;
		std::vector<AlignedData<data_size>>& data = balanced ? dataBalanced : dataUnbalanced;
		std::vector<char>& programs = balanced ? programsBalanced : programsUnbalanced;
		uint_fast32_t& dataCount = balanced ? dataBalancedCount : dataUnbalancedCount;

		data.resize(dataCount + 1);
		programs.resize((dataCount + 1) * max_cache_size + 1);
		
		memcpy(&data[dataCount].data, currentData, data_size);
		data[dataCount].idx = currentDataIdx;
		data[dataCount].start = currentDataStart;
		data[dataCount].end = currentDataEnd;
		
		memcpy(&programs[dataCount * max_cache_size], currentProgram, currentProgramSize);
		programs[dataCount * max_cache_size + currentProgramSize] = '\0';

		dataCount++;
	}

	void AddComposite(
		AlignedData<data_size>* leftData, AlignedData<data_size>* rightData, 
		char * leftProgram, char * rightProgram, 
		unsigned leftProgramSize, unsigned rightProgramSize)
	{
		//char leftDataIndex = leftData[0] - startDataIndex;
		//char rightDataIndex = rightData[0] - startDataIndex;

		currentDataIdx = leftData->idx + rightData->idx;
		if (currentDataIdx < lowDataIdx || currentDataIdx >= highDataIdx)
			return;

		if (leftData->idx > 0)
		{
			for (uint8_t rightI = data_size - leftData->idx; rightI < data_size; rightI++)
				if (rightData->data[rightI] != 0)
					return;
			memcpy(currentData, leftData->data, data_size);
			for (uint8_t rightI = 0; rightI < data_size - leftData->idx; rightI++)
				currentData[rightI + leftData->idx] += rightData->data[rightI];
		}
		else
		{
			for (uint8_t i = 0; i < -leftData->idx; i++)
				if (rightData->data[i] != 0)
					return;
			memcpy(currentData, leftData->data, data_size);
			for (uint8_t rightI = -leftData->idx; rightI < data_size; rightI++)
				currentData[rightI + leftData->idx] += rightData->data[rightI];
		}

		memcpy(currentProgram, leftProgram, leftProgramSize);
		memcpy(currentProgram + leftProgramSize, rightProgram, rightProgramSize);
		
		currentDataStart = Min(leftData->start, rightData->start + leftData->idx);
		currentDataEnd = Max(leftData->end, rightData->end + leftData->idx);

		if (currentDataStart > 10000 || currentDataEnd > 10000)
		{
			volatile int x = 0;
		}

		AddData();
	}

	void CreateZero()
	{
		bordersBalanced[0] = dataBalancedCount;
		bordersUnbalanced[0] = dataUnbalancedCount;
		currentProgramSize = 0;
		memset(currentData, 0, data_size);

		currentDataIdx = 0;
		currentDataStart = data_size / 2;
		currentDataEnd = -static_cast<int>(data_size) / 2;
		AddData();

		bordersBalanced[1] = dataBalancedCount;
		bordersUnbalanced[1] = dataUnbalancedCount;
	}
	
	void CreateOne()
	{
		currentProgramSize = 1;
		memset(currentData, 0, data_size);
		
		currentDataIdx = 0;
		currentDataStart = 0;
		currentDataEnd = 1;
		
		currentProgram[0] = '+';
		currentData[zeroDataIdx] = 1;
		AddData();

		currentProgram[0] = '-';
		currentData[zeroDataIdx] = 255;
		AddData();

		currentData[zeroDataIdx] = 0;
		currentDataStart = data_size / 2;
		currentDataEnd = -static_cast<int>(data_size) / 2;
		
		currentProgram[0] = '>';
		currentDataIdx = 1;
		AddData();

		currentProgram[0] = '<';
		currentDataIdx = -1;
		AddData();

		bordersBalanced[2] = dataBalancedCount;
		bordersUnbalanced[2] = dataUnbalancedCount;
	}

	void CreateSize(uint_fast32_t size)
	{
		currentProgramSize = size;

		// NOTE: might be more efficient to manually call AddComposite 4 times for balanced/unbalanced pairs
		//	   likely doesn't make a difference but I am not sure

		for (uint_fast32_t leftSize = 1; leftSize < size; leftSize++)
		{
			for (int leftType = 0; leftType <= 1; leftType++)
			{
				for (int rightType = 0; rightType <= 1; rightType++)
				{
					uint_fast32_t * bordersLeft = leftType == 0 ? bordersBalanced : bordersUnbalanced;
					uint_fast32_t * bordersRight = rightType == 0 ? bordersBalanced : bordersUnbalanced;
					std::vector<AlignedData<data_size>> dataLeft = leftType == 0 ? dataBalanced : dataUnbalanced;
					std::vector<AlignedData<data_size>> dataRight = rightType == 0 ? dataBalanced : dataUnbalanced;
					std::vector<char> programsLeft = leftType == 0 ? programsBalanced : programsUnbalanced;
					std::vector<char> programsRight = rightType == 0 ? programsBalanced : programsUnbalanced;

					for (uint_fast32_t leftIndex = bordersLeft[leftSize]; leftIndex < bordersLeft[leftSize + 1]; leftIndex++)
					{
						for (uint_fast32_t rightIndex = bordersRight[size - leftSize]; rightIndex < bordersRight[size - leftSize + 1]; rightIndex++)
						{
							AddComposite(
								dataLeft.data() + leftIndex,
								dataRight.data() + rightIndex,
								programsLeft.data() + (leftIndex * max_cache_size),
								programsRight.data() + (rightIndex * max_cache_size),
								leftSize,
								size - leftSize
							);
						}
					}
				}
			}
		}

		bordersBalanced[size + 1] = dataBalancedCount;
		bordersUnbalanced[size + 1] = dataUnbalancedCount;
	}

public:
	void PrintAll()
	{
		for (uint_fast32_t e = 0; e < dataBalancedCount; e++)
		{
			std::cout << programsBalanced.data() + (e * max_cache_size) << std::endl;
			for (uint_fast32_t i = 0; i < data_size; i++)
				std::cout << std::setw(4) << (int)dataBalanced[e].data[i];
			std::cout << std::endl << std::string((dataBalanced[e].idx + data_size / 2) * 4, ' ') << "  ^" << std::endl;
			std::cout << dataBalanced[e].start << " " << dataBalanced[e].end;
			std::cin.ignore();
		}
		for (uint_fast32_t e = 0; e < dataUnbalancedCount; e++)
		{
			std::cout << programsUnbalanced.data() + (e * max_cache_size) << std::endl;
			for (uint_fast32_t i = 0; i < data_size; i++)
				std::cout << std::setw(4) << (int)dataUnbalanced[e].data[i];
			std::cout << std::endl << std::string((dataUnbalanced[e].idx + data_size / 2) * 4, ' ') << "  ^" << std::endl;
			std::cout << dataUnbalanced[e].start << " " << dataUnbalanced[e].end;
			std::cin.ignore();
		}
	}
};
