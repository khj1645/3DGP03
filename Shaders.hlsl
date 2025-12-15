struct MATERIAL
{
	float4					m_cAmbient;
	float4					m_cDiffuse;
	float4					m_cSpecular; //a = power
	float4					m_cEmissive;
};

// Shadow Map Resources
Texture2D<float>          gShadowMap          : register(t31);
SamplerComparisonState    gssShadow           : register(s2);

cbuffer cbShadow          : register(b9)
{
    matrix                  gmtxShadowTransform; // World -> Light Clip space -> Texture space
    float                   gShadowBias;
	float					gShadowMapSize; // Added for dynamic texel size calculation
	float2					gShadowPadding; // Padding to align
};

cbuffer cbCameraInfo : register(b1)
{
	matrix					gmtxView : packoffset(c0);
	matrix					gmtxProjection : packoffset(c4);
	matrix					gmtxInverseView : packoffset(c8);
	float3					gvCameraPosition : packoffset(c12);
};

cbuffer cbGameObjectInfo : register(b2)
{
	matrix					gmtxGameObject : packoffset(c0);
	MATERIAL				gMaterial : packoffset(c4);
	uint					gnTexturesMask : packoffset(c8);
};

cbuffer cbWaterInfo : register(b3)
{
	matrix		gf4x4TextureAnimation : packoffset(c0);
};

cbuffer cbTerrainTessellation : register(b8)
{
	float4 gvTessellationFactor; // x: MinTess, y: MaxTess, z: MinDist, w: MaxDist
	float  gTerrainHeightScale;
	float3 gvUnused;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Water Shaders
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VS_WATER_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD0;
};

struct VS_WATER_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

#include "Light.hlsl"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//#define _WITH_VERTEX_LIGHTING

#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40

Texture2D gtxtAlbedoTexture : register(t24);
Texture2D gtxtSpecularTexture : register(t25);
Texture2D gtxtNormalTexture : register(t26);
Texture2D gtxtMetallicTexture : register(t27);
Texture2D gtxtEmissionTexture : register(t28);
Texture2D gtxtDetailAlbedoTexture : register(t29);
Texture2D gtxtDetailNormalTexture : register(t30);

Texture2D gtxtWaterBaseTexture : register(t6);
Texture2D gtxtWaterDetail0Texture : register(t7);
Texture2D gtxtWaterDetail1Texture : register(t8);

SamplerState gssWrap : register(s0);
SamplerState gssClamp : register(s1);

// Shadow calculation function
float CalcPcfShadow(float4 positionW)
{
    float4 shadowPos = mul(positionW, gmtxShadowTransform);
    shadowPos /= shadowPos.w;

    // Point 1: Shadow Coordinate Range Check
    // If the pixel is outside the light's frustum, it's not shadowed.
    if (shadowPos.x < 0.0f || shadowPos.x > 1.0f ||
        shadowPos.y < 0.0f || shadowPos.y > 1.0f ||
        shadowPos.z < 0.0f || shadowPos.z > 1.0f)
    {
        return 1.0f; // No shadow
    }

    // Point 3: Dynamic Texel Size Calculation
    float2 texel = float2(1.0f / gShadowMapSize, 1.0f / gShadowMapSize);
    float  z     = shadowPos.z - gShadowBias;

    float s = 0.0f;
    [unroll] for(int y=-1; y<=1; y++)
    {
        [unroll] for(int x=-1; x<=1; x++)
        {
            s += gShadowMap.SampleCmpLevelZero(gssShadow, shadowPos.xy + float2(x,y)*texel, z);
        }
    }
    return s / 9.0f;
}

VS_WATER_OUTPUT VSTerrainWater(VS_WATER_INPUT input)
{
    VS_WATER_OUTPUT output;

    
    float4 worldPos = mul(float4(input.position, 1.0f), gmtxGameObject);
    
   
    output.position = mul(mul(worldPos, gmtxView), gmtxProjection);
    
    
    output.uv = input.uv;

    return output;
}

