#pragma once

#define _WIN32_DCOM
#define UNICODE
#include <iostream>
#include <vector>  
#include <string>  
#include <stdio.h>  
#include <stdlib.h> 
#include <random>

using namespace std;
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "credui.lib")
#pragma comment(lib, "comsuppw.lib")
#include <wincred.h>
#include <strsafe.h>

#include <json/json.h>

#include "Utils.h"

using namespace std;

class RemoteTasks
{
public:
	IWbemServices* wmi_login(string username, string password, string computer_name);
	boolean isWindows(IWbemServices* pSvc, Json::Value user_token);
	boolean wmi_post_tasks(IWbemServices* pSvc, Json::Value user_token, Json::Value task_token);
};

