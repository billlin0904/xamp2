// Most of this is from http://workblog.pilin.name/2014/03/decoding-parameters-of-thrown-c.html
// The PMD structure and decoding logc is from http://www.openrce.org/articles/full_view/23

// I'm wrapping in my namespace to avoid unnecessary namespace pollution.

namespace stdx::detail {

#define _EH_PTR64

#ifdef _WIN64
#define _EH_RELATIVE_OFFSETS 1
#endif

struct PMD
{
	int mdisp;  //member displacement
	int pdisp;  //vbtable displacement
	int vdisp;  //displacement inside vbtable
};

void* adjustThis(const PMD& pmd, void* pThisRaw) {
	auto pThis = (char*)pThisRaw;
	pThis += pmd.mdisp;
	if (pmd.pdisp != -1)
	{
		const char *vbtable = pThis + pmd.pdisp;
		pThis += *(const int*)(vbtable + pmd.vdisp);
	}
	return pThis;
}


using PMFN = __int32;

typedef struct TypeDescriptor
{
#if defined(_WIN64) || defined(_RTTI) /*IFSTRIP=IGN*/
	const void * _EH_PTR64 pVFTable; // Field overloaded by RTTI
#else
	DWORD hash;   // Hash value computed from type's decorated name
#endif
	void * _EH_PTR64   spare;  // reserved, possible for RTTI
	char name[];   // The decorated name of the type; 0 terminated.
} TypeDescriptor;

typedef const struct _s_CatchableType {
	unsigned int properties;    // Catchable Type properties (Bit field)
#if _EH_RELATIVE_OFFSETS && !defined(VERSP_IA64) && !defined(_M_CEE_PURE) /*IFSTRIP=IGN*/
	__int32   pType;     // Image relative offset of TypeDescriptor
#else
	TypeDescriptor * _EH_PTR64 pType;  // Pointer to the type descriptor for this type
#endif
	PMD    thisDisplacement;  // Pointer to instance of catch type within
							  //  thrown object.
	int    sizeOrOffset;   // Size of simple-type object or offset into
						   //  buffer of 'this' pointer for catch object
	PMFN   copyFunction;   // Copy constructor or CC-closure
} CatchableType;

typedef const struct _s_CatchableTypeArray {
	int nCatchableTypes;
#if _EH_RELATIVE_OFFSETS && !defined(VERSP_IA64) && !defined(_M_CEE_PURE) /*IFSTRIP=IGN*/
	__int32   arrayOfCatchableTypes[]; // Image relative offset of Catchable Types
#else
	CatchableType * _EH_PTR64 arrayOfCatchableTypes[];
#endif
} CatchableTypeArray;

typedef const struct _s_ThrowInfo {
	unsigned int attributes;   // Throw Info attributes (Bit field)
	PMFN   pmfnUnwind;   // Destructor to call when exception
						 // has been handled or aborted.
#if _EH_RELATIVE_OFFSETS && !defined(VERSP_IA64) && !defined(_M_CEE_PURE) /*IFSTRIP=IGN*/
	__int32   pForwardCompat;  // Image relative offset of Forward compatibility frame handler
	__int32   pCatchableTypeArray;// Image relative offset of CatchableTypeArray
#else
#if defined(__cplusplus)
	int(__cdecl* _EH_PTR64 pForwardCompat)(...); // Forward compatibility frame handler
#else
	int(__cdecl* _EH_PTR64 pForwardCompat)(); // Forward compatibility frame handler
#endif
	CatchableTypeArray * _EH_PTR64 pCatchableTypeArray; // Pointer to list of pointers to types.
#endif
} ThrowInfo;

typedef struct EHExceptionRecord {
	DWORD  ExceptionCode;   // The code of this exception. (= EH_EXCEPTION_NUMBER)
	DWORD  ExceptionFlags;   // Flags determined by NT
	struct _EXCEPTION_RECORD *ExceptionRecord; // An extra exception record (not used)
	void *   ExceptionAddress;  // Address at which exception occurred
	DWORD   NumberParameters;  // Number of extended parameters. (= EH_EXCEPTION_PARAMETERS)
	struct EHParameters {
		DWORD  magicNumber;  // = EH_MAGIC_NUMBER1
		void *  pExceptionObject; // Pointer to the actual object thrown
		ThrowInfo *pThrowInfo;  // Description of thrown object
#if _EH_RELATIVE_OFFSETS
		void  *pThrowImageBase; // Image base of thrown object
#endif
	} params;
} EHExceptionRecord;

}
