#pragma once
/* staticライブラリにしたら要らない
#ifdef _MSC_VER
#ifdef pumila_EXPORTS
#define PUMILA_DLL __declspec(dllexport)
#else
#define PUMILA_DLL __declspec(dllimport)
#endif
#ifdef _DEBUG
#define PUMILA_NS pumilad
#else
#define PUMILA_NS pumila
#endif
#else
*/
#define PUMILA_DLL
#define PUMILA_NS pumila
// #endif

