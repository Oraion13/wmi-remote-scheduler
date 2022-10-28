#include "Utils.h"

// convert string to BSTR
BSTR Utils::convertMBSToBSTR(const string& str) {
    int wslen = ::MultiByteToWideChar(CP_ACP, 0, str.data(), str.length(), NULL, 0);

    BSTR wsdata = ::SysAllocStringLen(NULL, wslen);
    ::MultiByteToWideChar(CP_ACP, 0, str.data(), str.length(), wsdata, wslen);

    return wsdata;
}

// convert wchar_t to string
string Utils::convertWCSToMBS(const wchar_t* pstr, long wslen) {
    int len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, NULL, 0, NULL, NULL);

    string dblstr(len, '\0');
    len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, &dblstr[0], len, NULL, NULL);

    return dblstr;
}

// convert BSTR to string
string Utils::callConvertWCSToMBS(BSTR* bstr) {
    UINT length = SysStringLen(*bstr);        // Ask COM for the size of the BSTR
    wchar_t* wcs_string = new wchar_t[length + 1]; // Note: SysStringLen doesn't 
    // include the space needed for the NULL
    wcscpy_s(wcs_string, length + 1, *bstr);

    return convertWCSToMBS(wcs_string, wcslen(wcs_string));
}

// read json data from the page
Json::Value Utils::readRequestBody() {
    // Copy all data from cin, using iterators.
    istreambuf_iterator<char> begin(cin);
    istreambuf_iterator<char> end;
    string json_str(begin, end);

    Json::Reader reader;
    Json::Value root;

    reader.parse(json_str, root);

    return root == NULL ? NULL : root;
}

// send status
void Utils::sendStatus(int status_code, int key, char* value) {
    if (key == 0) {
        printf("Status: %d Bad Request\r\n\r\n", status_code);
        printf("{\"error\":\"%s\"}\r\n\r\n", value);
    }
    else {
        printf("Status: %d OK\r\n\r\n", status_code);
        printf("{\"message\":\"%s\"}\r\n\r\n", value);
    }
}