float4 PSTerrainWater(VS_WATER_OUTPUT input) : SV_TARGET
{
	float2 uv = input.uv;

	
	uv = mul(float3(input.uv, 1.0f), (float3x3)gf4x4TextureAnimation).xy;

	
	float4 cBaseTexColor = gtxtWaterBaseTexture.SampleLevel(gssWrap, uv, 0);
	float4 cDetail0TexColor = gtxtWaterDetail0Texture.SampleLevel(gssWrap, uv * 20.0f, 0);
	float4 cDetail1TexColor = gtxtWaterDetail1Texture.SampleLevel(gssWrap, uv * 20.0f, 0);

	
	float4 cColor = lerp(cBaseTexColor * cDetail0TexColor, cDetail1TexColor.r * 0.5f, 0.35f);
	
	return(cColor);
}

struct VS_STANDARD_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
};

struct VS_STANDARD_OUTPUT
{
	float4 position : SV_POSITION;
	float3 positionW : POSITION;
	float3 normalW : NORMAL;
	float3 tangentW : TANGENT;
	float3 bitangentW : BITANGENT;
	float2 uv : TEXCOORD;
};

VS_STANDARD_OUTPUT VSStandard(VS_STANDARD_INPUT input)
{
    VS_STANDARD_OUTPUT output;

    // 위치(점) -> w=1
    float4 posW = mul(float4(input.position, 1.0f), gmtxGameObject);
    output.positionW = posW.xyz;

    // 방향 벡터 -> w=0  (평행이동 제외!)
    output.tangentW   = normalize(mul((float3x3)gmtxGameObject, input.tangent));
    output.bitangentW = normalize(mul((float3x3)gmtxGameObject, input.bitangent));
    output.normalW    = normalize(mul((float3x3)gmtxGameObject, input.normal)); // 임시

    // TBN을 직교화(특히 tangent)
    float3 N = output.normalW;
    float3 T = normalize(output.tangentW - N * dot(output.tangentW, N));
    float3 B = normalize(cross(N, T));   // 입력 bitangent 대신 cross로 재구성(더 안정적)

    // 저장할 땐 직교화된 걸 저장
    output.tangentW   = T;
    output.bitangentW = B;
    output.normalW    = N;

    output.position = mul(mul(posW, gmtxView), gmtxProjection);
    output.uv = input.uv;
    return output;
}

float4 PSStandard(VS_STANDARD_OUTPUT input) : SV_TARGET
{
	float4 cAlbedoColor = gMaterial.m_cDiffuse;
	float4 cSpecularColor = gMaterial.m_cSpecular;
	float4 cNormalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float4 cMetallicColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float4 cEmissionColor = gMaterial.m_cEmissive;

	if (gnTexturesMask & MATERIAL_ALBEDO_MAP) cAlbedoColor *= gtxtAlbedoTexture.Sample(gssWrap, input.uv);
	if (gnTexturesMask & MATERIAL_SPECULAR_MAP) cSpecularColor *= gtxtSpecularTexture.Sample(gssWrap, input.uv);
	if (gnTexturesMask & MATERIAL_NORMAL_MAP) cNormalColor = gtxtNormalTexture.Sample(gssWrap, input.uv);
	if (gnTexturesMask & MATERIAL_METALLIC_MAP) cMetallicColor = gtxtMetallicTexture.Sample(gssWrap, input.uv);
	if (gnTexturesMask & MATERIAL_EMISSION_MAP) cEmissionColor *= gtxtEmissionTexture.Sample(gssWrap, input.uv);
	
	float3 normalW = normalize(input.normalW);
	if (gnTexturesMask & MATERIAL_NORMAL_MAP)
    {
        float3x3 TBN = float3x3(input.tangentW, input.bitangentW, input.normalW);
        float3 vNormalTS = normalize(cNormalColor.rgb * 2.0f - 1.0f);
        normalW = normalize(mul(vNormalTS, TBN));
    }
    
    // Calculate full illumination
    float4 cIllumination = Lighting(input.positionW, normalW);

    // Calculate shadow factor
    float shadowFactor = CalcPcfShadow(float4(input.positionW, 1.0f));
    
    // Apply shadow factor by lerping between ambient light and full illumination.
    // This is a simple and effective way to apply shadow without complex refactoring.
    float4 cFinalColor = lerp(gcGlobalAmbientLight * gMaterial.m_cAmbient, cIllumination, shadowFactor);
	cFinalColor.a = gMaterial.m_cDiffuse.a; // Preserve original alpha
    
    return cFinalColor;
}

