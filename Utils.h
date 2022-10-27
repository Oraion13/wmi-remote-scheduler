#pragma once

#include <iostream>
#include <WTypes.h>
#include <string>
#include <json/json.h>

using namespace std;

class Utils
{
public:
	BSTR convertMBSToBSTR(const string& str);
	string convertWCSToMBS(const wchar_t* pstr, long wslen);
	string callConvertWCSToMBS(BSTR* bstr);

	Json::Value readRequestBody();
};

