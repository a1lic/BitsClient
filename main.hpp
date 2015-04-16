#ifndef MAIN_HPP
#define MAIN_HPP
#pragma once

#if !defined(EXTERN_C)
#if defined(__cplusplus)
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif
#endif

#include <windows.h>

union result_t
{
	void *c_pvoid;
	const void *c_pcvoid;

	bool c_bool;
	bool *c_pbool;
	const bool *c_pcbool;

	signed char c_char;
	signed char *c_pcchar;
	const signed char *c_ccharptr;

	unsigned char c_uchar;
	unsigned char *c_puchar;
	const unsigned char *c_pcuchar;

	signed int c_int;
	signed int *c_pint;
	const signed int *c_pcint;

	unsigned int c_uint;
	unsigned int *c_puint;
	const unsigned int *c_pcuint;

	signed long c_long;
	signed long *c_plong;
	const signed long *c_pclong;

	unsigned long c_ulong;
	unsigned long *c_pulong;
	const unsigned long *c_pculong;

	signed long long c_llong;
	signed long long *c_pllong;
	const signed long long *c_pcllong;

	unsigned long long c_ullong;
	unsigned long long *c_pullong;
	const unsigned long long *c_pcullong;

	float c_float;
	float *c_pfloat;
	const float *c_pcfloat;

	double c_double;
	double *c_pdouble;
	const double *c_pcdouble;
};
typedef union result_t result_t;

union winresult_t
	BOOL w_bool;
	BOOL *w_boolptr;
	const BOOL *w_cboolptr;
	BOOLEAN w_boolean;
	BOOLEAN *w_booleanptr;
	const BOOLEAN *w_cbooleanptr;
	BYTE w_byte;
	BYTE *w_byteptr;
	const BYTE *w_cbyteptr;
	SHORT w_short;
	SHORT *w_shortptr;
	const SHORT *w_cshortptr;
	USHORT w_ushort;
	USHORT *w_ushortptr;
	const USHORT *w_cushortptr;
	LONG w_long;
	LONG *w_longptr;
	WORD w_word;
	DWORD w_dword;
	HRESULT w_hresult;
	LRESULT w_lresult;
	INT_PTR w_intptr;
	UINT_PTR w_uintptr;
	LONG_PTR w_longptr;
	ULONG_PTR w_ulongptr;
	DWORD_PTR w_dwordptr;
};
typedef union winresult_t winresult_t;

EXTERN_C void Debugf(PCTSTR, ...);
EXTERN_C int MessageBoxf(HWND,PCTSTR,UINT,PCTSTR, ...);

#endif
