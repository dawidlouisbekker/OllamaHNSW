#pragma once
#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <vector>
#include <iostream>


#pragma comment(lib, "wbemuuid.lib")


void printWMIError(HRESULT hr) {
    _com_error err(hr);
    std::wcout << L"Error: " << err.ErrorMessage() << std::endl;
}

void printVariantValue(const VARIANT& vtProp) {
    switch (vtProp.vt) {
    case VT_BSTR:
        std::wcout << vtProp.bstrVal << std::endl;
        break;
    case VT_I4:
        std::wcout << vtProp.lVal << std::endl;
        break;
    case VT_BOOL:
        std::wcout << (vtProp.boolVal ? L"True" : L"False") << std::endl;
        break;
    case VT_ARRAY | VT_I4:
    case VT_ARRAY | VT_UI1: {
        // Handle arrays (e.g., byte arrays or integer arrays)
        SAFEARRAY* pSafeArray = vtProp.parray;
        long lLBound, lUBound;
        SafeArrayGetLBound(pSafeArray, 1, &lLBound);
        SafeArrayGetUBound(pSafeArray, 1, &lUBound);
        for (long i = lLBound; i <= lUBound; ++i) {
            long val;
            SafeArrayGetElement(pSafeArray, &i, &val);
            std::wcout << val << L" ";
        }
        std::wcout << std::endl;
        break;
    }
    case VT_UNKNOWN: {
        // For object references (like `Win32_ComputerSystem`)
        std::wcout << L"Object Reference (unknown type)" << std::endl;
        break;
    }
    default:
        std::wcout << L"Unsupported or unknown type" << std::endl;
        break;
    }
}


void listHardDriveControllers() {
    HRESULT hres;

    // Initialize COM.
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        printWMIError(hres);
        return;
    }

    // Set up security levels for WMI access.
    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL
    );
    if (FAILED(hres)) {
        printWMIError(hres);
        CoUninitialize();
        return;
    }

    // Initialize the IWbemLocator.
    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(
        CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc
    );
    if (FAILED(hres)) {
        printWMIError(hres);
        CoUninitialize();
        return;
    }

    // Connect to WMI namespace.
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(
        BSTR(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc
    );
    if (FAILED(hres)) {
        printWMIError(hres);
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // Set security levels on the proxy.
    hres = CoSetProxyBlanket(
        pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE
    );
    if (FAILED(hres)) {
        printWMIError(hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // Query for hard drive controllers.
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        BSTR(L"WQL"), BSTR(L"SELECT * FROM Win32_DiskDrive"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator
    );
    if (FAILED(hres)) {
        printWMIError(hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // Retrieve the results.
    IWbemClassObject* pClassObject;
    ULONG uReturn = 0;
    while (pEnumerator) {
        hres = pEnumerator->Next(WBEM_INFINITE, 1, &pClassObject, &uReturn);
        if (0 == uReturn) {
            break;
        }

        // Enumerate all properties of the class object.
        SAFEARRAY* psa = NULL;
        hres = pClassObject->GetNames(0, WBEM_FLAG_ALWAYS, NULL, &psa);
        if (FAILED(hres)) {
            printWMIError(hres);
            pClassObject->Release();
            continue;
        }

        long lLBound, lUBound;
        SafeArrayGetLBound(psa, 1, &lLBound);
        SafeArrayGetUBound(psa, 1, &lUBound);

        // Loop through all property names.
        for (long i = lLBound; i <= lUBound; ++i) {
            BSTR bstrPropName;
            hres = SafeArrayGetElement(psa, &i, &bstrPropName);
            if (FAILED(hres)) {
                printWMIError(hres);
                continue;
            }

            VARIANT vtProp;
            hres = pClassObject->Get(bstrPropName, 0, &vtProp, 0, 0);
            if (FAILED(hres)) {
                printWMIError(hres);
                continue;
            }

            std::wcout << L"Property: " << bstrPropName << L" = ";
            printVariantValue(vtProp);

            VariantClear(&vtProp);
        }

        // Release the property name array and the class object.
        SafeArrayDestroy(psa);
        pClassObject->Release();
    }

    // Clean up.
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();
}
