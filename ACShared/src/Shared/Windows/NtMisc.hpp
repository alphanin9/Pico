#pragma once
#include <Shared/Windows/Ntdll.hpp>
namespace Windows
{
typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    const UNICODE_STRING* ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;       // PSECURITY_DESCRIPTOR;
    PVOID SecurityQualityOfService; // PSECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef const OBJECT_ATTRIBUTES* PCOBJECT_ATTRIBUTES;

typedef enum _OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation,         // q: OBJECT_BASIC_INFORMATION
    ObjectNameInformation,          // q: OBJECT_NAME_INFORMATION
    ObjectTypeInformation,          // q: OBJECT_TYPE_INFORMATION
    ObjectTypesInformation,         // q: OBJECT_TYPES_INFORMATION
    ObjectHandleFlagInformation,    // qs: OBJECT_HANDLE_FLAG_INFORMATION
    ObjectSessionInformation,       // s: void // change object session // (requires SeTcbPrivilege)
    ObjectSessionObjectInformation, // s: void // change object session // (requires SeTcbPrivilege)
    MaxObjectInfoClass
} OBJECT_INFORMATION_CLASS;

/**
 * The OBJECT_BASIC_INFORMATION structure contains basic information about an object.
 */
typedef struct _OBJECT_BASIC_INFORMATION
{
    ULONG Attributes; // The attributes of the object include whether the object is permanent, can be inherited, and
                      // other characteristics.
    ACCESS_MASK GrantedAccess; // Specifies a mask that represents the granted access when the object was created.
    ULONG HandleCount;         // The number of handles that are currently open for the object.
    ULONG PointerCount; // The number of references to the object from both handles and other references, such as those
                        // from the system.
    ULONG PagedPoolCharge;        // The amount of paged pool memory that the object is using.
    ULONG NonPagedPoolCharge;     // The amount of non-paged pool memory that the object is using.
    ULONG Reserved[3];            // Reserved for future use.
    ULONG NameInfoSize;           // The size of the name information for the object.
    ULONG TypeInfoSize;           // The size of the type information for the object.
    ULONG SecurityDescriptorSize; // The size of the security descriptor for the object.
    LARGE_INTEGER CreationTime; // The time when a symbolic link was created. Not supported for other types of objects.
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

/**
 * The OBJECT_NAME_INFORMATION structure contains various statistics and properties about an object type.
 */
typedef struct _OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING TypeName;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    UCHAR TypeIndex; // since WINBLUE
    CHAR ReservedByte;
    ULONG PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_TYPES_INFORMATION
{
    ULONG NumberOfTypes;
    OBJECT_TYPE_INFORMATION TypeInfo[1]; // Eternal classic
} OBJECT_TYPES_INFORMATION, *POBJECT_TYPES_INFORMATION;

typedef struct _OBJECT_HANDLE_FLAG_INFORMATION
{
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_FLAG_INFORMATION, *POBJECT_HANDLE_FLAG_INFORMATION;
} // namespace Windows