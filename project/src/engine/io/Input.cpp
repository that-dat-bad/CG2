#include "Input.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#include <cassert>




void Input::Initialize(HINSTANCE hInstance,HWND hwnd)
{
	HRESULT result;



	//インスタンスの生成
	result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput_, nullptr);
	assert(SUCCEEDED(result));

	//キーボードデバイスの生成
	result = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
	assert(SUCCEEDED(result));

	//入力データのセット
	result = keyboard_->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	//排他制御レベルのセット
	result = keyboard_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));



	//マウスデバイスの生成
	result = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
	assert(SUCCEEDED(result));

	//入力データのセット
	result = mouse_->SetDataFormat(&c_dfDIMouse2);
	assert(SUCCEEDED(result));

	//排他制御レベルのセット
	mouse_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	assert(SUCCEEDED(result));
}

void Input::Update()
{
	//キー入力を保存
	memcpy(preKeys_, keys_, sizeof(keys_));

	// 入力取得
	keyboard_->Acquire();
	mouse_->Acquire();
	keyboard_->GetDeviceState(sizeof(keys_), keys_);
	DIMOUSESTATE2 mouseState = {};
	mouse_->GetDeviceState(sizeof(mouseState), &mouseState);

}

bool Input::pushKey(BYTE keyNumber)
{
	if (keys_[keyNumber])
	{
		return true;
	}
	return false;
}

bool Input::triggerKey(BYTE keyNumber)
{
	if (!preKeys_[keyNumber]&&keys_[keyNumber])
	{
		return true;
	}

	return false;
}
