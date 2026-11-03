// KeyKey 2 text service
// Text-service plumbing is based on the Microsoft Windows-classic-samples "SampleIME".
// See ../../../THIRD_PARTY_NOTICES.md for the Microsoft sample license.

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <msctf.h>
#include <ctffunc.h>
#include <olectl.h>
#include <strsafe.h>
#include <assert.h>
#include <combaseapi.h>

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

#include "resource.h"