float4 PSStandardReflectionTest(VS_STANDARD_OUTPUT input) : SV_Target
{
    return float4(1, 0, 0, 1);
}

float4 PSStandardPlayer(VS_STANDARD_OUTPUT input) : SV_TARGET
{
	float4 c = PSStandard(input); // 기존 로직 재사용(함수로 빼는 게 더 좋음)
	c.a = 0.3f; // 투명도 30%
	return c;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_SKYBOX_CUBEMAP_INPUT
{
	float3 position : POSITION;
};

struct VS_SKYBOX_CUBEMAP_OUTPUT
{
	float3	positionL : POSITION;
	float4	position : SV_POSITION;
};

VS_SKYBOX_CUBEMAP_OUTPUT VSSkyBox(VS_SKYBOX_CUBEMAP_INPUT input)
{
	VS_SKYBOX_CUBEMAP_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.positionL = input.position;

	return(output);
}

TextureCube gtxtSkyCubeTexture : register(t13);


float4 PSSkyBox(VS_SKYBOX_CUBEMAP_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtSkyCubeTexture.Sample(gssClamp, input.positionL);

	return(cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_SPRITE_TEXTURED_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
};

struct VS_SPRITE_TEXTURED_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_SPRITE_TEXTURED_OUTPUT VSTextured(VS_SPRITE_TEXTURED_INPUT input)
{
	VS_SPRITE_TEXTURED_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.uv = input.uv;

	return(output);
}

/*
float4 PSTextured(VS_SPRITE_TEXTURED_OUTPUT input, uint nPrimitiveID : SV_PrimitiveID) : SV_TARGET
{
	float4 cColor;
	if (nPrimitiveID < 2)
		cColor = gtxtTextures[0].Sample(gWrapSamplerState, input.uv);
	else if (nPrimitiveID < 4)
		cColor = gtxtTextures[1].Sample(gWrapSamplerState, input.uv);
	else if (nPrimitiveID < 6)
		cColor = gtxtTextures[2].Sample(gWrapSamplerState, input.uv);
	else if (nPrimitiveID < 8)
		cColor = gtxtTextures[3].Sample(gWrapSamplerState, input.uv);
	else if (nPrimitiveID < 10)
		cColor = gtxtTextures[4].Sample(gWrapSamplerState, input.uv);
	else
		cColor = gtxtTextures[5].Sample(gWrapSamplerState, input.uv);
	float4 cColor = gtxtTextures[NonUniformResourceIndex(nPrimitiveID/2)].Sample(gWrapSamplerState, input.uv);

	return(cColor);
}
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
Texture2D gtxtTerrainTexture : register(t14);
Texture2D gtxtDetailTexture : register(t15);
Texture2D gtxtAlphaTexture : register(t16);
Texture2D gtxtAlphaTextures[] : register(t23);

float4 PSTextured(VS_SPRITE_TEXTURED_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtTerrainTexture.Sample(gssWrap, input.uv);

	return(cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tessellation Shaders
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VS_TERRAIN_INPUT
{
	float3 posL : POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

struct VS_TERRAIN_OUTPUT
{
	float3 posW : POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

VS_TERRAIN_OUTPUT VSTerrain(VS_TERRAIN_INPUT input)
{
	VS_TERRAIN_OUTPUT o;

	float4 posW = mul(float4(input.posL, 1.0f), gmtxGameObject);
	o.posW = posW.xyz;
	o.color = input.color;
	o.uv0 = input.uv0;
	o.uv1 = input.uv1;

	return o;
}

struct HS_TERRAIN_CONSTANT_DATA
{
	float Edges[4]  : SV_TessFactor;
	float Inside[2] : SV_InsideTessFactor;
};

HS_TERRAIN_CONSTANT_DATA HSTerrainConstant(InputPatch<VS_TERRAIN_OUTPUT, 4> ip, uint PatchID : SV_PrimitiveID)
{
	HS_TERRAIN_CONSTANT_DATA o;

	// 패치 중심 (월드)
	float3 center = 0.25f * (ip[0].posW + ip[1].posW + ip[2].posW + ip[3].posW);

	// 카메라 위치(cbCameraInfo 안의 gvCameraPosition 사용)
	float dist = distance(center, gvCameraPosition.xyz);

	float minTess = gvTessellationFactor.x;
	float maxTess = gvTessellationFactor.y;
	float minDist = gvTessellationFactor.z;
	float maxDist = gvTessellationFactor.w;

	// 0~1 LOD factor
	float t = saturate((dist - minDist) / (maxDist - minDist));
	float tess = lerp(maxTess, minTess, t);

	o.Edges[0] = o.Edges[1] = o.Edges[2] = o.Edges[3] = tess;
	o.Inside[0] = o.Inside[1] = tess;

	return o;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("HSTerrainConstant")]
[maxtessfactor(64.0f)]
VS_TERRAIN_OUTPUT HSTerrain(InputPatch<VS_TERRAIN_OUTPUT, 4> ip, uint i : SV_OutputControlPointID)
{
	return ip[i];
}

struct DS_TERRAIN_OUTPUT
{
	float4 posH : SV_POSITION;
	float3 posW : POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

[domain("quad")]
DS_TERRAIN_OUTPUT DSTerrain(HS_TERRAIN_CONSTANT_DATA input, float2 uv : SV_DomainLocation, const OutputPatch<VS_TERRAIN_OUTPUT, 4> patch)
{
    DS_TERRAIN_OUTPUT o;

    // 색, UV 보간 (기존 그대로)
    float4 cTop = lerp(patch[0].color, patch[1].color, uv.x);
    float4 cBot = lerp(patch[2].color, patch[3].color, uv.x);
    o.color = lerp(cTop, cBot, uv.y);

    float2 uv0Top = lerp(patch[0].uv0, patch[1].uv0, uv.x);
    float2 uv0Bot = lerp(patch[2].uv0, patch[3].uv0, uv.x);
    o.uv0 = lerp(uv0Top, uv0Bot, uv.y);

    float2 uv1Top = lerp(patch[0].uv1, patch[1].uv1, uv.x);
    float2 uv1Bot = lerp(patch[2].uv1, patch[3].uv1, uv.x);
    o.uv1 = lerp(uv1Top, uv1Bot, uv.y);

    // 위치 보간
    float3 pTop = lerp(patch[0].posW, patch[1].posW, uv.x);
    float3 pBot = lerp(patch[2].posW, patch[3].posW, uv.x);
    o.posW = lerp(pTop, pBot, uv.y);

    //  이 두 줄은 잠시 주석 처리
	float h = gtxtAlphaTexture.SampleLevel(gssClamp, o.uv0, 0).r;
    o.posW.y += h * gTerrainHeightScale;

    // 최종 클립 좌표
    o.posH = mul(float4(o.posW, 1.0f), gmtxView);
    o.posH = mul(o.posH, gmtxProjection);

    return o;
}


float4 PSTerrain(DS_TERRAIN_OUTPUT input) : SV_TARGET
{
    // (기존) 텍스처 섞어서 albedo 만들기
    float4 cBaseTexColor   = gtxtTerrainTexture.Sample(gssWrap, input.uv0);
    float4 cDetailTexColor = gtxtDetailTexture.Sample(gssWrap, input.uv1);
    float4 albedo = cBaseTexColor * 0.7f + cDetailTexColor * 0.3f;

    // (추가) 그림자 계수
    float shadow = CalcPcfShadow(float4(input.posW, 1.0f));

    // (추가) 지형은 조명이 약하니까 “완전 검정” 말고 ambient를 남겨주는 게 보기 좋음
    float3 ambient = albedo.rgb * 0.05f; // Use a much darker ambient for higher contrast

    // 기존에 “버텍스 라이팅 색을 쓰고 싶다”는 의도가 있으면 input.color 같은 걸 곱해도 됨
    float3 lit = albedo.rgb;

    float3 finalColor = lerp(ambient, lit, shadow);
    
    return float4(finalColor, albedo.a);
}



// Tessellation Shaders for Shadow Map
VS_TERRAIN_OUTPUT VS_TERRAIN_SHADOW(VS_TERRAIN_INPUT input)
{
	VS_TERRAIN_OUTPUT o;
	float4 posW = mul(float4(input.posL, 1.0f), gmtxGameObject);
	o.posW = posW.xyz;
	// We don't need color or uvs for the shadow pass, but the struct is shared
	o.color = float4(0,0,0,0);
	o.uv0 = input.uv0;
	o.uv1 = float2(0,0);
	return o;
}

// Hull shader can be reused, as it only calculates tessellation factors based on distance
HS_TERRAIN_CONSTANT_DATA HS_TERRAIN_SHADOW(InputPatch<VS_TERRAIN_OUTPUT, 4> ip, uint PatchID : SV_PrimitiveID)
{
	return HSTerrainConstant(ip, PatchID);
}
struct VS_SHADOW_OUTPUT
{
	float4 position : SV_POSITION;
};
// Domain shader for shadow pass: only calculates position
[domain("quad")]
VS_SHADOW_OUTPUT DS_TERRAIN_SHADOW(HS_TERRAIN_CONSTANT_DATA input, float2 uv : SV_DomainLocation, const OutputPatch<VS_TERRAIN_OUTPUT, 4> patch)
{
    VS_SHADOW_OUTPUT o;

    // Interpolate world position
    float3 pTop = lerp(patch[0].posW, patch[1].posW, uv.x);
    float3 pBot = lerp(patch[2].posW, patch[3].posW, uv.x);
    float3 posW = lerp(pTop, pBot, uv.y);

	// Interpolate UVs for height map lookup
	float2 uv0Top = lerp(patch[0].uv0, patch[1].uv0, uv.x);
    float2 uv0Bot = lerp(patch[2].uv0, patch[3].uv0, uv.x);
    float2 uv0 = lerp(uv0Top, uv0Bot, uv.y);

    // Get height from texture
	float h = gtxtAlphaTexture.SampleLevel(gssClamp, uv0, 0).r;
    posW.y += h * gTerrainHeightScale;

    // Final clip space position for depth
    o.position = mul(float4(posW, 1.0f), gmtxView);
    o.position = mul(o.position, gmtxProjection);

    return o;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UI SHADERS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Texture2D gtxtUITexture : register(t0);

// VS_SPRITE_TEXTURED_INPUT and VS_SPRITE_TEXTURED_OUTPUT are already defined and are suitable for UI.

VS_SPRITE_TEXTURED_OUTPUT VS_UI(VS_SPRITE_TEXTURED_INPUT input)
{
	VS_SPRITE_TEXTURED_OUTPUT output;

	output.position = float4(input.position, 1.0f);
	output.uv = input.uv;

	return(output);
}

float4 PS_UI(VS_SPRITE_TEXTURED_OUTPUT input) : SV_TARGET
{
	return gtxtUITexture.Sample(gssWrap, input.uv);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
// Billboard Shaders
//-------------------------------------------------------------------------------------------------------------------------------------------------

Texture2D gtxtBillboard : register(t17); // 단일 텍스처로 변경

//-------------------------------------------------------------------------------------------------------------------------------------------------
// VS_INPUT: 애플리케이션에서 정점 하나의 정보를 받습니다.
//-------------------------------------------------------------------------------------------------------------------------------------------------
struct VS_BILLBOARD_INPUT
{
	float3 position : POSITION; // 빌보드가 생성될 월드 공간의 위치
};

//-------------------------------------------------------------------------------------------------------------------------------------------------
// GS_INPUT: Vertex Shader에서 넘어온 정보를 받습니다.
//-------------------------------------------------------------------------------------------------------------------------------------------------
struct GS_BILLBOARD_INPUT
{
	float3 position : POSITION; // VS에서 전달된 월드 공간 위치
};

//-------------------------------------------------------------------------------------------------------------------------------------------------
// PS_INPUT: Geometry Shader에서 생성된 정점 정보를 받습니다.
//-------------------------------------------------------------------------------------------------------------------------------------------------
struct PS_BILLBOARD_INPUT
{
	float4 position : SV_POSITION; // 최종 클립 공간 위치
	float2 uv : TEXCOORD;       // 텍스처 UV 좌표
};


GS_BILLBOARD_INPUT VSBillboard(VS_BILLBOARD_INPUT input)
{
	GS_BILLBOARD_INPUT output;
	output.position = input.position; // 월드 위치를 그대로 전달
	return output;
}


[maxvertexcount(4)]
void GSBillboard(point GS_BILLBOARD_INPUT input[1], inout TriangleStream<PS_BILLBOARD_INPUT> outputStream)
{
   
	float2 size = float2(8.0f, 8.0f);

	
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 right = normalize(float3(gmtxInverseView._11, 0.0f, gmtxInverseView._13));

	float3 positions[4];
	positions[0] = input[0].position + (-right * size.x) + (up * size.y);
	positions[1] = input[0].position + (right * size.x) + (up * size.y);
	positions[2] = input[0].position + (-right * size.x) - (up * size.y);
	positions[3] = input[0].position + (right * size.x) - (up * size.y);

	float2 uvs[4] =
	{
		float2(0.0f, 0.0f),
		float2(1.0f, 0.0f),
		float2(0.0f, 1.0f),
		float2(1.0f, 1.0f)
	};

	PS_BILLBOARD_INPUT output;
	
	[unroll]
	for (int i = 0; i < 4; i++)
	{
		output.position = float4(positions[i], 1.0f);
		output.position = mul(output.position, gmtxView);
		output.position = mul(output.position, gmtxProjection);
		output.uv = uvs[i];
		outputStream.Append(output);
	}
	
	outputStream.RestartStrip();
}

float4 PSBillboard(PS_BILLBOARD_INPUT input) : SV_TARGET
{
	float4 color = gtxtBillboard.Sample(gssWrap, input.uv);
    
	
	return color;
}


#define EXP_FRAME_COLS 8
#define EXP_FRAME_ROWS 8

struct VS_GS_INPUT
{
    float3 position : POSITION;
    uint   frame    : TEXCOORD0; // Frame index
};

struct GS_PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
	uint   frame    : TEXCOORD1;
};

VS_GS_INPUT VS_Explosion(float3 position : POSITION)
{
    VS_GS_INPUT output;
    output.position = mul(float4(position, 1.0f), gmtxGameObject).xyz;
    output.frame = gnTexturesMask; 
    return output;
}

[maxvertexcount(4)]
void GS_Explosion(point VS_GS_INPUT input[1], inout TriangleStream<GS_PS_INPUT> outputStream)
{
    float3 up = float3(gmtxView._12, gmtxView._22, gmtxView._32);
    float3 right = float3(gmtxView._11, gmtxView._21, gmtxView._31);

    float halfSize = 20.0f; // Explosion size

    float4 positions[4];
    positions[0] = float4(input[0].position + (-right + up) * halfSize, 1.0f);
    positions[1] = float4(input[0].position + ( right + up) * halfSize, 1.0f);
    positions[2] = float4(input[0].position + (-right - up) * halfSize, 1.0f);
    positions[3] = float4(input[0].position + ( right - up) * halfSize, 1.0f);

    GS_PS_INPUT output;
    
    output.position = mul(positions[0], gmtxView);
    output.position = mul(output.position, gmtxProjection);
    output.uv = float2(0.0f, 0.0f);
	output.frame = input[0].frame;
    outputStream.Append(output);

    output.position = mul(positions[1], gmtxView);
    output.position = mul(output.position, gmtxProjection);
    output.uv = float2(1.0f, 0.0f);
	output.frame = input[0].frame;
    outputStream.Append(output);

    output.position = mul(positions[2], gmtxView);
    output.position = mul(output.position, gmtxProjection);
    output.uv = float2(0.0f, 1.0f);
	output.frame = input[0].frame;
    outputStream.Append(output);

	output.position = mul(positions[3], gmtxView);
    output.position = mul(output.position, gmtxProjection);
    output.uv = float2(1.0f, 1.0f);
	output.frame = input[0].frame;
    outputStream.Append(output);
	
	outputStream.RestartStrip();
}

// Pixel Shader: Calculate the correct UV for the sprite sheet and sample the texture
float4 PS_Explosion(GS_PS_INPUT input) : SV_TARGET
{
    uint frame = input.frame;
    
    float fCellW = 1.0f / EXP_FRAME_COLS;
    float fCellH = 1.0f / EXP_FRAME_ROWS;
    
    uint u_idx = frame % EXP_FRAME_COLS;
    uint v_idx = frame / EXP_FRAME_COLS;
    
    float2 startUV = float2(u_idx * fCellW, v_idx * fCellH);
    
    float2 finalUV = startUV + input.uv * float2(fCellW, fCellH);
    
    float4 color = gtxtAlbedoTexture.Sample(gssWrap, finalUV);
    
    // Discard pixel if alpha is too low to prevent square outlines
    clip(color.a - 0.01f); 
    
    return color;
}

float4 PSMirror(VS_STANDARD_OUTPUT input) : SV_TARGET
{
	float4 cColor = gMaterial.m_cAmbient;
	cColor.a = gMaterial.m_cAmbient.a;
	return cColor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MOTION BLUR COMPUTE SHADER
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RWTexture2D<float4> g_OutputTexture : register(u0);
Texture2D<float4>   g_InputTexture  : register(t0);
SamplerState        g_Sampler       : register(s0);

cbuffer cbMotionBlur : register(b4)
{
    int   g_nWidth;
    int   g_nHeight;
    float g_fBlurStrength;
    int   g_nDirection;
};

// 1. 강의 자료처럼 그룹 내 스레드들이 공유할 메모리 선언 (16x16 크기)
groupshared float4 g_SharedCache[16][16];

[numthreads(16, 16, 1)]
void CSMotionBlur(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID)
{
    // 전역 좌표 (이미지 전체에서의 위치)
    int2 texCoord = dispatchThreadID.xy;
    // 그룹 내 좌표 (0~15, 0~15) - 강의 자료의 구조를 따르기 위해 필요
    int2 localCoord = groupThreadID.xy;

    // -------------------------------------------------------------------------
    // [강의 자료 스타일] 1. 공유 메모리(L1 캐시)에 데이터 미리 로드 (Pre-loading)
    // -------------------------------------------------------------------------
    // 현재 스레드가 담당하는 픽셀을 VRAM에서 읽어와 공유 메모리에 저장합니다.
    // 경계 검사: 이미지 크기를 벗어나지 않는 경우에만 로드
    if (texCoord.x < g_nWidth && texCoord.y < g_nHeight)
    {
        g_SharedCache[localCoord.y][localCoord.x] = g_InputTexture[texCoord];
    }
    else
    {
        g_SharedCache[localCoord.y][localCoord.x] = float4(0, 0, 0, 0);
    }

    // -------------------------------------------------------------------------
    // [강의 자료 스타일] 2. 동기화 (Synchronization)
    // -------------------------------------------------------------------------
    // 그룹 내 모든 스레드가 공유 메모리 로딩을 마칠 때까지 대기합니다.
    GroupMemoryBarrierWithGroupSync();

    // -------------------------------------------------------------------------
    // 3. 블러 연산 (기존 로직 유지 + 최적화)
    // -------------------------------------------------------------------------
    float2 center = float2(g_nWidth * 0.5f, g_nHeight * 0.5f);
    float2 dir = center - texCoord;
    const int numSamples = 12;
    float4 finalColor = float4(0, 0, 0, 0);

    for (int i = 0; i < numSamples; ++i)
    {
        float2 offset = dir * (i / (float)numSamples) * g_fBlurStrength;
        
        // 샘플링할 정확한 위치 (실수 좌표)
        float2 samplePos = texCoord + offset;
        int2 sampleIntPos = int2(samplePos); // 정수 좌표 변환

        // ---------------------------------------------------------------------
        // [강의 자료 응용] 공유 메모리 활용 판단
        // 샘플링할 위치가 '현재 스레드 그룹(16x16)' 안에 있는지 확인합니다.
        // dispatchThreadID에서 localCoord를 빼면 현재 그룹의 시작점(Top-Left)을 알 수 있습니다.
        // ---------------------------------------------------------------------
        int2 groupStartPos = texCoord - localCoord; // 현재 그룹의 시작 좌표
        int2 relativePos = sampleIntPos - groupStartPos; // 그룹 내 상대 좌표

        // 만약 샘플링 위치가 현재 공유 메모리 캐시(0~15 범위) 안에 있다면?
        if (relativePos.x >= 0 && relativePos.x < 16 && 
            relativePos.y >= 0 && relativePos.y < 16)
        {
            // [Fast] 공유 메모리에서 읽음 (강의 자료 방식)
            // 주의: 공유 메모리는 Point Sampling이므로 텍스처 필터링(Bilinear) 효과는 약간 줄어들 수 있음
            finalColor += g_SharedCache[relativePos.y][relativePos.x]; 
        }
        else
        {
            // [Slow] 범위를 벗어나면 기존처럼 텍스처에서 읽음 (VRAM 접근)
            // 블러 효과를 똑같이 유지하기 위해 Clamp 적용
            float2 sampleCoord = clamp(samplePos, float2(0, 0), float2(g_nWidth-1, g_nHeight-1));
            finalColor += g_InputTexture.SampleLevel(g_Sampler, sampleCoord / float2(g_nWidth, g_nHeight), 0);
        }
    }
    
    finalColor /= numSamples;

    // 결과 출력
    if (texCoord.x < g_nWidth && texCoord.y < g_nHeight)
    {
        g_OutputTexture[texCoord] = finalColor;
    }
}
//-----------------------------------------------------------------------------
// Shadow Map Shaders
//-----------------------------------------------------------------------------
struct VS_SHADOW_INPUT
{
	float3 position : POSITION;
};



VS_SHADOW_OUTPUT VS_SHADOW(VS_SHADOW_INPUT input)
{
    VS_SHADOW_OUTPUT output;
    
    output.position = mul(float4(input.position, 1.0f), gmtxGameObject);
    output.position = mul(output.position, gmtxView);
    output.position = mul(output.position, gmtxProjection);
    
    return output;
}

void PS_NULL(VS_SHADOW_OUTPUT input)
{
    // Depth-only pass, no pixel output
}