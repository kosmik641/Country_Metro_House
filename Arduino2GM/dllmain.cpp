#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "Interface.h"
#include <windows.h>
#include <string>

/*
В return функции возвращается кол-во возвращаемых в Lua объектов
*/

#define PushCLua( _function, _name ) LUA->PushCFunction(_function); LUA->SetField(-2, _name);
using namespace GarrysMod::Lua;
HANDLE m_HCOM;
int comState = -10;

int WriteByte(lua_State* state)
{
	unsigned int Bytes_To_Send = LUA->GetNumber(2); // Кол-во входящих байт
	byte* Input_Bytes = new byte[Bytes_To_Send + 2]();
	if (LUA->IsType(1, Type::TABLE)) // Проверяем, что первый параметр - таблица
	{
		unsigned int Input_Bytes_Index = 0;
		LUA->PushNil(); // Первый ключ
		// Функция lua_next перебирает все пары "ключ"-"значение" в таблице,
		// Вторым параметром указывается индекс в стеке, по которому расположен массив (таблица Lua)

		while (LUA->Next(1) != 0) {
			// В паре "ключ" находится по индексу -2, "значение" находится по индексу -1
			Input_Bytes[Input_Bytes_Index++] = (byte)LUA->GetNumber(-1);
			LUA->Pop(1);// освобождает стек для следующей итерации
		}
	}

	DWORD WrBytes = 0;
	WriteFile(m_HCOM, Input_Bytes, (DWORD)Bytes_To_Send, &WrBytes, NULL);
	delete[] Input_Bytes;

	return 0;
}

int ReadByte(lua_State* state)
{
	int Nmb_Of_Input_Bytes = LUA->GetNumber(1);
	byte* Input_Bytes = new byte[Nmb_Of_Input_Bytes + 2]();

	DWORD Read_Len = 0;
	ReadFile(m_HCOM, Input_Bytes, Nmb_Of_Input_Bytes, &Read_Len, NULL);
	PurgeComm(m_HCOM, PURGE_RXCLEAR);

	LUA->Top();
	LUA->CreateTable();
	for (int i = 0; i < Nmb_Of_Input_Bytes; i++) {
		LUA->PushNumber(i); // Кладём индекс
		LUA->PushNumber(Input_Bytes[i]); // Значение по индексу
		LUA->RawSet(-3); // Закидываем в таблицу
	}

	delete[] Input_Bytes;

	return 1;
}

int StartCOM(lua_State* state)
{
	int portNumber = LUA->GetNumber(1);
	int portBaudRate = LUA->GetNumber(2);

	wchar_t port_numb_str[14];

	wsprintfW(port_numb_str, L"\\\\.\\COM%d", portNumber);
	m_HCOM = CreateFile(port_numb_str, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	Sleep(1000);
	if (m_HCOM == INVALID_HANDLE_VALUE)
	{
		comState = -1;
		LUA->PushNumber(comState);
		return 1;
	}
	else
	{
		DCB serialParams;
		int Sys = GetCommState(m_HCOM, &serialParams);

		if (Sys == 0)
		{
			comState = -2;
			LUA->PushNumber(comState);
			return 1;
		}

		serialParams.BaudRate = (DWORD)portBaudRate;
		serialParams.ByteSize = 8;
		serialParams.Parity = NOPARITY;
		serialParams.StopBits = ONESTOPBIT;
		Sys = SetCommState(m_HCOM, &serialParams);

		if (Sys == 0)
		{
			comState = -3;
			LUA->PushNumber(comState);
			return 1;
		}

		SetCommMask(m_HCOM, EV_TXEMPTY);

		COMMTIMEOUTS Timeout;
		GetCommTimeouts(m_HCOM, &Timeout);
		Timeout.ReadIntervalTimeout = MAXDWORD;
		Timeout.ReadTotalTimeoutMultiplier = 0;
		Timeout.ReadTotalTimeoutConstant = 100;
		Timeout.WriteTotalTimeoutMultiplier = 0;
		Timeout.WriteTotalTimeoutConstant = 1000;
		SetCommTimeouts(m_HCOM, &Timeout);

		SetupComm(m_HCOM, 1000, 1000);

		comState = 0;
		LUA->PushNumber(comState);
	}

	LUA->PushNumber(comState);
	return 1;
}

int StopCOM(lua_State* state)
{
	if (!((m_HCOM == INVALID_HANDLE_VALUE) || (m_HCOM == NULL)))
	{
		PurgeComm(m_HCOM, PURGE_TXCLEAR | PURGE_RXCLEAR);
		CloseHandle(m_HCOM);
		m_HCOM = NULL;
		comState = -10;
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
			PushCLua(Version, "Version"); // string UART.Version()
			PushCLua(StartCOM, "StartCOM"); // int UART.StartCOM(port)
			PushCLua(StopCOM, "StopCOM"); // int UART.StopCOM()
			PushCLua(WriteByte, "WriteByte"); // UART.WriteByte(table Bytes, int nBytes)
			PushCLua(ReadByte, "ReadByte"); // string UART.ReadByte(int bytesToRead)
			PushCLua(GetCOMState, "GetCOMState"); // int UART.GetCOMState()
		LUA->SetField(-2, "UART");
	LUA->Pop();

	return 0;
}
// Called when the module closes
GMOD_MODULE_CLOSE()
{
	PurgeComm(m_HCOM, PURGE_TXCLEAR | PURGE_RXCLEAR);
	CloseHandle(m_HCOM);
	m_HCOM = NULL;
	return 0;
}