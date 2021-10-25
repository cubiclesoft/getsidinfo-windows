Get Windows SID Information Command-Line Utility
================================================

Dumps information about Windows Security Identifiers (SIDs) as JSON.  Released under a MIT or LGPL license.

Learn about SIDs, this tool, and much more:

[![Windows Security Objects:  A Crash Course + A Brand New Way to Start Processes on Microsoft Windows video](https://user-images.githubusercontent.com/1432111/118288197-0574ec00-b489-11eb-96e5-fab0f6149171.png)](https://www.youtube.com/watch?v=pmteqkbBfAY "Windows Security Objects:  A Crash Course + A Brand New Way to Start Processes on Microsoft Windows")

[![Donate](https://cubiclesoft.com/res/donate-shield.png)](https://cubiclesoft.com/donate/) [![Discord](https://img.shields.io/discord/777282089980526602?label=chat&logo=discord)](https://cubiclesoft.com/product-support/github/)

Features
--------

* Command-line action!
* Dumps the results of the LookupAccountSid()/LookupAccountName() and NetUserGetInfo() Windows APIs as JSON.  Easily consumed by most programming and scripting languages.
* Pre-built binaries using Visual Studio (statically linked C++ runtime, minimal file size of ~112K, direct Win32 API calls).
* Windows subsystem variant.
* Unicode support.
* Has a liberal open source license.  MIT or LGPL, your choice.
* Sits on GitHub for all of that pull request and issue tracker goodness to easily submit changes and ideas respectively.

Useful Information
------------------

Running the command with the `/?` option will display the options:

```
(C) 2021 CubicleSoft.  All Rights Reserved.

Syntax:  getsidinfo.exe [options] SIDorAcct [SID2orAcct] [SID3orAcct] ...

Options:
        /v
        Verbose mode.

        /system=SystemName
                Retrieve information from the specified system.

        /file=OutputFile
                File to write the JSON output to instead of stdout.
```

Example usage:

```
C:\>wmic useraccount get name,sid
Name                SID
Administrator       S-1-5-21-1304824241-3403877634-2989090281-500
DefaultAccount      S-1-5-21-1304824241-3403877634-2989090281-503
Guest               S-1-5-21-1304824241-3403877634-2989090281-501
T2                  S-1-5-21-1304824241-3403877634-2989090281-1003
WDAGUtilityAccount  S-1-5-21-1304824241-3403877634-2989090281-504

C:\>getsidinfo S-1-5-21-1304824241-3403877634-2989090281-1003 S-1-5-32-544
{"S-1-5-21-1304824241-3403877634-2989090281-1003": {"success": true, "sid": "S-1-5-21-1304824241-3403877634-2989090281-1003", "domain": "MY-PC", "account": "T2", "type": 1, "net_info": {"full_name": "", "comment": "", "user_comment": "", "password_age": 5855194, "password_expired": 0, "bad_passwords": 0, "num_logons": 4, "priv_level": 1, "flags": 66081, "auth_flags": 0, "home_dir": "", "home_dir_drive": "", "profile": "", "script_path": "", "params": "", "workstations": "", "last_logon": 1612641291, "last_logoff": 0, "acct_expires": -1, "disk_quota": -1, "units_per_week": 168, "logon_hours": "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", "logon_server": "\\\\*", "country_code": 0, "code_page": 0, "primary_group_id": 513, "internet_identity": false, "reg_profile_path": "C:\\Users\\T2"}},

"S-1-5-32-544": {"success": true, "sid": "S-1-5-32-544", "domain": "BUILTIN", "account": "Administrators", "type": 4}}

C:\>getsidinfo MY-PC\Guest
{"MY-PC\\Guest": {"success": true, "sid": "S-1-5-21-1304824241-3403877634-2989090281-501", "domain": "MY-PC", "account": "Guest", "type": 1, "net_info": {"full_name": "", "comment": "Built-in account for guest access to the computerdomain", "user_comment": "", "password_age": 0, "password_expired": 0, "bad_passwords": 0, "num_logons": 0, "priv_level": 0, "flags": 66147, "auth_flags": 0, "home_dir": "", "home_dir_drive": "", "profile": "", "script_path": "", "params": "", "workstations": "", "last_logon": 0, "last_logoff": 0, "acct_expires": -1, "disk_quota": -1, "units_per_week": 168, "logon_hours": "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", "logon_server": "\\\\*", "country_code": 0, "code_page": 0, "primary_group_id": 513, "internet_identity": false}}}
```

The second command dumps information about two SIDs - one user account and the BUILTIN\Administrators group.  The third command dumps information using an account name instead of SID.

Windows Subsystem Variant
-------------------------

While `getsidinfo.exe` is intended for use with console apps, `getsidinfo-win.exe` is intended for detached console and GUI applications.  Starting `getsidinfo.exe` in certain situations will briefly flash a console window before displaying the error message.  Calling `getsidinfo-win.exe` instead will no longer show the console window.

There is one additional option specifically for `messagebox-win.exe` called `/attach` which attempts to attach to the console of the parent process (if any).

More Information
----------------

The Windows registry also contains several SIDs not available using the 'wmic' tool and associated user profile paths on the local system:

```
C:\>reg query "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList" /s /t REG_EXPAND_SZ

HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList\S-1-5-18
    ProfileImagePath    REG_EXPAND_SZ    %systemroot%\system32\config\systemprofile

HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList\S-1-5-19
    ProfileImagePath    REG_EXPAND_SZ    %systemroot%\ServiceProfiles\LocalService

HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList\S-1-5-20
    ProfileImagePath    REG_EXPAND_SZ    %systemroot%\ServiceProfiles\NetworkService

HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList\S-1-5-21-1304824241-3403877634-2989090281-1003
    ProfileImagePath    REG_EXPAND_SZ    C:\Users\T2
```

SIDs, such as BUILTIN\Administrators (S-1-5-32-544), are known as "Well-Known SIDs."  Microsoft publishes [a mostly complete list of Well-Known SIDs](https://docs.microsoft.com/en-us/troubleshoot/windows-server/identity/security-identifiers-in-windows).

Related Tools
-------------

* [GetTokenInformation](https://github.com/cubiclesoft/gettokeninformation-windows) - Dumps information about Windows security tokens (SIDs, privileges, etc) as JSON.
* [CreateProcess](https://github.com/cubiclesoft/createprocess-windows) - A powerful Windows API command-line utility that can start processes with all sorts of options including custom user tokens.
