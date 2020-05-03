#pragma once

#include <FLHook.h>
#include <plugin.h>
#include "PluginUtilities.h"

PLUGIN_RETURNCODE returncode;

void AddExceptionInfoLog(struct SEHException* pep);
#define LOG_EXCEPTION { AddLog("ERROR Exception in %s", __FUNCTION__); AddExceptionInfoLog(0); }