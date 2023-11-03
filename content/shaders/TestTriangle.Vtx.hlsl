
struct vertex_output
{
    float4 f4Position : SV_POSITION;
    float3 f3Color    : COLOR0;
};

struct simple_vertex
{
    float3 f3Position;
};

struct vertex_draw_constants
{
    uint uVertexOffset;
    uint uVertexBufferIndex;
};

#define BufferSpace space0

ByteAddressBuffer                     BufferTable[]        : register(t0, BufferSpace);
ConstantBuffer<vertex_draw_constants> cVertexDrawConstants : register(b0, space0);

simple_vertex ExtractVertex(uint uVertexId, vertex_draw_constants DrawConstants)
{
    simple_vertex Result = BufferTable[DrawConstants.uVertexBufferIndex].Load<simple_vertex>((DrawConstants.uVertexOffset + uVertexId) * sizeof(simple_vertex));
    return Result;
}

vertex_output main(uint uVertexId : SV_VertexID)
{
    simple_vertex Vertex = ExtractVertex(uVertexId, cVertexDrawConstants);
    
    vertex_output Result;
    Result.f4Position = float4(Vertex.f3Position, 1.0f);
    Result.f3Color    = float3(1.0f, 0.0f, 0.0f);
    
    return Result;
}