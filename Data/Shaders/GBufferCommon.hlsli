
float3 PackNormal(float3 normal)
{
    return normal * 0.5f + 0.5f;
}

float3 UnpackNormal(float3 normal)
{
    return normal * 2.0f - 1.0f;
}