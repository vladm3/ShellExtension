#include "FileInfoLogger.h"
#include <codecvt>
#include <Windows.h>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <clocale>

extern HWND hDlgWnd;

FileInfoLogger::FileInfoLogger(void):
	pool(8),
	polynomial(0xEDB88320)
{
	this->calculateCrc32Table();
}

FileInfoLogger::FileInfoLogger(wchar_t** fNames, size_t fNumber, const wchar_t* logFName):
	pool(8),
	polynomial(0xEDB88320)
{
	this->calculateCrc32Table();
	this->setLogFileName(logFName);
	this->setFileNames(fNames, fNumber);
}

FileInfoLogger::FileInfoLogger(const FileInfoLogger &a):
	polynomial(0xEDB88320),
	pool(8)
{
	this->calculateCrc32Table();
	this->logFileName = a.logFileName;
	this->fileNames = a.fileNames;
}

FileInfoLogger::FileInfoLogger(std::vector<std::wstring> fNames, std::wstring logFName):
	pool(8),
	polynomial(0xEDB88320)
{
	std::setlocale(LC_ALL, "ru_RU.UTF-8");
	this->calculateCrc32Table();
	this->setLogFileName(logFName);
	this->setFileNames(fNames);
}

FileInfoLogger::~FileInfoLogger(void)
{
}

FileInfoLogger& FileInfoLogger::operator=(const FileInfoLogger &a)
{
	this->fileNames = a.fileNames;
	this->logFileName = a.logFileName;

	return *this;
}

void FileInfoLogger::logInfo()
{
	logFile.open(this->logFileName, std::wfstream::in | std::wfstream::out);
	std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
	logFile.imbue(loc);

	logFile.seekg(0, logFile.end);
	long long length = logFile.tellg();
	logFile.seekg(0, logFile.beg);
	logFile.seekp(length, logFile.beg);

	BY_HANDLE_FILE_INFORMATION fileInfo;
	SYSTEMTIME stUTC, stLocal;
	wchar_t stringTime[25];

	for (size_t i = 0; i < fileNames.size(); i++)
	{
		HANDLE hFile = CreateFile(fileNames[i].c_str(), 
			GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if(hFile == INVALID_HANDLE_VALUE)
		{
			logFile << L"[E] Error while opening '" << fileNames[i] << L"'\n";
			continue;
		}

		if(GetFileInformationByHandle(hFile, &fileInfo))
		{
			logFile << fileNames[i] << L"  ";

			FileTimeToSystemTime(&(fileInfo.ftCreationTime), &stUTC);
			SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
			swprintf(stringTime, 25, L"%02d/%02d/%d %02d:%02d", 
				stLocal.wDay, stLocal.wMonth, stLocal.wYear, stLocal.wHour, stLocal.wMinute);

			uint64_t fSize = fileInfo.nFileSizeHigh;
			fSize *= 0x100000000ui64;
			fSize += fileInfo.nFileSizeLow;

			wchar_t f = L'';
			double hrfSize = fileSizeToHRF(fSize, &f);
			logFile << stringTime << L"  " << std::setprecision(3) << hrfSize;
			if(f)
			{
				logFile << f;
			}
			logFile << L"B  0x........\n";
		}
		else
		{
			logFile << L"[E] Error while fetching information from '" << fileNames[i] << L"'\n";
		}

		CloseHandle(hFile);
	}
	logFile.close();

	size_t len = fileNames.size();

	pool.setCallbackFunction(
		[len]()
		{
			PostMessage(hDlgWnd, WM_COMMAND, 9, len);
		}
	);

	for(size_t i = 0; i < fileNames.size(); i++)
	{
		std::wstring fName = fileNames[i];
		pool.enqueue<void>(
			[fName, i, this, length]
			{
				uint32_t hash = this->getFileHash(fName);
				syncObj.lock();
				logFile.open(this->logFileName, std::wfstream::in | std::wfstream::out);
				logFile.seekg(length);
				for (int j = 0; j <= i; j++)
				{
					logFile.ignore(0x7fffffffffffffff, L'\n');
				}
				long long l = logFile.tellg();
				l -= 10;
				logFile.seekp(l, logFile.beg);

				logFile << std::hex << std::setw(8) << std::setfill(L'0') << hash;
				logFile.close();
				syncObj.unlock();

				PostMessage(hDlgWnd, WM_COMMAND, 10, fileNames.size());
			}
		);
	}

	//PostMessage(hDlgWnd, WM_COMMAND, 11, 0);
}

uint32_t FileInfoLogger::getFileHash(const std::wstring &fName)
{
	std::ifstream file(fName, std::ifstream::binary);
	const size_t buf_size = 4096 * 4096 * 32;
	uint64_t length = 0;
	char *buf = new char[buf_size];
	uint32_t crc = 0;
	size_t cur = 0;

	file.seekg(0, file.end);
	length = file.tellg();
	file.seekg(0, file.beg);

	while(length)
	{
		cur = length > buf_size ? buf_size : length;
		length -= cur;
		file.read(buf, cur);
		crc = this->crc32((void*)buf, cur, crc);
	}

	file.close();
	delete [] buf;

	return crc;
}

uint32_t FileInfoLogger::crc32(const void* data, size_t length, uint32_t previousCrc32)
{
	uint32_t* current = (uint32_t*) data;
	uint32_t crc = ~previousCrc32;

	while(length >= 8)
	{
		uint32_t one = *current++ ^ crc;
		uint32_t two = *current++;

		crc = crc32Lookup[7][one & 0xFF] ^ 
			crc32Lookup[6][(one >> 8) & 0xFF] ^ 
			crc32Lookup[5][(one >> 16) & 0xFF] ^ 
			crc32Lookup[4][(one >> 24) & 0xFF] ^ 
			crc32Lookup[3][two & 0xFF] ^ 
			crc32Lookup[2][(two >> 8) & 0xFF] ^ 
			crc32Lookup[1][(two >> 16) & 0xFF] ^ 
			crc32Lookup[0][(two >> 24) & 0xFF];

		length -= 8;
	}

	unsigned char* currentChar = (unsigned char*)current;

	while(length--)
	{
		crc = (crc >> 8) ^ crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];
	}

	return ~crc;
}

