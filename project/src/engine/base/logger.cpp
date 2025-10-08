#include "logger.h"
#include <debugapi.h>
#include <Windows.h>
namespace logger {
	//ログ用関数
	void Log(const std::string& message) {
		OutputDebugStringA(message.c_str());
	}
}
