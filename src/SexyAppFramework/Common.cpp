#include "Common.h"
#include "misc/MTRand.h"
#include "misc/Debug.h"
#include <locale>
#include <codecvt>
#include <errno.h>
#include <filesystem>
#include <chrono>
#include <stdarg.h>

#ifdef __SWITCH__
#include <switch.h>
#elif defined(__3DS__)
#include <3ds.h>
#endif

#include "misc/PerfTimer.h"

//HINSTANCE Sexy::gHInstance;
bool Sexy::gDebug = false;
static Sexy::MTRand gMTRand;
namespace Sexy
{
	std::filesystem::path gAppDataFolder;
	std::filesystem::path gResourceFolder;
}

void Sexy::PrintF(const char *text, ...)
{
	char str[1024];

	va_list args;
	va_start(args, text);
	vsnprintf(str, sizeof(str), text, args);
	va_end(args);

#if defined(__SWITCH__) || defined(__3DS__)
	svcOutputDebugString(str, sizeof(str));
#endif

	fprintf(stdout, "%s", str);
}

int Sexy::Rand()
{
	return gMTRand.Next();
}

int Sexy::Rand(int range)
{
	return gMTRand.Next((unsigned long)range);
}

float Sexy::Rand(float range)
{
	return gMTRand.Next(range);
}

void Sexy::SRand(ulong theSeed)
{
	gMTRand.SRand(theSeed);
}

std::string Sexy::GetAppDataFolder()
{
	return Sexy::gAppDataFolder.string();
}

void Sexy::SetAppDataFolder(const std::string& thePath)
{
	Sexy::gAppDataFolder = thePath;
}

std::string Sexy::GetAppDataPath(const std::string& theRelativePath)
{
	return (Sexy::gAppDataFolder / theRelativePath).string();
}

std::string Sexy::GetResourceFolder()
{
	return Sexy::gResourceFolder.string();
}

void Sexy::SetResourceFolder(const std::string& thePath)
{
	Sexy::gResourceFolder = thePath;
}

std::string Sexy::GetResourcePath(const std::string& theRelativePath)
{
	return (Sexy::gResourceFolder / theRelativePath).string();
}


std::string Sexy::URLEncode(const std::string& theString)
{
	char* aHexChars = (char*)"0123456789ABCDEF";

	std::string aString;

	for (unsigned i = 0; i < theString.length(); i++)
	{
		switch (theString[i])
		{
		case ' ':
			aString.insert(aString.end(), '+');
			break;
		case '?':
		case '&':
		case '%':
		case '+':
		case '\r':
		case '\n':
		case '\t':
			aString.insert(aString.end(), '%');
			aString.insert(aString.end(), aHexChars[(theString[i] >> 4) & 0xF]);
			aString.insert(aString.end(), aHexChars[(theString[i]     ) & 0xF]);
			break;
		default:
			aString.insert(aString.end(), theString[i]);
		}
	}

	return aString;
}

std::string Sexy::StringToUpper(const std::string& theString)
{
	std::string aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += toupper(theString[i]);

	return aString;
}

std::wstring Sexy::StringToUpper(const std::wstring& theString)
{
	std::wstring aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += towupper(theString[i]);

	return aString;
}

std::string Sexy::StringToLower(const std::string& theString)
{
	std::string aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += tolower(theString[i]);

	return aString;
}

std::wstring Sexy::StringToLower(const std::wstring& theString)
{
	std::wstring aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += tolower(theString[i]);

	return aString;
}

std::wstring Sexy::StringToWString(const std::string &theString)
{
	try
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> aConv;
		return aConv.from_bytes(theString);
	}
	catch (const std::range_error&)
	{
		std::wstring aFallback;
		aFallback.reserve(theString.length());
		for (size_t i = 0; i < theString.length(); i++)
			aFallback += (unsigned char)theString[i];
		return aFallback;
	}
}

std::string Sexy::WStringToString(const std::wstring &theString)
{
	try
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> aConv;
		return aConv.to_bytes(theString);
	}
	catch (const std::range_error&)
	{
		std::string aFallback;
		aFallback.reserve(theString.length());
		for (auto ch : theString)
			aFallback += (ch <= 0xFF) ? (char)ch : '?';
		return aFallback;
	}
}

SexyString Sexy::StringToSexyString(const std::string& theString)
{
#ifdef _USE_WIDE_STRING
	return StringToWString(theString);
#else
	return SexyString(theString);
#endif
}