void FileInfoLogger::calculateCrc32Table()
{
	for(unsigned int i = 0; i <= 0xFF; i++)
	{
		uint32_t crc = i;
		for(unsigned int j = 0; j < 8; j++)
			crc = (crc >> 1) ^ ((crc & 1) * polynomial);
		crc32Lookup[0][i] = crc;
	}

	for(unsigned int i = 0; i <= 0xFF; i++)
	{
		//Slicing-by-4
		crc32Lookup[1][i] = (crc32Lookup[0][i] >> 8) ^ crc32Lookup[0][crc32Lookup[0][i] & 0xFF];
		crc32Lookup[2][i] = (crc32Lookup[1][i] >> 8) ^ crc32Lookup[0][crc32Lookup[1][i] & 0xFF];
		crc32Lookup[3][i] = (crc32Lookup[2][i] >> 8) ^ crc32Lookup[0][crc32Lookup[2][i] & 0xFF];

		//Slicing-by-8
		crc32Lookup[4][i] = (crc32Lookup[3][i] >> 8) ^ crc32Lookup[0][crc32Lookup[3][i] & 0xFF];
		crc32Lookup[5][i] = (crc32Lookup[4][i] >> 8) ^ crc32Lookup[0][crc32Lookup[4][i] & 0xFF];
		crc32Lookup[6][i] = (crc32Lookup[5][i] >> 8) ^ crc32Lookup[0][crc32Lookup[5][i] & 0xFF];
		crc32Lookup[7][i] = (crc32Lookup[6][i] >> 8) ^ crc32Lookup[0][crc32Lookup[6][i] & 0xFF];
	}
}

double FileInfoLogger::fileSizeToHRF(uint64_t fs, wchar_t* f)
{
	if(fs > 0x100000000ui64 / 4)
	{
		*f = L'G';
		return fs / 1.073741824e9;
	}
	if(fs > 1048576ui64)
	{
		*f = L'M';
		return fs / 1.048576e6;
	}
	if(fs > 1024ui64)
	{
		*f = 'K';
		return fs / 1.024e3;
	}
	*f = L'';
	return fs * 1.0;
}

void FileInfoLogger::setFileNames(wchar_t** fNames, size_t fNumber)
{
	this->fileNames.clear();

	for(unsigned int i = 0; i < fNumber; i++)
	{
		DWORD attr = GetFileAttributes(fNames[i]);
		if(attr != 0xFFFFFFFF)
		{
			if((attr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
			{
				std::wstring tmp(fNames[i]);
				fileNames.push_back(tmp);
			}
		}
		else
		{
			std::wofstream errLog(this->getLogFileName(), std::wofstream::app);
			errLog << L"[E] Error! File '" << fNames[i] << L"' doesn't exist!" << L"\n";
			errLog.close();
		}
	}

	std::sort(this->fileNames.begin(), this->fileNames.end(), std::locale(""));
}

void FileInfoLogger::setFileNames(std::vector<std::wstring> &fNames)
{
	fileNames.clear();

	for(unsigned int i = 0; i < fNames.size(); i++)
	{
		DWORD attr = GetFileAttributes(fNames[i].c_str());
		if(attr != 0xFFFFFFFF)
		{
			if((attr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
			{
				fileNames.push_back(fNames[i]);
			}
		}
		else
		{
			std::wofstream errLog(this->getLogFileName(), std::wofstream::app);
			errLog << L"[E] Error! File '" << fNames[i] << L"' doesn't exist!" << L"\n";
			errLog.close();
		}
	}

	std::sort(this->fileNames.begin(), this->fileNames.end(), std::locale(""));
}

void FileInfoLogger::setLogFileName(const wchar_t* logFName)
{
	logFileName = std::wstring(logFName);
	std::wofstream wof(logFileName, std::wofstream::app);
	wof.close();
}

void FileInfoLogger::setLogFileName(const std::wstring &logFName)
{
	logFileName = logFName;
	std::wofstream wof(logFileName, std::wofstream::app);
	wof.close();
}

std::vector<std::wstring> FileInfoLogger::getFileNames() const
{
	return this->fileNames;
}

std::wstring FileInfoLogger::getLogFileName() const
{
	return this->logFileName;
}

uint32_t FileInfoLogger::getPolynomial() const
{
	return this->polynomial;
}
