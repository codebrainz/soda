#include "SourceFile.h"
#include <stdexcept>
#include <iostream>
#include <cassert>
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

#define TAB_WIDTH 8u

namespace SODA
{

	struct SOURCE_FILE::PRIVATE
	{
		COMPILER &comp;
		std::string fn;
		DWORD dwSize;
		HANDLE hFile;
		HANDLE hMapFile;
		void *mapData;

		PRIVATE(COMPILER &comp, const std::string &fn)
			: comp(comp), fn(fn)
		{
			hFile = CreateFile(fn.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile == INVALID_HANDLE_VALUE)
				throw std::runtime_error("failed to open file '" + fn + "' (error code " + std::to_string(GetLastError()) + ")");
			dwSize = GetFileSize(hFile, nullptr);
			if (dwSize == INVALID_FILE_SIZE)
				throw std::runtime_error("invalid file size of file '" + fn + "' found (error code " + std::to_string(GetLastError()) + ")");
			else if (dwSize == 0)
				throw std::runtime_error("cannot open file '" + fn + "' of zero bytes");
			hMapFile = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, dwSize, nullptr);
			if (hMapFile == NULL)
				throw std::runtime_error("failed to memory map file '" + fn + "' (error code " + std::to_string(GetLastError()) + ")");
			mapData = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
			if (mapData == NULL)
				throw std::runtime_error("failed to create view of memory mapped file '" + fn + "' (error code " + std::to_string(GetLastError()) + ")");
		}

		~PRIVATE()
		{
			UnmapViewOfFile(mapData);
			CloseHandle(hMapFile);
			CloseHandle(hFile);
		}
	};

	SOURCE_FILE::SOURCE_FILE(COMPILER &compiler, const std::string & fn)
		: m_Impl(new PRIVATE(compiler, fn))
	{
	}

	SOURCE_FILE::~SOURCE_FILE()
	{
		delete m_Impl;
	}		
	
	COMPILER &SOURCE_FILE::GetCompiler()
	{
		return m_Impl->comp;
	}

	const COMPILER &SOURCE_FILE::GetCompiler() const
	{
		return m_Impl->comp;
	}

	const std::string &SOURCE_FILE::GetFileName() const
	{
		return m_Impl->fn;
	}

	size_t SOURCE_FILE::GetSize() const
	{
		return m_Impl->dwSize;
	}

	void SOURCE_FILE::GetPosition(size_t inputOffset, size_t &outputLine, size_t &outputColumn) const
	{
		assert(inputOffset < m_Impl->dwSize);
		outputLine = 1;
		outputColumn = 0;
		for (auto i = 0u; i < inputOffset; i++)
		{
			switch (static_cast<char>(operator[](i)))
			{
			case '\n':
				outputLine++;
				outputColumn = 0;
				break;
			case '\r':
				outputColumn = 0;
				break;
			case '\t':
				outputColumn += TAB_WIDTH;
				break;
			default:
				outputColumn++;
				break;
			}
		}
	}

	uint8_t &SOURCE_FILE::operator[](size_t off)
	{
		assert(off < m_Impl->dwSize);
		return static_cast<uint8_t*>(m_Impl->mapData)[off];
	}

	const uint8_t &SOURCE_FILE::operator[](size_t off) const
	{
		assert(off < m_Impl->dwSize);
		return static_cast<const uint8_t*>(m_Impl->mapData)[off];
	}

} // namespace Soda