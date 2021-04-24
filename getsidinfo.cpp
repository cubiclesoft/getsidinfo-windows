// A simple program to dump information about SIDs as consumable JSON.
//
// (C) 2021 CubicleSoft.  All Rights Reserved.

#define UNICODE
#define _UNICODE
#define _CRT_SECURE_NO_WARNINGS

#ifdef _MBCS
#undef _MBCS
#endif

#include <cstdio>
#include <cstdlib>
#include <windows.h>
#include <lm.h>
#include <sddl.h>
#include <tchar.h>

#include "utf8/utf8_util.h"
#include "utf8/utf8_file_dir.h"
#include "utf8/utf8_mixed_var.h"
#include "json/json_serializer.h"

#ifdef SUBSYSTEM_WINDOWS
// If the caller is a console application and is waiting for this application to complete, then attach to the console.
void InitVerboseMode(void)
{
	if (::AttachConsole(ATTACH_PARENT_PROCESS))
	{
		if (::GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
		{
			freopen("CONOUT$", "w", stdout);
			setvbuf(stdout, NULL, _IONBF, 0);
		}

		if (::GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
		{
			freopen("CONOUT$", "w", stderr);
			setvbuf(stderr, NULL, _IONBF, 0);
		}
	}
}
#endif

void DumpSyntax(TCHAR *currfile)
{
#ifdef SUBSYSTEM_WINDOWS
	InitVerboseMode();
#endif

	_tprintf(_T("(C) 2021 CubicleSoft.  All Rights Reserved.\n\n"));

	_tprintf(_T("Syntax:  %s [options] SID [SID2] [SID3] ...\n\n"), currfile);

	_tprintf(_T("Options:\n"));

	_tprintf(_T("\t/v\n\
\tVerbose mode.\n\
\n\
\t/system=SystemName\n\
\tRetrieve information from the specified system.\n\
\n\
\t/file=OutputFile\n\
\tFile to write the JSON output to instead of stdout.\n\n"));

#ifdef SUBSYSTEM_WINDOWS
	_tprintf(_T("\t/attach\n"));
	_tprintf(_T("\tAttempt to attach to a parent console if it exists.\n\n"));
#endif
}


void DumpOutput(CubicleSoft::UTF8::File &OutputFile, CubicleSoft::JSON::Serializer &OutputJSON)
{
	size_t y;

	if (OutputFile.IsOpen())  OutputFile.Write((std::uint8_t *)OutputJSON.GetBuffer(), OutputJSON.GetCurrPos(), y);
	else  printf("%s", OutputJSON.GetBuffer());

	OutputJSON.ResetPos();
}

void DumpWinError(CubicleSoft::JSON::Serializer &OutputJSON, DWORD winerror)
{
	LPTSTR errmsg = NULL;
	CubicleSoft::UTF8::UTF8MixedVar<char[8192]> TempVar;

	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, winerror, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errmsg, 0, NULL);

	if (errmsg == NULL)  OutputJSON.AppendStr("winerror", "Unknown Windows error message.");
	else
	{
		TempVar.SetUTF8(errmsg);
		OutputJSON.AppendStr("winerror", TempVar.GetStr());

		::LocalFree(errmsg);
	}
}

void DumpErrorMsg(CubicleSoft::UTF8::File &OutputFile, CubicleSoft::JSON::Serializer &OutputJSON, const char *errorstr, const char *errorcode, DWORD winerror)
{
	OutputJSON.AppendBool("success", false);
	OutputJSON.AppendStr("error", errorstr);
	OutputJSON.AppendStr("errorcode", errorcode);
	DumpWinError(OutputJSON, winerror);
	OutputJSON.AppendUInt("winerrorcode", winerror);

	DumpOutput(OutputFile, OutputJSON);
}

struct SidInfo {
	PSID MxSid;
	TCHAR MxDomainName[1024];
	TCHAR MxAccountName[1024];
	SID_NAME_USE MxSidType;
};

int _tmain(int argc, TCHAR **argv)
{
	bool verbose = false;
	LPTSTR systemname = NULL;
	LPTSTR filename = NULL;
	const char *errorstr = NULL, *errorcode = NULL;
	DWORD winerror;
	int result = 1;

	// Process command-line options.
	int x;
	for (x = 1; x < argc; x++)
	{
		if (!_tcsicmp(argv[x], _T("/v")))  verbose = true;
		else if (!_tcsicmp(argv[x], _T("/?")) || !_tcsicmp(argv[x], _T("/h")))
		{
			DumpSyntax(argv[0]);

			return 0;
		}
		else if (!_tcsncicmp(argv[x], _T("/system="), 8))  systemname = argv[x] + 8;
		else if (!_tcsncicmp(argv[x], _T("/file="), 6))  filename = argv[x] + 6;
		else if (!_tcsicmp(argv[x], _T("/attach")))
		{
#ifdef SUBSYSTEM_WINDOWS
			// For the Windows subsystem only, attempt to attach to a parent console if it exists.
			InitVerboseMode();
#endif
		}
		else
		{
			// Probably reached the command to execute portion of the arguments.
			break;
		}
	}

	if (x >= argc)
	{
		DumpSyntax(argv[0]);

		return 0;
	}

	if (verbose)
	{
#ifdef SUBSYSTEM_WINDOWS
		InitVerboseMode();
#endif

		_tprintf(_T("Arguments:\n"));
		for (int x2 = 0; x2 < argc; x2++)
		{
			_tprintf(_T("\targv[%d] = %s\n"), x2, argv[x2]);
		}
		_tprintf(_T("\n"));
	}

	// Handle output to a file.
	CubicleSoft::UTF8::UTF8MixedVar<char[8192]> TempVar;
	CubicleSoft::UTF8::File OutputFile;
	size_t y;
	if (filename != NULL)
	{
		TempVar.SetUTF8(filename);
		if (!OutputFile.Open(TempVar.GetStr(), O_CREAT | O_WRONLY | O_TRUNC))
		{
#ifdef SUBSYSTEM_WINDOWS
			InitVerboseMode();
#endif

			_tprintf(_T("Unable to open '%s' for writing.\n"), filename);

			return 0;
		}
	}


	// Process SIDs.
	char outputbuffer[4096];
	CubicleSoft::JSON::Serializer OutputJSON;

	OutputJSON.SetBuffer((std::uint8_t *)outputbuffer, sizeof(outputbuffer));
	OutputJSON.StartObject();

	SidInfo TempSidInfo;
	DWORD acctbuffersize, domainbuffersize;
	NET_API_STATUS netstatus;
	LPUSER_INFO_4 userinfo4;
	LPUSER_INFO_24 userinfo24;
	int x2;

	for (; x < argc; x++)
	{
		OutputJSON.SetValSplitter(",\n\n");
		TempVar.SetUTF8(argv[x]);
		OutputJSON.StartObject(TempVar.GetStr());
		OutputJSON.SetValSplitter(", ");

		if (!::ConvertStringSidToSid(argv[x], &TempSidInfo.MxSid))  DumpErrorMsg(OutputFile, OutputJSON, "Unable to convert string to a SID.", "conversion_failed", ::GetLastError());
		else
		{
			acctbuffersize = sizeof(TempSidInfo.MxAccountName) / sizeof(TCHAR);
			domainbuffersize = sizeof(TempSidInfo.MxDomainName) / sizeof(TCHAR);

			if (!::LookupAccountSid(systemname, TempSidInfo.MxSid, TempSidInfo.MxAccountName, &acctbuffersize, TempSidInfo.MxDomainName, &domainbuffersize, &TempSidInfo.MxSidType))  DumpErrorMsg(OutputFile, OutputJSON, "Unable to get SID information.", "lookup_account_sid_failed", ::GetLastError());
			else
			{
				OutputJSON.AppendBool("success", true);

				TempVar.SetUTF8(TempSidInfo.MxDomainName);
				OutputJSON.AppendStr("domain", TempVar.GetStr());

				TempVar.SetUTF8(TempSidInfo.MxAccountName);
				OutputJSON.AppendStr("account", TempVar.GetStr());

				OutputJSON.AppendUInt("type", TempSidInfo.MxSidType);

				if (TempSidInfo.MxSidType == SidTypeUser || TempSidInfo.MxSidType == SidTypeDeletedAccount)
				{
					OutputJSON.StartObject("net_info");

					userinfo4 = NULL;
					netstatus = ::NetUserGetInfo(TempSidInfo.MxDomainName, TempSidInfo.MxAccountName, 4, (LPBYTE *)&userinfo4);
					if (netstatus != NERR_Success || userinfo4 == NULL)  DumpErrorMsg(OutputFile, OutputJSON, "Unable to get network user info.", "net_user_get_info_failed", netstatus);
					else
					{
						TempVar.SetUTF8(userinfo4->usri4_full_name);
						OutputJSON.AppendStr("full_name", TempVar.GetStr());

						TempVar.SetUTF8(userinfo4->usri4_comment);
						OutputJSON.AppendStr("comment", TempVar.GetStr());

						TempVar.SetUTF8(userinfo4->usri4_usr_comment);
						OutputJSON.AppendStr("user_comment", TempVar.GetStr());

						OutputJSON.AppendUInt("password_age", userinfo4->usri4_password_age);
						OutputJSON.AppendUInt("password_expired", userinfo4->usri4_password_expired);
						OutputJSON.AppendInt("bad_passwords", (std::int32_t)userinfo4->usri4_bad_pw_count);
						OutputJSON.AppendInt("num_logons", (std::int32_t)userinfo4->usri4_num_logons);
						OutputJSON.AppendUInt("priv_level", userinfo4->usri4_priv);

						OutputJSON.AppendUInt("flags", userinfo4->usri4_flags);

						OutputJSON.AppendUInt("auth_flags", userinfo4->usri4_auth_flags);

						TempVar.SetUTF8(userinfo4->usri4_home_dir);
						OutputJSON.AppendStr("home_dir", TempVar.GetStr());

						TempVar.SetUTF8(userinfo4->usri4_home_dir_drive);
						OutputJSON.AppendStr("home_dir_drive", TempVar.GetStr());

						TempVar.SetUTF8(userinfo4->usri4_profile);
						OutputJSON.AppendStr("profile", TempVar.GetStr());

						TempVar.SetUTF8(userinfo4->usri4_script_path);
						OutputJSON.AppendStr("script_path", TempVar.GetStr());

						TempVar.SetUTF8(userinfo4->usri4_parms);
						OutputJSON.AppendStr("params", TempVar.GetStr());

						TempVar.SetUTF8(userinfo4->usri4_workstations);
						OutputJSON.AppendStr("workstations", TempVar.GetStr());

						OutputJSON.AppendUInt("last_logon", userinfo4->usri4_last_logon);
						OutputJSON.AppendUInt("last_logoff", userinfo4->usri4_last_logoff);
						OutputJSON.AppendInt("acct_expires", (std::int32_t)userinfo4->usri4_acct_expires);
						OutputJSON.AppendInt("disk_quota", (std::int32_t)userinfo4->usri4_max_storage);
						OutputJSON.AppendUInt("units_per_week", userinfo4->usri4_units_per_week);

						TempVar.SetSize(0);
						for (x2 = 0; x2 < 21; x2++)
						{
							if (userinfo4->usri4_logon_hours[x2] < 16)  TempVar.AppendChar('0');
							TempVar.AppendUInt(userinfo4->usri4_logon_hours[x2], 16);
						}
						OutputJSON.AppendStr("logon_hours", TempVar.GetStr());

						TempVar.SetUTF8(userinfo4->usri4_logon_server);
						OutputJSON.AppendStr("logon_server", TempVar.GetStr());

						OutputJSON.AppendUInt("country_code", userinfo4->usri4_country_code);
						OutputJSON.AppendUInt("code_page", userinfo4->usri4_code_page);
						OutputJSON.AppendUInt("primary_group_id", userinfo4->usri4_primary_group_id);

						userinfo24 = NULL;
						netstatus = ::NetUserGetInfo(TempSidInfo.MxDomainName, TempSidInfo.MxAccountName, 24, (LPBYTE *)&userinfo24);
						if (netstatus != NERR_Success || userinfo24 == NULL)  OutputJSON.AppendBool("internet_identity", false);
						else
						{
							OutputJSON.AppendBool("internet_identity", (userinfo24->usri24_internet_identity ? true : false));

							if (userinfo24->usri24_internet_identity)
							{
								TempVar.SetUTF8(userinfo24->usri24_internet_provider_name);
								OutputJSON.AppendStr("internet_provider", TempVar.GetStr());

								TempVar.SetUTF8(userinfo24->usri24_internet_principal_name);
								OutputJSON.AppendStr("internet_principal", TempVar.GetStr());
							}
						}

						if (userinfo24 != NULL)  ::NetApiBufferFree(userinfo24);

						DumpOutput(OutputFile, OutputJSON);
					}

					if (userinfo4 != NULL)  ::NetApiBufferFree(userinfo4);

					OutputJSON.EndObject();
				}
			}

			::LocalFree(TempSidInfo.MxSid);
		}

		DumpOutput(OutputFile, OutputJSON);

		OutputJSON.EndObject();
	}

	OutputJSON.EndObject();
	OutputJSON.Finish();

	DumpOutput(OutputFile, OutputJSON);

	if (!OutputFile.IsOpen())  printf("\n");
	else  OutputFile.Write("\n", y);

	OutputFile.Close();

	// Let the OS clean up after this program.  It is lazy, but whatever.
	if (verbose)  _tprintf(_T("Return code = %i\n"), result);

	return result;
}

#ifdef SUBSYSTEM_WINDOWS
#ifndef UNICODE
// Swiped from:  https://stackoverflow.com/questions/291424/canonical-way-to-parse-the-command-line-into-arguments-in-plain-c-windows-api
LPSTR* CommandLineToArgvA(LPSTR lpCmdLine, INT *pNumArgs)
{
	int retval;
	retval = ::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, NULL, 0);
	if (!SUCCEEDED(retval))  return NULL;

	LPWSTR lpWideCharStr = (LPWSTR)malloc(retval * sizeof(WCHAR));
	if (lpWideCharStr == NULL)  return NULL;

	retval = ::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, lpWideCharStr, retval);
	if (!SUCCEEDED(retval))
	{
		free(lpWideCharStr);

		return NULL;
	}

	int numArgs;
	LPWSTR* args;
	args = ::CommandLineToArgvW(lpWideCharStr, &numArgs);
	free(lpWideCharStr);
	if (args == NULL)  return NULL;

	int storage = numArgs * sizeof(LPSTR);
	for (int i = 0; i < numArgs; i++)
	{
		BOOL lpUsedDefaultChar = FALSE;
		retval = ::WideCharToMultiByte(CP_ACP, 0, args[i], -1, NULL, 0, NULL, &lpUsedDefaultChar);
		if (!SUCCEEDED(retval))
		{
			::LocalFree(args);

			return NULL;
		}

		storage += retval;
	}

	LPSTR* result = (LPSTR *)::LocalAlloc(LMEM_FIXED, storage);
	if (result == NULL)
	{
		::LocalFree(args);

		return NULL;
	}

	int bufLen = storage - numArgs * sizeof(LPSTR);
	LPSTR buffer = ((LPSTR)result) + numArgs * sizeof(LPSTR);
	for (int i = 0; i < numArgs; ++ i)
	{
		BOOL lpUsedDefaultChar = FALSE;
		retval = ::WideCharToMultiByte(CP_ACP, 0, args[i], -1, buffer, bufLen, NULL, &lpUsedDefaultChar);
		if (!SUCCEEDED(retval))
		{
			::LocalFree(result);
			::LocalFree(args);

			return NULL;
		}

		result[i] = buffer;
		buffer += retval;
		bufLen -= retval;
	}

	::LocalFree(args);

	*pNumArgs = numArgs;
	return result;
}
#endif

int CALLBACK WinMain(HINSTANCE /* hInstance */, HINSTANCE /* hPrevInstance */, LPSTR lpCmdLine, int /* nCmdShow */)
{
	int argc;
	TCHAR **argv;
	int result;

#ifdef UNICODE
	argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
#else
	argv = CommandLineToArgvA(lpCmdLine, &argc);
#endif

	if (argv == NULL)  return 0;

	result = _tmain(argc, argv);

	::LocalFree(argv);

	return result;
}
#endif
