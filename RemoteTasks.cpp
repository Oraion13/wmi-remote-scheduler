#include "RemoteTasks.h"

IWbemServices* RemoteTasks::wmi_login(string username, string password, string computer_name) {
    HRESULT hres;
    Utils* utils = new Utils;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        char  buffer[200];
        sprintf_s(buffer, 200, "Failed to initialize COM library.Error code = 0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        return NULL;                  // Program has failed.
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------

    hres = CoInitializeSecurity(
        NULL,
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_CONNECT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE,    // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
    );

    if (FAILED(hres))
    {
        char  buffer[200];
        sprintf_s(buffer, 200, "Failed to initialize security. Error code = 0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        CoUninitialize();
        return NULL;                    // Program has failed.
    }

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        char  buffer[200];
        sprintf_s(buffer, 200, "Failed to create IWbemLocator object. Err code = 0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        CoUninitialize();
        return NULL;                 // Program has failed.
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices* pSvc = NULL;

    // Get the user name and password for the remote computer
    //CREDUI_INFO cui;
    bool useToken = false;
    bool useNTLM = true;

    wchar_t pszName[CREDUI_MAX_USERNAME_LENGTH + 1] = { 0 };
    wchar_t pszPwd[CREDUI_MAX_USERNAME_LENGTH + 1] = { 0 };
    wchar_t pszAuthority[CREDUI_MAX_USERNAME_LENGTH + 1];

    StringCchPrintf(pszName, CREDUI_MAX_USERNAME_LENGTH + 1, L"%s", wstring(username.begin(), username.end()).c_str());
    StringCchPrintf(pszPwd, CREDUI_MAX_USERNAME_LENGTH + 1, L"%s", wstring(password.begin(), password.end()).c_str());

    // change the computerName strings below to the full computer name
    // of the remote computer
    if (!useNTLM)
    {
        StringCchPrintf(pszAuthority, CREDUI_MAX_USERNAME_LENGTH + 1, L"kERBEROS:%s", wstring(computer_name.begin(), computer_name.end()).c_str()); // VM - DESKTOP-USOAAOQ, Local - INC-3449
    }

    // Connect to the remote root\cimv2 namespace
    // and obtain pointer pSvc to make IWbemServices calls.
    //---------------------------------------------------------
    string strNetworkResource = "\\\\" + computer_name + "\\root\\cimv2";
    hres = pLoc->ConnectServer(
        _bstr_t(wstring(strNetworkResource.begin(), strNetworkResource.end()).c_str()),
        _bstr_t(useToken ? NULL : pszName),    // User name
        _bstr_t(useToken ? NULL : pszPwd),     // User password
        NULL,                              // Locale             
        NULL,                              // Security flags
        _bstr_t(useNTLM ? NULL : pszAuthority),// Authority        
        NULL,                              // Context object 
        &pSvc                              // IWbemServices proxy
    );
  
    if (FAILED(hres))
    {
        char  buffer[200];
        sprintf_s(buffer, 200, "Could not connect. Error code = 0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        pLoc->Release();
        CoUninitialize();
        return NULL;                // Program has failed.
    }
    pLoc->Release();
    //CoUninitialize(); // for later usage

    SecureZeroMemory(pszName, sizeof(pszName));
    SecureZeroMemory(pszPwd, sizeof(pszPwd));

    return pSvc;
}

// ------------------------------------ Check if the OS is Windows -------------------------------------- //
boolean RemoteTasks::isWindows(IWbemServices* pSvc, Json::Value user_token) {
    HRESULT hres;
    Utils* utils = new Utils;

    // Get the user name and password for the remote computer
   //CREDUI_INFO cui;
    bool useToken = false;
    bool useNTLM = true;

    string username = user_token["username"].asCString();
    string password = user_token["password"].asCString();

    wchar_t pszName[CREDUI_MAX_USERNAME_LENGTH + 1] = { 0 };
    wchar_t pszPwd[CREDUI_MAX_USERNAME_LENGTH + 1] = { 0 };
    wchar_t pszDomain[CREDUI_MAX_USERNAME_LENGTH + 1];
    wchar_t pszUserName[CREDUI_MAX_USERNAME_LENGTH + 1];

    StringCchPrintf(pszName, CREDUI_MAX_USERNAME_LENGTH + 1, L"%s", wstring(username.begin(), username.end()).c_str());
    StringCchPrintf(pszPwd, CREDUI_MAX_USERNAME_LENGTH + 1, L"%s", wstring(password.begin(), password.end()).c_str());

    COAUTHIDENTITY* userAcct = NULL;
    COAUTHIDENTITY authIdent;

    if (!useToken)
    {
        memset(&authIdent, 0, sizeof(COAUTHIDENTITY));
        authIdent.PasswordLength = wcslen(pszPwd);
        authIdent.Password = (USHORT*)pszPwd;

        LPWSTR slash = wcschr(pszName, L'\\');
        if (slash == NULL)
        {
            char  buffer[200];
            sprintf_s(buffer, 200, "Could not create Auth identity. No domain specified");
            utils->sendStatus(400, 0, buffer);
            pSvc->Release();
            CoUninitialize();
            return false;               // Program has failed.
        }

        StringCchCopy(pszUserName, CREDUI_MAX_USERNAME_LENGTH + 1, slash + 1);
        authIdent.User = (USHORT*)pszUserName;
        authIdent.UserLength = wcslen(pszUserName);

        StringCchCopyN(pszDomain, CREDUI_MAX_USERNAME_LENGTH + 1, pszName, slash - pszName);
        authIdent.Domain = (USHORT*)pszDomain;
        authIdent.DomainLength = slash - pszName;
        authIdent.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        userAcct = &authIdent;
    }


    // Step 6: --------------------------------------------------
    // Set security levels on a WMI connection ------------------
    hres = CoSetProxyBlanket(
        pSvc,                           // Indicates the proxy to set
        RPC_C_AUTHN_DEFAULT,            // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_DEFAULT,            // RPC_C_AUTHZ_xxx
        COLE_DEFAULT_PRINCIPAL,         // Server principal name 
        RPC_C_AUTHN_LEVEL_CALL,  // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE,    // RPC_C_IMP_LEVEL_xxx
        userAcct,                       // client identity
        EOAC_NONE                       // proxy capabilities 
    );

    if (FAILED(hres))
    {
        char  buffer[200];
        sprintf_s(buffer, 200, "Could not set proxy blanket. Error code = 0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;               // Program has failed.
    }

    // Step 7: --------------------------------------------------
    // Use the IWbemServices pointer to make requests of WMI ----

    // For example, get the name of the operating system
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("Select * from Win32_OperatingSystem"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        char  buffer[200];
        sprintf_s(buffer, 200, "Query for operating system name failed. Error code = 0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;               // Program has failed.
    }

    // Step 8: -------------------------------------------------
    // Secure the enumerator proxy
    hres = CoSetProxyBlanket(
        pEnumerator,                    // Indicates the proxy to set
        RPC_C_AUTHN_DEFAULT,            // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_DEFAULT,            // RPC_C_AUTHZ_xxx
        COLE_DEFAULT_PRINCIPAL,         // Server principal name 
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE,    // RPC_C_IMP_LEVEL_xxx
        userAcct,                       // client identity
        EOAC_NONE                       // proxy capabilities 
    );

    if (FAILED(hres))
    {
        char  buffer[200];
        sprintf_s(buffer, 200, "Could not set proxy blanket on enumerator. Error code = 0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        pEnumerator->Release();
        pSvc->Release();
        CoUninitialize();
        return false;               // Program has failed.
    }


    // Step 9: -------------------------------------------------
    // Get the data from the query in step 7 -------------------

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        // Get the value of the Name property
        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        //wcout << " OS Name : " << vtProp.bstrVal << endl;
        if (wcsstr(vtProp.bstrVal, L"Windows") == NULL) {
            char  buffer[200];
            sprintf_s(buffer, 200, "Cannot perform task scheduling in this PC");
            utils->sendStatus(400, 0, buffer);
            pEnumerator->Release();
            pSvc->Release();

            VariantClear(&vtProp);
            pclsObj->Release();
            pclsObj = NULL;
            CoUninitialize();
            return false;
        }

        VariantClear(&vtProp);

        pclsObj->Release();
        pclsObj = NULL;
    }
    pEnumerator->Release();

    SecureZeroMemory(pszName, sizeof(pszName));
    SecureZeroMemory(pszPwd, sizeof(pszPwd));
    SecureZeroMemory(pszUserName, sizeof(pszUserName));
    SecureZeroMemory(pszDomain, sizeof(pszDomain));
    CoUninitialize();

    return true;
}

// ---------------------------------------------- Post task -------------------------------------------------- //
boolean RemoteTasks::wmi_post_tasks(IWbemServices* pSvc, Json::Value user_token, Json::Value task_token) {
    HRESULT hres;
    Utils* utils = new Utils;
    // Get the user name and password for the remote computer
   //CREDUI_INFO cui;
    bool useToken = false;
    bool useNTLM = true;

    string username = user_token["username"].asCString();
    string password = user_token["password"].asCString();

    wchar_t pszName[CREDUI_MAX_USERNAME_LENGTH + 1] = { 0 };
    wchar_t pszPwd[CREDUI_MAX_USERNAME_LENGTH + 1] = { 0 };
    wchar_t pszDomain[CREDUI_MAX_USERNAME_LENGTH + 1];
    wchar_t pszUserName[CREDUI_MAX_USERNAME_LENGTH + 1];

    StringCchPrintf(pszName, CREDUI_MAX_USERNAME_LENGTH + 1, L"%s", wstring(username.begin(), username.end()).c_str());
    StringCchPrintf(pszPwd, CREDUI_MAX_USERNAME_LENGTH + 1, L"%s", wstring(password.begin(), password.end()).c_str());

    COAUTHIDENTITY* userAcct = NULL;
    COAUTHIDENTITY authIdent;

    if (!useToken)
    {
        memset(&authIdent, 0, sizeof(COAUTHIDENTITY));
        authIdent.PasswordLength = wcslen(pszPwd);
        authIdent.Password = (USHORT*)pszPwd;

        LPWSTR slash = wcschr(pszName, L'\\');
        if (slash == NULL)
        {
            char  buffer[200];
            sprintf_s(buffer, 200, "Could not create Auth identity. No domain specified");
            utils->sendStatus(400, 0, buffer);
            pSvc->Release();
            CoUninitialize();
            return false;               // Program has failed.
        }

        StringCchCopy(pszUserName, CREDUI_MAX_USERNAME_LENGTH + 1, slash + 1);
        authIdent.User = (USHORT*)pszUserName;
        authIdent.UserLength = wcslen(pszUserName);

        StringCchCopyN(pszDomain, CREDUI_MAX_USERNAME_LENGTH + 1, pszName, slash - pszName);
        authIdent.Domain = (USHORT*)pszDomain;
        authIdent.DomainLength = slash - pszName;
        authIdent.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        userAcct = &authIdent;
    }


    // Step 6: --------------------------------------------------
    // Set security levels on a WMI connection ------------------
    hres = CoSetProxyBlanket(
        pSvc,                           // Indicates the proxy to set
        RPC_C_AUTHN_DEFAULT,            // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_DEFAULT,            // RPC_C_AUTHZ_xxx
        COLE_DEFAULT_PRINCIPAL,         // Server principal name 
        RPC_C_AUTHN_LEVEL_CALL,  // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE,    // RPC_C_IMP_LEVEL_xxx
        userAcct,                       // client identity
        EOAC_NONE                       // proxy capabilities 
    );

    if (FAILED(hres))
    {
        char  buffer[200];
        sprintf_s(buffer, 200, "Could not set proxy blanket. Error code = 0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;               // Program has failed.
    }

    // -------------------------------------------------------
    // Create scheduled tasks in remote system

    BSTR MethodName = SysAllocString(L"Create");
    BSTR ClassName = SysAllocString(L"Win32_ScheduledJob");

    IWbemClassObject* pClass = NULL;
    hres = pSvc->GetObject(ClassName, 0, NULL, &pClass, NULL);
    if (FAILED(hres)) {
        char  buffer[200];
        sprintf_s(buffer, 200, "GetObject Error: %u", GetLastError());
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;               // Program has failed.
    }

    // Input and Output parameters
    IWbemClassObject* pInSignature = NULL;
    IWbemClassObject* pOutSignature = NULL;
    hres = pClass->GetMethod(MethodName, 0, &pInSignature, &pOutSignature);

    // Check for get method errors
    //switch (hres) {
    //case WBEM_S_NO_ERROR:
    //    //cout << "_S_NO_ERROR: " << hex << hres << endl;
    //    break;
    //case WBEM_E_NOT_FOUND:
    //    cout << "_E_NOT_FOUND: " << hex << hres << endl;
    //    break;
    //case WBEM_E_OUT_OF_MEMORY:
    //    cout << "_E_OUT_OF_MEMORY: " << hex << hres << endl;
    //    break;
    //case 0x8004101e:
    //    wcout << "pClass->GetMethod returned 0x8004101e "
    //        << "(Illegal operation, apparently) for '"
    //        << MethodName << "'" << endl;
    //    break;
    //default:
    //    cout << "unknown: " << hex << hres << endl;
    //    break;
    //}

    if (FAILED(hres)) {
        char  buffer[200];
        sprintf_s(buffer, 200, "Could not get method");
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;
    }

    IWbemClassObject* pInParams = NULL;
    pInSignature->SpawnInstance(0, &pInParams);
    pInSignature->Release();
    IWbemClassObject* pOutParams = NULL;
    pOutSignature->SpawnInstance(0, &pOutParams);
    pOutSignature->Release();

    VARIANT argCommand;
    string title = task_token["title"].asCString();
    string description = task_token["description"].asCString();
    string command = "C:\Notifier.exe ";
    command.append(title);
    command.append(" 0809x ");
    command.append(description);
    VariantInit(&argCommand);
    argCommand.vt = VT_BSTR; // string type
    argCommand.bstrVal = utils->convertMBSToBSTR(command);
    hres = pInParams->Put(L"Command", 0, &argCommand, 0);

    if (FAILED(hres)) {
        char  buffer[200];
        sprintf_s(buffer, 200, "args Command error: 0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;
    }

    VARIANT argStartTime;
    string start_time = task_token["start_time"].asCString();
    VariantInit(&argStartTime);
    argStartTime.vt = VT_BSTR; // string type
    argStartTime.bstrVal = utils->convertMBSToBSTR(start_time);
    hres = pInParams->Put(L"StartTime", 0, &argStartTime, 0);

    if (FAILED(hres)) {
        char  buffer[200];
        sprintf_s(buffer, 200, "args StartTime error: 0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;
    }

    VARIANT argRunRepeatedly;
    bool run_repeat = task_token["run_repeat"].asBool();
    VariantInit(&argRunRepeatedly);
    argRunRepeatedly.vt = VT_BOOL; // bool type
    argRunRepeatedly.boolVal = run_repeat;
    hres = pInParams->Put(L"RunRepeatedly", 0, &argRunRepeatedly, 0);

    if (FAILED(hres)) {
        char  buffer[200];
        sprintf_s(buffer, 200, "args RunRepeatedly error:  0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;
    }

    VARIANT argDaysOfWeek;
    int days_of_week = task_token["days_of_week"].asInt();
    VariantInit(&argDaysOfWeek);
    argDaysOfWeek.vt = VT_I4;
    argDaysOfWeek.intVal = days_of_week; // 2 | 16; // Tuesday and Friday
    hres = pInParams->Put(L"DaysOfWeek", 0, &argDaysOfWeek, 0);
    if (FAILED(hres)) {
        char  buffer[200];
        sprintf_s(buffer, 200, "args DaysOfWeek error:  0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;
    }

    VARIANT argDaysOfMonth;
    int days_of_month = task_token["days_of_month"].asInt();
    VariantInit(&argDaysOfMonth);
    argDaysOfMonth.vt = VT_I4;
    argDaysOfMonth.intVal = days_of_month; // 1048576; // 21th day in month
    hres = pInParams->Put(L"DaysOfMonth", 0, &argDaysOfMonth, 0);
    if (FAILED(hres)) {
        char  buffer[200];
        sprintf_s(buffer, 200, "args DaysOfMonth error: 0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;
    }

    VARIANT argInteractWithDesktop;
    VariantInit(&argInteractWithDesktop);
    argInteractWithDesktop.vt = VT_BOOL;
    argInteractWithDesktop.boolVal = true;
    hres = pInParams->Put(L"InteractWithDesktop", 0, &argInteractWithDesktop, 0);
    if (FAILED(hres)) {
        char  buffer[200];
        sprintf_s(buffer, 200, "args InteractWithDesktop error:  0x%x", hres);
        utils->sendStatus(400, 0, buffer);
        return false;
    }

    TCHAR InstancePath[] = L"Win32_ScheduledJob=@";
    BSTR Text = NULL;
    IWbemClassObject* pOutParam = NULL;
    string object_path = "\\\\";
    object_path.append(user_token["computer_name"].asCString());
    object_path.append("\\root\\cimv2:Win32_ScheduledJob");
    hres = pSvc->ExecMethod(utils->convertMBSToBSTR(object_path), MethodName, 0,
        NULL, pInParams, &pOutParam, NULL);

    if (FAILED(hres)) {
        char  buffer[200];
        sprintf_s(buffer, 200, "ExecMethod error: 0x%x", hres);
        // See http://msdn.microsoft.com/en-us/library/cc250949%28v=prot.10%29.aspx
        // for returned codes. This Windows error handling is ghastly.
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;
    }

    pOutParam->GetObjectText(0, &Text);
    if (wcsstr(Text, L"ReturnValue = 0;") == NULL) {
        char  buffer[200];
        //string msg = utils->callConvertWCSToMBS(&Text);
        //printf("%S", Text);
        sprintf_s(buffer, 200, "Cannot post task");
        utils->sendStatus(400, 0, buffer);
        pSvc->Release();
        CoUninitialize();
        return false;
    }


    // Cleanup
    // ========
        // When you have finished using the credentials,
    // erase them from memory.
    SecureZeroMemory(pszName, sizeof(pszName));
    SecureZeroMemory(pszPwd, sizeof(pszPwd));
    SecureZeroMemory(pszUserName, sizeof(pszUserName));
    SecureZeroMemory(pszDomain, sizeof(pszDomain));

    VariantClear(&argCommand);
    VariantClear(&argStartTime);
    VariantClear(&argRunRepeatedly);
    VariantClear(&argDaysOfWeek);
    VariantClear(&argDaysOfMonth);
    VariantClear(&argInteractWithDesktop);

    pInParams->Release();
    pOutParams->Release();

    pSvc->Release();

    CoUninitialize();

    return true;
}