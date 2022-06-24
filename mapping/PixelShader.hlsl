//--------------------------------------------------------------------------------------
// 전역 변수 선언
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D	g_txDiffuse : register (t0);
TextureCube g_txEnvMap	: register (t1);
SamplerState g_samLinear: register (s0);

cbuffer cb0 : register(b0)
{
	matrix	g_matWorld		: packoffset(c0);
	matrix	g_matView		: packoffset(c4);
	matrix	g_matProj		: packoffset(c8);
	float4  g_MeshColor     : packoffset(c12);
};
//-----------------------------------------------------------------------------
//상수 버퍼
////-----------------------------------------------------------------------------
cbuffer cbObjectChangesEveryFrame: register(b1)
{
	// Object
	matrix				g_matNormal;
	// Global
	float4				cb_vLightVector;
	//float g_MeshColor.w	ref_at_norm_incidence = 1.33f;
	float4				cb_vEyePos;
	float4				cb_vEyeDir;
};
//cbuffer cbObjectNeverChanges
//{
//	// Object
//	float4				g_AmbientMaterialColor;
//    float4				g_DiffuseMaterialColor;
//    float4				g_EmissionMaterialColor; 
//    // Global
//    float4				cb_AmbientLightColor;
//    float4				cb_DiffuseLightColor;  
//};
//-----------------------------------------------------------------------------
//정점 및 픽셀 쉐이더 입출력 버퍼
//-----------------------------------------------------------------------------
struct PNCT_VERTEX
{
	float3 p			: POSITION;
	float3 n			: NORMAL;
	float4 c			: COLOR0;
	float2 t			: TEXCOORD0;
};
struct P4T2_PS_INPUT
{
	float4 p			: SV_POSITION;
	float3 v			: TEXCOORD0;
};
struct PCT5_PS_INPUT
{
	float4 p			: SV_POSITION;
	float4 c			: COLOR0;
	float2 t			: TEXCOORD0;
	float3 n			: TEXCOORD1;
	float3 r			: TEXCOORD2;
	float3 f			: TEXCOORD3;
	float3 e			: TEXCOORD4;
};

float3 Reflect(float3 incident, float3 normal)
{
	float cosI = dot(normal, incident);
	return incident - 2 * cosI * normal;
}

float3 Refract(float3 incident, float3 normal, float etaRatio)
{
	float cosI = dot(incident, normal);
	float cosT2 = 1.0f - etaRatio * etaRatio * (1.0f - cosI * cosI);
	float3 T = etaRatio * incident + ((etaRatio * cosI - sqrt(abs(cosT2))) * normal);
	return T;
}

float ComputeFresnel(float3 vReflect, float3 vNormal, float F0)
{
	float cosAngle = 1 - saturate(dot(vReflect, vNormal));
	float result = pow(cosAngle, 5.0f);
	result = saturate(mad(result, 1 - saturate(F0), F0));
	return result;
}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PCT5_PS_INPUT VS(PNCT_VERTEX input)
{
	PCT5_PS_INPUT output = (PCT5_PS_INPUT)0;
	const float ref_at_norm_incidence = 1.33f;

	float4 vWorldPos = mul(float4(input.p, 1.0f), g_matWorld);
	float4 vViewPos = mul(vWorldPos, g_matView);
	output.p = mul(vViewPos, g_matProj);
	output.n = normalize(mul(input.n, (float3x3)g_matNormal));

	float fDot = dot(-cb_vLightVector.xyz, output.n);
	output.c = float4(fDot, fDot, fDot, 1.0f);
	output.t = input.t;

	//  camera/eye -> V    
	float3 Incident = normalize(vWorldPos.xyz - cb_vEyePos.xyz);
	output.e = Incident;
	//  R = I - 2*N * (I.N)
	//output.r		= normalize(Incident - 2 * output.n * dot(Incident, output.n));
	output.r = normalize(reflect(Incident, output.n));
	output.f = normalize(refract(Incident, output.n, 1 / ref_at_norm_incidence));
	return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PCT5_PS_INPUT input) : SV_Target
{
	float4 DiffuseColor = g_txDiffuse.Sample(g_samLinear, input.t);
	float4 ReflectedColor = g_txEnvMap.Sample(g_samLinear, input.r);
	return lerp(DiffuseColor, ReflectedColor, 1);
}
float4 FRESNEL_PS(PCT5_PS_INPUT input) : SV_Target
{
	const float ref_at_norm_incidence = 1.33f;
	float4 DiffuseColor = g_txDiffuse.Sample(g_samLinear, input.t);
	float4 ReflectedColor = g_txEnvMap.Sample(g_samLinear, input.r);

	float R0 = pow(1.0 - ref_at_norm_incidence, 2.0) / pow(1.0 + ref_at_norm_incidence, 2.0);
	float fresnel = ComputeFresnel(input.r, input.n, R0);

	// 환경맵과 디퓨fl즈맵의 보간
	float4 color = lerp(DiffuseColor, ReflectedColor, fresnel);
	color.a = 1.0f;
	return color;
}
float4 REFRACTION_PS(PCT5_PS_INPUT input) : SV_Target
{
	const float ref_at_norm_incidence = 1.33f;
	float4 DiffuseColor = g_txDiffuse.Sample(g_samLinear, input.t);
	float4 ReflectedColor = g_txEnvMap.Sample(g_samLinear, input.r);
	float4 RefractedColor = g_txEnvMap.Sample(g_samLinear, input.f);

	float R0 = pow(1.0 - ref_at_norm_incidence, 2.0) / pow(1.0 + ref_at_norm_incidence, 2.0);
	float fresnel = ComputeFresnel(input.r, input.n, R0);

	float4 color = lerp(RefractedColor, ReflectedColor,fresnel);
	color.a = 1.0f;
	return color;
}
