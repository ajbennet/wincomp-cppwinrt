#include "pch.h"
#include "Class.h"

namespace winrt::WinRTComponent::implementation
{

	hstring Class::MyProperty()
    {
		return L"Hello from an UNPACKAGED WinRT Component! :D";

//        throw hresult_not_implemented();

    }

    void Class::MyProperty(hstring /* value */)
    {
        // throw hresult_not_implemented();
    }

	hstring Class::MyProperty2()
    {
		return L"Hello from an UNPACKAGED WinRT Component! :D";

//        throw hresult_not_implemented();

    }

    void Class::MyProperty2(hstring /* value */)
    {
        // throw hresult_not_implemented();
    }
}
