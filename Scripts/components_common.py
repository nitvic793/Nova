import enum

class FieldTypes(enum.IntEnum):
    FIELD_UNDEFINED = 0
    FIELD_FLOAT     = 1
    FIELD_INT       = 2
    FIELD_FLOAT2    = 3
    FIELD_FLOAT3    = 4
    FIELD_FLOAT4    = 5
    FIELD_STRING    = 6
    FIELD_HANDLE_TEX    = 7
    FIELD_HANDLE_MESH   = 8
    FIELD_HANDLE_MAT    = 9