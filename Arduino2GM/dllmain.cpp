#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "Interface.h"
#include <windows.h>
#include <string>

#define PushCLua( _function, _name ) LUA->PushCFunction(_function); LUA->SetField(-2, _name);
using namespace GarrysMod::Lua;
HANDLE hSerial;
int comState = -10;

int ReadByte(lua_State* state)
{
	int Nmb_Of_Input_Bytes = LUA->GetNumber(1);
	byte* Input_Bytes = new byte[Nmb_Of_Input_Bytes]();

	DWORD Read_Len = 0;
	ReadFile(hSerial, Input_Bytes, Nmb_Of_Input_Bytes, &Read_Len, NULL);
	PurgeComm(hSerial, PURGE_RXCLEAR);

	LUA->Top();
	LUA->CreateTable();
	for (int i = 0; i < Nmb_Of_Input_Bytes; i++) {
		LUA->PushNumber(i); // Push index
		LUA->PushNumber(Input_Bytes[i]); // Push value by index
		LUA->RawSet(-3); // Push table
	}

	delete[] Input_Bytes;

	return 1;
}

int WriteByte(lua_State* state)
{
	int Bytes_To_Send = LUA->GetNumber(2);
	byte* Input_Bytes = new byte[Bytes_To_Send + 2]();
	if (LUA->IsType(1, Type::TABLE))
	{
		unsigned int Input_Bytes_Index = 0;
		LUA->PushNil();
		while (LUA->Next(1) != 0) {
			Input_Bytes[Input_Bytes_Index++] = (byte)LUA->GetNumber(-1);
			LUA->Pop(1);
		}
	}

	DWORD WrBytes = 0;
	WriteFile(hSerial, Input_Bytes, (DWORD)Bytes_To_Send, &WrBytes, NULL);
	delete[] Input_Bytes;

	return 0;
}

int StartCOM(lua_State* state)
{
	// Get parameters
	int portNumber = LUA->GetNumber(1);
	int portBaudRate = LUA->GetNumber(2);

	// Connect to COM
	LPTSTR inPortName = new TCHAR[12];
	wsprintfW(inPortName, L"\\\\.\\COM%d", portNumber);
	hSerial = CreateFile(inPortName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	Sleep(500);
	// Check errors
	if (hSerial == INVALID_HANDLE_VALUE) // Если не удалось открыть
	{
		switch (GetLastError())
		{
		case ERROR_FILE_NOT_FOUND:
			comState = -1;
			break;
		default:
			comState = -2;
			break;
		}
		LUA->PushNumber(comState);
		CloseHandle(hSerial);
		hSerial = NULL;
		return 1;
	}

	// Set parameters
	DCB serialParams;
	if (!GetCommState(hSerial, &serialParams)) {
		comState = -3;
		LUA->PushNumber(comState);
		CloseHandle(hSerial);
		hSerial = NULL;
		return 1;
	}
	serialParams.BaudRate = (DWORD)portBaudRate;
	serialParams.ByteSize = 8;
	serialParams.Parity = PARITY_NONE;
	serialParams.StopBits = ONESTOPBIT;
	if (!SetCommState(hSerial, &serialParams))
	{
		comState = -4;
		LUA->PushNumber(comState);
		CloseHandle(hSerial);
		hSerial = NULL;
		return 1;
	}

	// Set timeouts
	COMMTIMEOUTS serialTimeout;
	serialTimeout.ReadIntervalTimeout = 50;
	serialTimeout.ReadTotalTimeoutMultiplier = 0;
	serialTimeout.ReadTotalTimeoutConstant = 0;
	serialTimeout.WriteTotalTimeoutMultiplier = 0;
	serialTimeout.WriteTotalTimeoutConstant = 0;
	if (!SetCommTimeouts(hSerial, &serialTimeout)) {
		comState = -5;
		LUA->PushNumber(comState);
		CloseHandle(hSerial);
		hSerial = NULL;
		return 1;
	}

	comState = 0;
	LUA->PushNumber(comState);
	return 1;
}

int StopCOM(lua_State* state)
{
	if (!((hSerial == INVALID_HANDLE_VALUE) || (hSerial == NULL)))
	{
		comState = -10;
		PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
		CloseHandle(hSerial);
		hSerial = NULL;
	}
	return 0;
}

int GetCOMState(lua_State* state)
{
	LUA->PushNumber(comState);
	return 1;
}

int Version(lua_State* state)
{
	char date_str[30];
	sprintf_s(date_str, "Bulded %s %s", __TIME__, __DATE__);
	LUA->PushString(date_str);
	return 1;
}

// Called when the module opens
GMOD_MODULE_OPEN()
{
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->CreateTable();
			PushCLua(Version, "Version");		 // string UART.Version()
			PushCLua(StartCOM, "StartCOM");	 	 // int UART.StartCOM(port)
			PushCLua(StopCOM, "StopCOM");		 // int UART.StopCOM()
			PushCLua(WriteByte, "WriteByte");	 // UART.WriteByte(table Bytes, int nBytes)
			PushCLua(ReadByte, "ReadByte");		 // string UART.ReadByte(int bytesToRead)
			PushCLua(GetCOMState, "GetCOMState");// int UART.GetCOMState()
		LUA->SetField(-2, "UART");
	LUA->Pop();

	return 0;
}
// Called when the module closes
GMOD_MODULE_CLOSE()
{
	PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
	CloseHandle(hSerial);
	hSerial = NULL;
	return 0;
}