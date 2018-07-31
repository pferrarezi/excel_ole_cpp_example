#include <iostream>
#include <ole2.h>
#include <ctime>

#define lpolestr_cast const_cast<LPOLESTR>


// AutoWrap() - Automation helper function...
HRESULT AutoWrap(int autoType, VARIANT *pvResult, IDispatch *pDisp, LPOLESTR ptName, int cArgs...) {
    // Begin variable-argument list...
    va_list marker;
            va_start(marker, cArgs);

    if (!pDisp) {
        //MessageBox(nullptr, "NULL IDispatch passed to AutoWrap()", "Error", 0x10010);
        std::cout << "Erro: NULL IDispatch passed to AutoWrap()" << std::endl;
        _exit(0);
    }

    // Variables used...
    DISPPARAMS dp = { nullptr, nullptr, 0, 0 };
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPID dispID;
    HRESULT hr;
    char buf[200];
    char szName[200];


    // Convert down to ANSI
    WideCharToMultiByte(CP_ACP, 0, ptName, -1, szName, 256, nullptr, nullptr);

    // Get DISPID for name passed...
    hr = pDisp->GetIDsOfNames(IID_NULL, &ptName, 1, LOCALE_USER_DEFAULT, &dispID);
    if (FAILED(hr)) {
        sprintf(buf, "IDispatch::GetIDsOfNames(\"%s\") failed w/err 0x%08lx", szName, hr);
        std::cout << "Erro: AutoWrap()" << std::endl;
        //MessageBox(nullptr, buf, "AutoWrap()", 0x10010);
        _exit(0);
        return hr;
    }

    // Allocate memory for arguments...
    auto *pArgs = new VARIANT[cArgs + 1];

    // Extract arguments...
    for (int i = 0; i<cArgs; i++) {
        pArgs[i] = va_arg(marker, VARIANT);
    }

    // Build DISPPARAMS
    dp.cArgs = cArgs;
    dp.rgvarg = pArgs;

    // Handle special-case for property-puts!
    if (autoType & DISPATCH_PROPERTYPUT) {
        dp.cNamedArgs = 1;
        dp.rgdispidNamedArgs = &dispidNamed;
    }

    // Make the call!
    hr = pDisp->Invoke(dispID, IID_NULL, LOCALE_SYSTEM_DEFAULT, static_cast<WORD>(autoType), &dp, pvResult, nullptr, nullptr);
    if (FAILED(hr)) {
        sprintf(buf, "IDispatch::Invoke(\"%s\"=%08lx) failed w/err 0x%08lx", szName, dispID, hr);
        //MessageBox(nullptr, buf, "AutoWrap()", 0x10010);
        std::cout << "Erro: AutoWrap()" << std::endl;
        _exit(0);
        return hr;
    }
    // End variable-argument section...
            va_end(marker);

    delete[] pArgs;

    return hr;
}

