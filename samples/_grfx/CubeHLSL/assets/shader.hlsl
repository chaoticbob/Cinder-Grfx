/* 
Compile:
    dxc -spirv -fspv-reflect -T vs_6_0 -E vsmain -Fo shader.vert.spv shader.hlsl
    dxc -spirv -fspv-reflect -T ps_6_0 -E psmain -Fo shader.frag.spv shader.hlsl
*/

float4x4	ciModelViewProjection;
float3x3	ciNormalMatrix;

[[vk::combinedImageSampler]] Texture2D<float4> uTex0     : register(t2);
[[vk::combinedImageSampler]] SamplerState      uSampler0 : register(s2);

struct VSOutput {
    float4  Position : SV_POSITION;
    float2  TexCoord : TEXCOORD;
    float4	Color    : COLOR;
    float3	Normal   : NORMAL;
};

VSOutput vsmain(
    float4		ciPosition  : POSTIION,
    float2		ciTexCoord0 : TEXCOORD,
    float3		ciNormal    : NORMAL,
    float4		ciColor     : COLOR
)
{
    VSOutput output = (VSOutput)0;
	output.Position =  mul(ciModelViewProjection, ciPosition);
	output.Color    = ciColor;
	output.TexCoord = ciTexCoord0;
	output.Normal   = mul(ciNormalMatrix, ciNormal);
    return output;
}

float4 psmain( VSOutput input ) : SV_TARGET
{
	float3 normal   = normalize( -input.Normal );
	float  diffuse  = max( dot( normal, float3( 0, 0, -1 ) ), 0 );
    float4 oColor   = uTex0.Sample( uSampler0, input.TexCoord ) * input.Color * diffuse;
	return oColor;
}