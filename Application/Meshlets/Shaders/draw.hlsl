#include "../../Common/common_shader.h"

#define MS_ROOT_SIG "CBV(b0), \
                  SRV(t0), \
                  SRV(t1), \
                  SRV(t2), \
                  SRV(t3), \ 
                  SRV(t4)"
struct Meshlet
{
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
};

cbuffer Constants : register(b0)
{
	Camera MainCamera;
}

struct VertexOUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
};

StructuredBuffer<float3> Positions : register(t0);
StructuredBuffer<float3> Normals : register(t1);
StructuredBuffer<Meshlet> Meshlets : register(t2);
StructuredBuffer<uint> MeshletTriangles : register(t3);
ByteAddressBuffer UniqueVertexIB : register(t4);

uint3 UnpackPrimitive(uint primitive)
{
    // Unpacks a 10 bits per index triangle from a 32-bit uint.
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

[RootSignature(MS_ROOT_SIG)]
[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void MS(
    uint groupThreadID : SV_GroupThreadID,
    uint groupID : SV_GroupID,
    out indices uint3 tris[126],
    out vertices VertexOUT verts[64]
)
{
    Meshlet meshlet = Meshlets[groupID];

    SetMeshOutputCounts(meshlet.VertCount, meshlet.PrimCount);

    if (groupThreadID < meshlet.PrimCount)
    {
        tris[groupThreadID] = UnpackPrimitive(MeshletTriangles[meshlet.PrimOffset + groupThreadID]);
    }

    if (groupThreadID < meshlet.VertCount)
    {
        const uint localIndex = meshlet.VertOffset + groupThreadID;
        uint vertexIndex = UniqueVertexIB.Load(localIndex * 4);

        VertexOUT vertex;
        vertex.Position = GetClipPosition(Positions[vertexIndex], MainCamera);
        vertex.Normal = Normals[vertexIndex];
        verts[groupThreadID] = vertex;
    }
}

float4 PS(VertexOUT IN) : SV_Target
{
	return float4(IN.Normal, 1.0f);
}