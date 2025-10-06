#pragma once
#include<Windows.h>
#include <wrl/client.h>
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
class Input
{
public:
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	void Initialize(HINSTANCE hInstance, HWND hwnd);
	void Update();

	bool pushKey(BYTE keyNumber);
	bool triggerKey(BYTE keyNumber);

private:
	ComPtr<IDirectInput8> directInput_;
	ComPtr<IDirectInputDevice8> keyboard_;
	ComPtr<IDirectInputDevice8> mouse_;
	BYTE keys_[256] = {};
	BYTE preKeys_[256] = {};
};

