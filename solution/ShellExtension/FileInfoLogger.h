#pragma once
#include "ThreadPool.h"
#include <string>
#include <stdint.h>
#include <fstream>

class FileInfoLogger
{
public:
	FileInfoLogger(void);
	FileInfoLogger(wchar_t **fNames, size_t fNumber, const wchar_t *logFName);
	FileInfoLogger(std::vector<std::wstring> fNames, std::wstring logFName);
	FileInfoLogger(const FileInfoLogger &a);

	~FileInfoLogger(void);

	FileInfoLogger& operator=(const FileInfoLogger &a);

	void setFileNames(wchar_t **fNames, size_t fNumber);
	void setFileNames(std::vector<std::wstring> &fNames);
	void setLogFileName(const wchar_t *logFName);
	void setLogFileName(const std::wstring &logFName);

	std::vector<std::wstring> getFileNames() const;
	std::wstring getLogFileName() const;
	uint32_t getPolynomial() const;

	void logInfo();

private:
	std::mutex syncObj;

	std::vector<std::wstring> fileNames;
	std::wstring logFileName;

	const uint32_t polynomial;
	uint32_t crc32Lookup[8][256];

	std::wfstream logFile;

	ThreadPool pool;

	void calculateCrc32Table();
	uint32_t getFileHash(const std::wstring &fName);
	uint32_t crc32(const void* data, size_t length, uint32_t previousCrc32 = 0);
	double fileSizeToHRF(uint64_t fs, wchar_t* f);
};

