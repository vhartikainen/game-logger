#include "gamelog.h"

// #define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <psapi.h>
#include <Winbase.h>

#include "common.h"

GameLog::GameLog(Settings *settings) : settings(settings), current(NULL)
{
}

void GameLog::update(int apm)
{
    QDEBUG("[GameLog::update()] called");

    DWORD aProcesses[2048];
    DWORD cbNeeded, cProcesses;
    DWORD temp;

    unsigned int i;
    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ) {
        QDEBUG("[GameLog::update()] failed to enumerate processes");
        return;
    }

    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);
    QDEBUG("[GameLog::update()] found %d processes",cProcesses);

    for ( i = 0; i < cProcesses; i++ ) {
        if( aProcesses[i] != 0 ) {
            TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

            // Get a handle to the process.
            HANDLE hProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, aProcesses[i] );

            // Get the process name.
            if (NULL != hProcess ) {
                PDWORD len = &temp;
                *len = sizeof(szProcessName)/sizeof(TCHAR);
                QueryFullProcessImageNameW(hProcess, 0, szProcessName, len);

                QString processName = QString::fromWCharArray(szProcessName);
                processName = processName.mid(processName.lastIndexOf('\\')+1).toLower();

                if (settings->games.contains(processName)) {
                    QDEBUG("[GameLog::update()] found game %s", qPrintable(processName));

                    bool newSession = true;

                    if (current) {
                        QDEBUG("[GameLog::update()] checking if found game is the same as previous");

                        if (processName == current->game) {
                            // Update current session
                            current->update(apm);
                            newSession = false;
                            QDEBUG("[GameLog::update()] game already running.");

                        } else {
                            QDEBUG("[GameLog::update()] game not running before. closing previous session");

                        }
                    }

                    if (newSession) {
                        current = new Session(processName, settings->games[processName]->completeName);
                        QDEBUG("[GameLog::update()] game %s started",qPrintable(processName));

                    }

                    // Finish checking
                    CloseHandle( hProcess );
                    return;
                }
            }
            CloseHandle( hProcess );
        }
    }

    // No game was found
    delete current;
    current = NULL;
}