SexyString Sexy::WStringToSexyString(const std::wstring &theString)
{
#ifdef _USE_WIDE_STRING
	return SexyString(theString);
#else
	return WStringToString(theString);
#endif
}

std::string Sexy::SexyStringToString(const SexyString& theString)
{
#ifdef _USE_WIDE_STRING
	return WStringToString(theString);
#else
	return std::string(theString);
#endif
}

std::wstring Sexy::SexyStringToWString(const SexyString& theString)
{
#ifdef _USE_WIDE_STRING
	return std::wstring(theString);
#else
	return StringToWString(theString);
#endif
}

std::string Sexy::Trim(const std::string& theString)
{
	int aStartPos = 0;
	while ( aStartPos < (int) theString.length() && isspace(theString[aStartPos]) )
		aStartPos++;

	int anEndPos = theString.length() - 1;
	while ( anEndPos >= 0 && isspace(theString[anEndPos]) )
		anEndPos--;

	return theString.substr(aStartPos, anEndPos - aStartPos + 1);
}

std::wstring Sexy::Trim(const std::wstring& theString)
{   //0x5AFD80
	int aStartPos = 0;
	while ( aStartPos < (int) theString.length() && iswspace(theString[aStartPos]) )
		aStartPos++;

	int anEndPos = theString.length() - 1;
	while ( anEndPos >= 0 && iswspace(theString[anEndPos]) )
		anEndPos--;

	return theString.substr(aStartPos, anEndPos - aStartPos + 1);
}

bool Sexy::StringToInt(const std::string theString, int* theIntVal)
{
	*theIntVal = 0;

	if (theString.length() == 0)
		return false;

	int theRadix = 10;
	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		char aChar = theString[i];
		
		if ((theRadix == 10) && (aChar >= '0') && (aChar <= '9'))
			*theIntVal = (*theIntVal * 10) + (aChar - '0');
		else if ((theRadix == 0x10) && 
			(((aChar >= '0') && (aChar <= '9')) || 
			 ((aChar >= 'A') && (aChar <= 'F')) || 
			 ((aChar >= 'a') && (aChar <= 'f'))))
		{			
			if (aChar <= '9')
				*theIntVal = (*theIntVal * 0x10) + (aChar - '0');
			else if (aChar <= 'F')
				*theIntVal = (*theIntVal * 0x10) + (aChar - 'A') + 0x0A;
			else
				*theIntVal = (*theIntVal * 0x10) + (aChar - 'a') + 0x0A;
		}
		else if (((aChar == 'x') || (aChar == 'X')) && (i == 1) && (*theIntVal == 0))
		{
			theRadix = 0x10;
		}
		else
		{
			*theIntVal = 0;
			return false;
		}
	}

	if (isNeg)
		*theIntVal = -*theIntVal;

	return true;
}

bool Sexy::StringToInt(const std::wstring theString, int* theIntVal)
{
	*theIntVal = 0;

	if (theString.length() == 0)
		return false;

	int theRadix = 10;
	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		wchar_t aChar = theString[i];
		
		if ((theRadix == 10) && (aChar >= L'0') && (aChar <= L'9'))
			*theIntVal = (*theIntVal * 10) + (aChar - L'0');
		else if ((theRadix == 0x10) && 
			(((aChar >= L'0') && (aChar <= L'9')) || 
			 ((aChar >= L'A') && (aChar <= L'F')) || 
			 ((aChar >= L'a') && (aChar <= L'f'))))
		{			
			if (aChar <= L'9')
				*theIntVal = (*theIntVal * 0x10) + (aChar - L'0');
			else if (aChar <= L'F')
				*theIntVal = (*theIntVal * 0x10) + (aChar - L'A') + 0x0A;
			else
				*theIntVal = (*theIntVal * 0x10) + (aChar - L'a') + 0x0A;
		}
		else if (((aChar == L'x') || (aChar == L'X')) && (i == 1) && (*theIntVal == 0))
		{
			theRadix = 0x10;
		}
		else
		{
			*theIntVal = 0;
			return false;
		}
	}

	if (isNeg)
		*theIntVal = -*theIntVal;

	return true;
}

