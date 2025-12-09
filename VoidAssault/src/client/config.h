// client/config.h
#pragma once

// Общие настройки для клиента
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif