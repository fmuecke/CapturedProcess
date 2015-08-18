#pragma once
#include <Windows.h>
#include <vector>
#include <array>

struct StdPipes
{
	StdPipes()  {}

	~StdPipes()
	{
		CloseWriteHandles();
		CloseReadHandles();
	}

	void CloseReadHandles()
	{
		Close(in_read);
		Close(out_read);
		Close(err_read);
	}

	void CloseWriteHandles()
	{
		Close(in_write);
		Close(out_write);
		Close(err_write);
	}

	bool Create()
	{
		m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		m_sa.bInheritHandle = true;  // pipe handles should be inherited
		m_sa.lpSecurityDescriptor = nullptr;

		if (!::CreatePipe(&out_read, &out_write, &m_sa, 0) ||
			!::CreatePipe(&err_read, &err_write, &m_sa, 0) ||
			!::CreatePipe(&in_read, &in_write, &m_sa, 0))
		{
			return false;
		}

		// read (out/err) and write (in) should not be inheritable
		if (!::SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0) ||
			!::SetHandleInformation(err_read, HANDLE_FLAG_INHERIT, 0) ||
			!::SetHandleInformation(in_write, HANDLE_FLAG_INHERIT, 0))
		{
			return false;
		}

		return true;
	}

	void Close(HANDLE& h)
	{
		if (h != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}
	}

    static bool HasData(HANDLE hPipe)
    {
        std::array<char, 256> buffer;
        DWORD bytesRead = 0;
        DWORD bytesAvailable = 0;
        bool success = ::PeekNamedPipe(hPipe, &buffer[0], static_cast<DWORD>(buffer.size()), &bytesRead, &bytesAvailable, NULL) != 0;
        return bytesAvailable > 0 || bytesRead > 0;
    }

	static bool Read(HANDLE hPipe, std::vector<char>& result)
	{
		std::array<char, 4096> buffer;
		std::vector<char> _result;

		for (;;)
		{
			DWORD bytesRead = 0;
			bool success = ::ReadFile(hPipe, &buffer[0], static_cast<DWORD>(buffer.size()), &bytesRead, NULL) != 0;
			if (!success || bytesRead == 0)
			{
				if (_result.empty())
				{
					//std::cerr << "error reading from pipe: code " << GetLastError() << std::endl;
					return false;
				}

				break;
			}

			_result.insert(std::end(_result), std::begin(buffer), std::begin(buffer) + bytesRead);
		}

		result.swap(_result);

		return true;
	}

	StdPipes(const StdPipes&) = delete;
	StdPipes& operator=(const StdPipes&) = delete;

	HANDLE err_read = INVALID_HANDLE_VALUE;
	HANDLE err_write = INVALID_HANDLE_VALUE;
	HANDLE out_read = INVALID_HANDLE_VALUE;
	HANDLE out_write = INVALID_HANDLE_VALUE;
	HANDLE in_read = INVALID_HANDLE_VALUE;
	HANDLE in_write = INVALID_HANDLE_VALUE;

private:
	SECURITY_ATTRIBUTES m_sa;
};