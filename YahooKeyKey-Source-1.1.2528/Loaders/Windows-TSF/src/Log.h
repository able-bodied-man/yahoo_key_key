//
// Log.h - tiny diagnostic log for KeyKeyTSF
//
// The TSF text service is loaded into other people's processes and has no UI of
// its own, so when nothing happens there is nothing to look at. This writes a
// bounded log to %LOCALAPPDATA%\KeyKeyTSF\keykeytsf.log and mirrors it to the
// debugger, which is what makes "it does nothing" answerable.
//

#ifndef KEYKEY_LOG_H
#define KEYKEY_LOG_H

#include <string>
#include <windows.h>

void KeyKeyLogInit();
void KeyKeyLog(const wchar_t* format, ...);
std::wstring KeyKeyLogPath();

#endif // KEYKEY_LOG_H
