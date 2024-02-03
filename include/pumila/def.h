#pragma once
#ifdef _MSC_VER
#ifdef pumila_EXPORTS
#define PUMILA_DLL __declspec(dllexport)
#else
#define PUMILA_DLL __declspec(dllimport)
#endif
#else
#define PUMILA_DLL
#endif