bool Sexy::StringToDouble(const std::string theString, double* theDoubleVal)
{
	*theDoubleVal = 0.0;

	if (theString.length() == 0)
		return false;

	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		char aChar = theString[i];

		if ((aChar >= '0') && (aChar <= '9'))
			*theDoubleVal = (*theDoubleVal * 10) + (aChar - '0');
		else if (aChar == '.')
		{
			i++;
			break;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	double aMult = 0.1;
	for (; i < theString.length(); i++)
	{
		char aChar = theString[i];

		if ((aChar >= '0') && (aChar <= '9'))
		{
			*theDoubleVal += (aChar - '0') * aMult;	
			aMult /= 10.0;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	if (isNeg)
		*theDoubleVal = -*theDoubleVal;

	return true;
}

bool Sexy::StringToDouble(const std::wstring theString, double* theDoubleVal)
{
	*theDoubleVal = 0.0;

	if (theString.length() == 0)
		return false;

	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		wchar_t aChar = theString[i];

		if ((aChar >= L'0') && (aChar <= L'9'))
			*theDoubleVal = (*theDoubleVal * 10) + (aChar - L'0');
		else if (aChar == L'.')
		{
			i++;
			break;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	double aMult = 0.1;
	for (; i < theString.length(); i++)
	{
		wchar_t aChar = theString[i];

		if ((aChar >= L'0') && (aChar <= L'9'))
		{
			*theDoubleVal += (aChar - L'0') * aMult;	
			aMult /= 10.0;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	if (isNeg)
		*theDoubleVal = -*theDoubleVal;

	return true;
}

// TODO: Use <locale> for localization of number output?
SexyString Sexy::CommaSeperate(int theValue)
{	
	if (theValue == 0)
		return __S("0");

	SexyString aCurString;

	int aPlace = 0;
	int aCurValue = theValue;

	while (aCurValue > 0)
	{
		if ((aPlace != 0) && (aPlace % 3 == 0))
			aCurString = __S(',') + aCurString;
		aCurString = (SexyChar) (__S('0') + (aCurValue % 10)) + aCurString;
		aCurValue /= 10;
		aPlace++;
	}

	return aCurString;
}

std::string Sexy::GetCurDir()
{
	std::error_code ec;
	std::filesystem::path cur = std::filesystem::current_path(ec);
	if (ec)
		return std::string();
	return cur.string();
}

std::string Sexy::GetFullPath(const std::string& theRelPath)
{
	return GetPathFrom(theRelPath, GetCurDir());
}

std::string Sexy::GetPathFrom(const std::string& theRelPath, const std::string& theDir)
{
	std::filesystem::path relPath(theRelPath);
	if (relPath.is_absolute() || relPath.has_root_name())
		return relPath.lexically_normal().string();

	std::filesystem::path baseDir;
	if (!theDir.empty())
		baseDir = std::filesystem::path(theDir);
	else
	{
		std::error_code ec;
		baseDir = std::filesystem::current_path(ec);
		if (ec)
			return relPath.lexically_normal().string();
	}

	return (baseDir / relPath).lexically_normal().string();
}

bool Sexy::AllowAllAccess(const std::string& theFileName)
{
	return false;
}

bool Sexy::Deltree(const std::string& thePath)
{
	std::error_code ec;
	std::filesystem::path path(thePath);
	if (!std::filesystem::exists(path, ec))
		return false;

	std::filesystem::remove_all(path, ec);
	return !ec;
}

bool Sexy::FileExists(const std::string& theFileName)
{
	std::error_code ec;
	return std::filesystem::exists(theFileName, ec);
}

void Sexy::MkDir(const std::string& theDir)
{
	std::error_code ec;
	std::filesystem::create_directories(theDir, ec);
}

std::string Sexy::GetFileName(const std::string& thePath, bool noExtension)
{
	if (!thePath.empty())
	{
		char lastChar = thePath[thePath.length() - 1];
		if (lastChar == '\\' || lastChar == '/')
			return "";
	}

	std::filesystem::path path(thePath);
	if (noExtension)
		return path.stem().string();

	return path.filename().string();
}

std::string Sexy::GetFileDir(const std::string& thePath, bool withSlash)
{
	std::filesystem::path path(thePath);
	std::filesystem::path parent = path.parent_path();
	if (parent.empty())
		return "";

	std::string result = parent.string();
	if (withSlash)
	{
		char lastChar = result[result.length() - 1];
		if (lastChar != '/' && lastChar != '\\')
			result += std::filesystem::path::preferred_separator;
	}

	return result;
}

std::string Sexy::RemoveTrailingSlash(const std::string& theDirectory)
{
	int aLen = theDirectory.length();
	
	if ((aLen > 0) && ((theDirectory[aLen-1] == '\\') || (theDirectory[aLen-1] == '/')))
		return theDirectory.substr(0, aLen - 1);
	else
		return theDirectory;
}

std::string	Sexy::AddTrailingSlash(const std::string& theDirectory, bool backSlash)
{
	if (!theDirectory.empty())
	{
		char aChar = theDirectory[theDirectory.length()-1];
		if (aChar!='\\' && aChar!='/')
			return theDirectory + (backSlash ? '\\' : '/');
		else
			return theDirectory;
	}
	else
		return "";
}


time_t Sexy::GetFileDate(const std::string& theFileName)
{
	std::error_code ec;
	auto ftime = std::filesystem::last_write_time(theFileName, ec);
	if (ec)
		return 0;

	auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
	return std::chrono::system_clock::to_time_t(sctp);
}

std::string Sexy::vformat(const char* fmt, va_list argPtr) 
{
    // We draw the line at a 1MB string.
    const int maxSize = 1000000;

    // If the string is less than 161 characters,
    // allocate it on the stack because this saves
    // the malloc/free time.
    const int bufSize = 161;
	char stackBuffer[bufSize];

    int attemptedSize = bufSize - 1;

	int numChars = 0;
#ifdef _WIN32
	numChars = _vsnprintf(stackBuffer, attemptedSize, fmt, argPtr);
#else
	numChars = vsnprintf(stackBuffer, attemptedSize, fmt, argPtr);
#endif

	//cout << "NumChars: " << numChars << endl;

    if ((numChars >= 0) && (numChars <= attemptedSize)) 
	{
		// Needed for case of 160-character printf thing
		stackBuffer[numChars] = '\0';

        // Got it on the first try.
		return std::string(stackBuffer);
    }

    // Now use the heap.
    char* heapBuffer = nullptr;

    while (((numChars == -1) || (numChars > attemptedSize)) && 
		(attemptedSize < maxSize)) 
	{
        // Try a bigger size
        attemptedSize *= 2;
		heapBuffer = (char*)realloc(heapBuffer, (attemptedSize + 1));
#ifdef _WIN32
		numChars = _vsnprintf(heapBuffer, attemptedSize, fmt, argPtr);
#else
		numChars = vsnprintf(heapBuffer, attemptedSize, fmt, argPtr);
#endif
    }

	heapBuffer[numChars] = 0;

	std::string result = std::string(heapBuffer);

    free(heapBuffer);
    return result;
}

//overloaded StrFormat: should only be used by the xml strings
std::string Sexy::StrFormat(const char* fmt ...) 
{
    va_list argList;
    va_start(argList, fmt);
	std::string result = vformat(fmt, argList);
    va_end(argList);

    return result;
}

std::wstring Sexy::vformat(const wchar_t* fmt, va_list argPtr) 
{
    // We draw the line at a 1MB string.
    const int maxSize = 1000000;

    // If the string is less than 161 characters,
    // allocate it on the stack because this saves
    // the malloc/free time.
    const int bufSize = 161;
	wchar_t stackBuffer[bufSize];

    int attemptedSize = bufSize - 1;

	int numChars = 0;
#ifdef _WIN32
	numChars = _vsnwprintf(stackBuffer, attemptedSize, fmt, argPtr);
#else
	numChars = vswprintf(stackBuffer, attemptedSize, fmt, argPtr);
#endif

	//cout << "NumChars: " << numChars << endl;

    if ((numChars >= 0) && (numChars <= attemptedSize)) 
	{
		// Needed for case of 160-character printf thing
		stackBuffer[numChars] = '\0';

        // Got it on the first try.
		return std::wstring(stackBuffer);
    }

    // Now use the heap.
	wchar_t* heapBuffer = nullptr;

    while (((numChars == -1) || (numChars > attemptedSize)) && 
		(attemptedSize < maxSize)) 
	{
        // Try a bigger size
        attemptedSize *= 2;
		heapBuffer = (wchar_t*)realloc(heapBuffer, (attemptedSize + 1));
#ifdef _WIN32
		numChars = _vsnwprintf(heapBuffer, attemptedSize, fmt, argPtr);
#else
		numChars = vswprintf(heapBuffer, attemptedSize, fmt, argPtr);
#endif
    }

	heapBuffer[numChars] = 0;

	std::wstring result = std::wstring(heapBuffer);

    free(heapBuffer);

    return result;
}

//overloaded StrFormat: should only be used by the xml strings
std::wstring Sexy::StrFormat(const wchar_t* fmt ...)
{
    va_list argList;
    va_start(argList, fmt);
	std::wstring result = vformat(fmt, argList);
    va_end(argList);

    return result;
}

std::string Sexy::Evaluate(const std::string& theString, const DefinesMap& theDefinesMap)
{
	std::string anEvaluatedString = theString;

	for (;;)
	{
		size_t aPercentPos = anEvaluatedString.find('%');

		if (aPercentPos == std::string::npos)
			break;
		
		size_t aSecondPercentPos = anEvaluatedString.find('%', aPercentPos + 1);
		if (aSecondPercentPos == std::string::npos)
			break;

		std::string aName = anEvaluatedString.substr(aPercentPos + 1, aSecondPercentPos - aPercentPos - 1);

		std::string aValue;
		DefinesMap::const_iterator anItr = theDefinesMap.find(aName);		
		if (anItr != theDefinesMap.end())
			aValue = anItr->second;
		else
			aValue = "";		

		anEvaluatedString.erase(anEvaluatedString.begin() + aPercentPos, anEvaluatedString.begin() + aSecondPercentPos + 1);
		anEvaluatedString.insert(anEvaluatedString.begin() + aPercentPos, aValue.begin(), aValue.begin() + aValue.length());
	}

	return anEvaluatedString;
}

std::string Sexy::XMLDecodeString(const std::string& theString)
{
	std::string aNewString;

	// unused
	//int aUTF8Len = 0;
	//int aUTF8CurVal = 0;

	for (ulong i = 0; i < theString.length(); i++)
	{
		char c = theString[i];

		if (c == '&')
		{
			int aSemiPos = theString.find(';', i);

			if (aSemiPos != -1)
			{
				std::string anEntName = theString.substr(i+1, aSemiPos-i-1);
				i = aSemiPos;
											
				if (anEntName == "lt")
					c = '<';
				else if (anEntName == "amp")
					c = '&';
				else if (anEntName == "gt")
					c = '>';
				else if (anEntName == "quot")
					c = '"';
				else if (anEntName == "apos")
					c = '\'';
				else if (anEntName == "nbsp")
					c = ' ';
				else if (anEntName == "cr")
					c = '\n';
			}
		}				
		
		aNewString += c;
	}

	return aNewString;
}

std::wstring Sexy::XMLDecodeString(const std::wstring& theString)
{
	std::wstring aNewString;

	// unused
	//int aUTF8Len = 0;
	//int aUTF8CurVal = 0;

	for (ulong i = 0; i < theString.length(); i++)
	{
		wchar_t c = theString[i];

		if (c == L'&')
		{
			int aSemiPos = theString.find(L';', i);

			if (aSemiPos != -1)
			{
				std::wstring anEntName = theString.substr(i+1, aSemiPos-i-1);
				i = aSemiPos;
											
				if (anEntName == L"lt")
					c = L'<';
				else if (anEntName == L"amp")
					c = L'&';
				else if (anEntName == L"gt")
					c = L'>';
				else if (anEntName == L"quot")
					c = L'"';
				else if (anEntName == L"apos")
					c = L'\'';
				else if (anEntName == L"nbsp")
					c = L' ';
				else if (anEntName == L"cr")
					c = L'\n';
			}
		}				
		
		aNewString += c;
	}

	return aNewString;
}

std::string Sexy::XMLEncodeString(const std::string& theString)
{
	std::string aNewString;

	bool hasSpace = false;

	for (ulong i = 0; i < theString.length(); i++)
	{
		char c = theString[i];

		if (c == ' ')
		{
			if (hasSpace)
			{
				aNewString += "&nbsp;";
				continue;
			}
			
			hasSpace = true;
		}
		else
			hasSpace = false;

		/*if ((uchar) c >= 0x80)
		{
			// Convert to UTF
			aNewString += (char) (0xC0 | ((c >> 6) & 0xFF));
			aNewString += (char) (0x80 | (c & 0x3F));
		}
		else*/
		{		
			switch (c)
			{
			case '<':
				aNewString += "&lt;";
				break;
			case '&':		
				aNewString += "&amp;";
				break;
			case '>':
				aNewString += "&gt;";
				break;
			case '"':
				aNewString += "&quot;";
				break;
			case '\'':
				aNewString += "&apos;";
				break;
			case '\n':
				aNewString += "&cr;";
				break;
			default:
				aNewString += c;
				break;
			}
		}
	}

	return aNewString;
}

std::wstring Sexy::XMLEncodeString(const std::wstring& theString)
{
	std::wstring aNewString;

	bool hasSpace = false;

	for (ulong i = 0; i < theString.length(); i++)
	{
		wchar_t c = theString[i];

		if (c == ' ')
		{
			if (hasSpace)
			{
				aNewString += L"&nbsp;";
				continue;
			}
			
			hasSpace = true;
		}
		else
			hasSpace = false;

		/*if ((uchar) c >= 0x80)
		{
			// Convert to UTF
			aNewString += (char) (0xC0 | ((c >> 6) & 0xFF));
			aNewString += (char) (0x80 | (c & 0x3F));
		}
		else*/
		{		
			switch (c)
			{
			case L'<':
				aNewString += L"&lt;";
				break;
			case L'&':		
				aNewString += L"&amp;";
				break;
			case L'>':
				aNewString += L"&gt;";
				break;
			case L'"':
				aNewString += L"&quot;";
				break;
			case L'\'':
				aNewString += L"&apos;";
				break;
			case L'\n':
				aNewString += L"&cr;";
				break;
			default:
				aNewString += c;
				break;
			}
		}
	}

	return aNewString;
}

std::string Sexy::Upper(const std::string& _data)
{
	std::string s = _data;
	std::transform(s.begin(), s.end(), s.begin(), toupper);
	return s;
}

std::wstring Sexy::Upper(const std::wstring& _data)
{
	std::wstring s = _data;
	std::transform(s.begin(), s.end(), s.begin(), towupper);
	return s;
}

std::string Sexy::Lower(const std::string& _data)
{
	std::string s = _data;
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	return s;
}

std::wstring Sexy::Lower(const std::wstring& _data)
{
	std::wstring s = _data;
	std::transform(s.begin(), s.end(), s.begin(), towlower);
	return s;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int Sexy::StrFindNoCase(const char *theStr, const char *theFind)
{
	int p1,p2;
	int cp = 0;
	const int len1 = (int)strlen(theStr);
	const int len2 = (int)strlen(theFind);
	while(cp < len1)
	{
		p1 = cp;
		p2 = 0;
		while(p1<len1 && p2<len2)
		{
			if(tolower(theStr[p1])!=tolower(theFind[p2]))
				break;

			p1++; p2++;
		}
		if(p2==len2)
			return p1-len2;

		cp++;
	}

	return -1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool Sexy::StrPrefixNoCase(const char *theStr, const char *thePrefix, int maxLength)
{
	int i;
	char c1 = 0, c2 = 0;
	for (i=0; i<maxLength; i++)
	{
		c1 = tolower(*theStr++);
		c2 = tolower(*thePrefix++);

		if (c1==0 || c2==0)
			break;

		if (c1!=c2)
			return false;
	}

	return c2==0 || i==maxLength;
}

std::wstring Sexy::UTF8StringToWString(const std::string theString)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t> > cv;
	return cv.from_bytes(theString);
	/*
	int size = MultiByteToWideChar(CP_UTF8, 0, theString.c_str(), theString.length() + 1, nullptr, 0);
	wchar_t* buffer = new wchar_t[size];
	MultiByteToWideChar(CP_UTF8, 0, theString.c_str(), theString.length() + 1, buffer, size);
	std::wstring result = buffer;
	delete[] buffer;
	return result;
	*/
}

void Sexy::SMemR(void*& _Src, void* _Dst, size_t _Size)
{
	memcpy(_Dst, _Src, _Size);
	_Src = (void*)((size_t)_Src + _Size);
}

void Sexy::SMemRStr(void*& _Src, std::string& theString)
{
	size_t aStrLen;
	SMemR(_Src, &aStrLen, sizeof(aStrLen));
	theString.resize(aStrLen);
	SMemR(_Src, (void*)theString.c_str(), aStrLen);
}

void Sexy::SMemW(void*& _Dst, const void* _Src, size_t _Size)
{
	memcpy(_Dst, _Src, _Size);
	_Dst = (void*)((uintptr_t)_Dst + _Size);
}

void Sexy::SMemWStr(void*& _Dst, const std::string& theString)
{
	size_t aStrLen = theString.size();
	SMemW(_Dst, &aStrLen, sizeof(aStrLen));
	SMemW(_Dst, theString.c_str(), aStrLen);
}