int main()
{
    std::clock_t begin = std::clock();
    // Initialize COM for this thread...
    CoInitialize(nullptr);

    // Get CLSID for our server...
    CLSID clsid;
    HRESULT hr = CLSIDFromProgID(L"Excel.Application", &clsid);

    if (FAILED(hr)) {
        std::cout << "Erro: CLSIDFromProgID() falhou" << std::endl;
        //::MessageBox(nullptr, "CLSIDFromProgID() failed", "Error", 0x10010);
        return -1;
    }

    // Start server and get IDispatch...
    IDispatch *pXlApp;
    hr = CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER, IID_IDispatch, (void **)&pXlApp);
    if (FAILED(hr)) {
        std::cout << "Erro: Excel not registered properly" << std::endl;
        //::MessageBox(nullptr, "Excel not registered properly", "Error", 0x10010);
        return -2;
    }

    // Make it visible (i.e. app.visible = 1)
    {

        VARIANT x;
        x.vt = VT_I4;
        x.lVal = 1;
        AutoWrap(DISPATCH_PROPERTYPUT, nullptr, pXlApp, lpolestr_cast(L"Visible"), 1, x);
    }

    // Get Workbooks collection
    IDispatch *pXlBooks;
    {
        VARIANT result;
        VariantInit(&result);
        AutoWrap(DISPATCH_PROPERTYGET, &result, pXlApp, lpolestr_cast(L"Workbooks"), 0);
        pXlBooks = result.pdispVal;
    }

    // Call Workbooks.Add() to get a new workbook...
    IDispatch *pXlBook;
    {
        VARIANT result;
        VariantInit(&result);
        AutoWrap(DISPATCH_PROPERTYGET, &result, pXlBooks, lpolestr_cast(L"Add"), 0);
        pXlBook = result.pdispVal;
    }
    // ### Screen updating = false
    {

        VARIANT x;
        x.vt = VT_I4;
        x.lVal = 0;
        AutoWrap(DISPATCH_PROPERTYPUT, nullptr, pXlApp, lpolestr_cast(L"ScreenUpdating"), 1, x);
    }

    // ### Calculation = manual
    {
        VARIANT x;
        x.vt = VT_I4;
        x.lVal = -4135;
        AutoWrap(DISPATCH_PROPERTYPUT, nullptr, pXlApp, lpolestr_cast(L"Calculation"), 1, x);
    }

    // Create a 15x15 safearray of variants...
    unsigned int qtde_celulas = 700;
    VARIANT arr;
    arr.vt = VT_ARRAY | VT_VARIANT;
    {
        SAFEARRAYBOUND sab[2];
        sab[0].lLbound = 1;
        sab[0].cElements = qtde_celulas;
        sab[1].lLbound = 1;
        sab[1].cElements = qtde_celulas;
        arr.parray = SafeArrayCreate(VT_VARIANT, 2, sab);
    }

    // Fill safearray with some values...
    for (int i = 1; i <= qtde_celulas; i++) {
        for (int j = 1; j <= qtde_celulas; j++) {
            // Create entry value for (i,j)
            VARIANT tmp;
            tmp.vt = VT_I4;
            tmp.lVal = i * j;
            // Add to safearray...
            long indices[] = { i,j };
            SafeArrayPutElement(arr.parray, indices, (void *)&tmp);
        }
    }

    // Get ActiveSheet object
    IDispatch *pXlSheet;
    {
        VARIANT result;
        VariantInit(&result);
        AutoWrap(DISPATCH_PROPERTYGET, &result, pXlApp, lpolestr_cast(L"ActiveSheet"), 0);
        pXlSheet = result.pdispVal;
    }

    // Get Range object for the Range A1:O15...
    IDispatch *pXlRange;
    {
        VARIANT parm;
        parm.vt = VT_BSTR;
        parm.bstrVal = ::SysAllocString(L"A1:ZX700");

        VARIANT result;
        VariantInit(&result);
        AutoWrap(DISPATCH_PROPERTYGET, &result, pXlSheet, lpolestr_cast(L"Range"), 1, parm);
        VariantClear(&parm);

        pXlRange = result.pdispVal;
    }

    // Set range with our safearray...
    AutoWrap(DISPATCH_PROPERTYPUT, nullptr, pXlRange, lpolestr_cast(L"Value"), 1, arr);


    // ### Screen updating = true
    {

        VARIANT x;
        x.vt = VT_I4;
        x.lVal = 1;
        AutoWrap(DISPATCH_PROPERTYPUT, nullptr, pXlApp, lpolestr_cast(L"ScreenUpdating"), 1, x);
    }
    // ### Calculation = manual
    {
        VARIANT x;
        x.vt = VT_I4;
        x.lVal = -4105;
        AutoWrap(DISPATCH_PROPERTYPUT, nullptr, pXlApp, lpolestr_cast(L"Calculation"), 1, x);
    }

    // Wait for user...

    std::clock_t end = std::clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "tempo: " << elapsed_secs << std::endl;
    int a;
    std::cin >> a;
    //::MessageBox(nullptr, "All done.", "Notice", 0x10000);

    // Set .Saved property of workbook to TRUE so we aren't prompted
    // to save when we tell Excel to quit...
    {
        VARIANT x;
        x.vt = VT_I4;
        x.lVal = 1;
        AutoWrap(DISPATCH_PROPERTYPUT, nullptr, pXlBook, lpolestr_cast(L"Saved"), 1, x);
    }

    // Tell Excel to quit (i.e. App.Quit)
    AutoWrap(DISPATCH_METHOD, nullptr, pXlApp, lpolestr_cast(L"Quit"), 0);

    // Release references...
    pXlRange->Release();
    pXlSheet->Release();
    pXlBook->Release();
    pXlBooks->Release();
    pXlApp->Release();
    VariantClear(&arr);

    // Uninitialize COM for this thread...
    CoUninitialize();
    return 0;
}