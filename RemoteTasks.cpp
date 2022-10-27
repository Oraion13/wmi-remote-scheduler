#include "RemoteTasks.h"

IWbemServices* RemoteTasks::wmi_login(string username, string password, string computer_name) {
    cout << "Content-type:text/html\r\n\r\n";

    HRESULT hres;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        printf("<script>alert(\"Failed to initialize COM library. Error code = 0x%x\")</script>\r\n\r\n", hres);
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
        printf("<script>alert(\"Failed to initialize security. Error code = 0x%x\")</script>\r\n\r\n", hres);
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
        printf("<script>alert(\"Failed to create IWbemLocator object. Err code = 0xx%x\")</script>\r\n\r\n", hres);
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
        printf("<script>alert(\"Could not connect. Error code = 0x%x\")</script>\r\n\r\n", hres);
        pLoc->Release();
        CoUninitialize();
        return NULL;                // Program has failed.
    }
    pLoc->Release();
    CoUninitialize();

    SecureZeroMemory(pszName, sizeof(pszName));
    SecureZeroMemory(pszPwd, sizeof(pszPwd));

    return pSvc;
}

// ------------------------------------ Check if the OS is Windows -------------------------------------- //
boolean RemoteTasks::isWindows(IWbemServices* pSvc, Json::Value user_token) {
    HRESULT hres;
    // Get the user name and password for the remote computer
   //CREDUI_INFO cui;
    bool useToken = false;
    bool useNTLM = true;

    string username = user_token["username"].asCString();
    string password = user_token["password"].asCString();
    string computer_name = user_token["computer_name"].asCString();

    wchar_t pszName[CREDUI_MAX_USERNAME_LENGTH + 1] = { 0 };
    wchar_t pszPwd[CREDUI_MAX_USERNAME_LENGTH + 1] = { 0 };
    wchar_t pszDomain[CREDUI_MAX_USERNAME_LENGTH + 1];
    wchar_t pszUserName[CREDUI_MAX_USERNAME_LENGTH + 1];
    wchar_t pszAuthority[CREDUI_MAX_USERNAME_LENGTH + 1];

    StringCchPrintf(pszName, CREDUI_MAX_USERNAME_LENGTH + 1, L"%s", wstring(username.begin(), username.end()).c_str());
    StringCchPrintf(pszPwd, CREDUI_MAX_USERNAME_LENGTH + 1, L"%s", wstring(password.begin(), password.end()).c_str());

    COAUTHIDENTITY* userAcct = NULL;
    COAUTHIDENTITY authIdent;

    // Initialize COM. ------------------------------------------

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        printf("<script>alert(\"Failed to initialize COM library. Error code = 0x%x\")</script>\r\n\r\n", hres);
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
        printf("<script>alert(\"Failed to initialize security. Error code = 0x%x\")</script>\r\n\r\n", hres);
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
        printf("<script>alert(\"Failed to create IWbemLocator object. Err code = 0xx%x\")</script>\r\n\r\n", hres);
        CoUninitialize();
        return NULL;                 // Program has failed.
    }

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
        printf("<script>alert(\"Could not connect. Error code = 0x%x\")</script>\r\n\r\n", hres);
        pLoc->Release();
        CoUninitialize();
        return NULL;                // Program has failed.
    }
    pLoc->Release();

    if (!useToken)
    {
        memset(&authIdent, 0, sizeof(COAUTHIDENTITY));
        authIdent.PasswordLength = wcslen(pszPwd);
        authIdent.Password = (USHORT*)pszPwd;

        LPWSTR slash = wcschr(pszName, L'\\');
        if (slash == NULL)
        {
            printf("<script>alert(\"Could not create Auth identity. No domain specified\")</script>\r\n\r\n");
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

        BSTR ClassName = SysAllocString(L"Win32_ScheduledJob");

        //wcout << " name: " << pszName << endl;
        //wcout << " pwd: " << pszPwd << endl;
        //wcout << " domain: " << pszDomain << endl;
        //wcout << " user: " << pszUserName << endl;
        //wcout << " auth: " << pszAuthority << endl;
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
        printf("<script>alert(\"Could not set proxy blanket. Error code = 0x%x\")</script>\r\n\r\n", hres);
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
        printf("<script>alert(\"Query for operating system name failed. Error code = 0x%x\")</script>\r\n\r\n", hres);
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
        printf("<script>alert(\"Could not set proxy blanket on enumerator. Error code = 0x%x\")</script>\r\n\r\n", hres);
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
        if (wcsstr(vtProp.bstrVal, L"Windows") != NULL) {
            // set cookies
            cout << "<script>"
                << "window.localStorage.setItem(\"user_token\", JSON.stringify(" << user_token << ")); "
                << "</script>";
        }
        else {
            printf("<script>alert(\"Cannot perform task scheduling in this PC\")</script>\r\n\r\n");
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