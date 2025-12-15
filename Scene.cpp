//-----------------------------------------------------------------------------
// File: CScene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"
#include "GameFramework.h" // Added for GameFramework access
#include "UIRectMesh.h" // Added for UI Rect Mesh
#include "ScreenQuadMesh.h" // Added for Screen Quad Mesh
#include "UIShader.h" // Added for UI Shader
#include "Object.h"
#include "WaterObject.h" // Added for CWaterObject
#include "WaterShader.h" // Added for CWaterShader
#include "GameFramework.h"
#include "ScreenQuadMesh.h"
#include "UIShader.h"
#include "MirrorShader.h"
#include "Mesh.h"
#include "ShadowShader.h" // Added for shadow mapping
#include "d3dx12.h"

CDescriptorHeap* CScene::m_pDescriptorHeap = NULL;

CDescriptorHeap::CDescriptorHeap()
{
	m_d3dSrvCPUDescriptorStartHandle.ptr = NULL;
	m_d3dSrvGPUDescriptorStartHandle.ptr = NULL;
}

CDescriptorHeap::~CDescriptorHeap()
{
	if (m_pd3dCbvSrvDescriptorHeap) m_pd3dCbvSrvDescriptorHeap->Release();
}

void CScene::CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews)
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	d3dDescriptorHeapDesc.NumDescriptors = nConstantBufferViews + nShaderResourceViews; //CBVs + SRVs 
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap);

	m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle = m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle = m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle.ptr = m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle.ptr = m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);

	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle;
}

void CScene::CreateConstantBufferViews(ID3D12Device* pd3dDevice, int nConstantBufferViews, ID3D12Resource* pd3dConstantBuffers, UINT nStride)
{
	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = pd3dConstantBuffers->GetGPUVirtualAddress();
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	for (int j = 0; j < nConstantBufferViews; j++)
	{
		d3dCBVDesc.BufferLocation = d3dGpuVirtualAddress + (nStride * j);
		pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
		m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE CScene::CreateConstantBufferView(ID3D12Device* pd3dDevice, ID3D12Resource* pd3dConstantBuffer, UINT nStride)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	d3dCBVDesc.BufferLocation = pd3dConstantBuffer->GetGPUVirtualAddress();
	pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle;
	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	return(d3dCbvGPUDescriptorHandle);
}

D3D12_GPU_DESCRIPTOR_HANDLE CScene::CreateConstantBufferView(ID3D12Device* pd3dDevice, D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress, UINT nStride)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	d3dCBVDesc.BufferLocation = d3dGpuVirtualAddress;
	pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle;
	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	return(d3dCbvGPUDescriptorHandle);
}

void CScene::CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex)
{
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);

	int nTextures = pTexture->GetTextures();
	for (int i = 0; i < nTextures; i++)
	{
		ID3D12Resource* pShaderResource = pTexture->GetResource(i);
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(i);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
		pTexture->SetGpuDescriptorHandle(i, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
	if (pTexture->GetTextures() > 0) {
		pTexture->SetRootParameterIndex(0, nRootParameterStartIndex); // Set root parameter for the first texture
	}
}

void CScene::CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex, UINT nRootParameterStartIndex)
{
	ID3D12Resource* pShaderResource = pTexture->GetResource(nIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuDescriptorHandle = pTexture->GetGpuDescriptorHandle(nIndex);
	if (pShaderResource && !d3dGpuDescriptorHandle.ptr)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(nIndex);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(nIndex, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetRootParameterIndex(nIndex, nRootParameterStartIndex + nIndex);
	}
}

void CScene::CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex)
{
	ID3D12Resource* pShaderResource = pTexture->GetResource(nIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuDescriptorHandle = pTexture->GetGpuDescriptorHandle(nIndex);
	if (pShaderResource && !d3dGpuDescriptorHandle.ptr)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(nIndex);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(nIndex, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
}

CScene::CScene(CGameFramework* pGameFramework)
{
	m_pGameFramework = pGameFramework;
	m_xmf4x4WaterAnimation = XMFLOAT4X4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	// Shadow map members initialization
	m_pd3dShadowMap = nullptr;
	m_pd3dShadowDsvHeap = nullptr;
	m_d3dShadowDsvCpuHandle = { 0 };
	m_d3dShadowSrvGpuHandle = { 0 };
	m_pLightCamera = nullptr;
}

CScene::~CScene()
{
}

void CScene::BuildDefaultLightsAndMaterials()
{
	m_nLights = 4;
	m_pLights = new LIGHT[m_nLights];
	::ZeroMemory(m_pLights, sizeof(LIGHT) * m_nLights);

	m_xmf4GlobalAmbient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);

	m_pLights[0].m_bEnable = true;
	m_pLights[0].m_nType = POINT_LIGHT;
	m_pLights[0].m_fRange = 1000.0f;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.1f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.0f);
	m_pLights[0].m_xmf3Position = XMFLOAT3(30.0f, 30.0f, 30.0f);
	m_pLights[0].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_pLights[0].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.001f, 0.0001f);
	m_pLights[1].m_bEnable = true;
	m_pLights[1].m_nType = SPOT_LIGHT;
	m_pLights[1].m_fRange = 500.0f;
	m_pLights[1].m_xmf4Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_pLights[1].m_xmf4Diffuse = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	m_pLights[1].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[1].m_xmf3Position = XMFLOAT3(-50.0f, 20.0f, -5.0f);
	m_pLights[1].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	m_pLights[1].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[1].m_fFalloff = 8.0f;
	m_pLights[1].m_fPhi = (float)cos(XMConvertToRadians(40.0f));
	m_pLights[1].m_fTheta = (float)cos(XMConvertToRadians(20.0f));
	m_pLights[2].m_bEnable = true;
	m_pLights[2].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[2].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[2].m_xmf4Diffuse = XMFLOAT4(0.9f, 0.8f, 0.6f, 1.0f); // Warmer color
	m_pLights[2].m_xmf4Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 0.0f);
	XMFLOAT3 dir = XMFLOAT3(0.5f, -0.2f, 0.5f); // Very low angle for dramatic, long shadows
	XMVECTOR vDir = XMLoadFloat3(&dir);
	vDir = XMVector3Normalize(vDir);
	XMStoreFloat3(&m_pLights[2].m_xmf3Direction, vDir);
	m_pLights[3].m_bEnable = true;
	m_pLights[3].m_nType = SPOT_LIGHT;
	m_pLights[3].m_fRange = 600.0f;
	m_pLights[3].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[3].m_xmf4Diffuse = XMFLOAT4(0.3f, 0.7f, 0.0f, 1.0f);
	m_pLights[3].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[3].m_xmf3Position = XMFLOAT3(50.0f, 30.0f, 30.0f);
	m_pLights[3].m_xmf3Direction = XMFLOAT3(0.0f, 1.0f, 1.0f);
	m_pLights[3].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[3].m_fFalloff = 8.0f;
	m_pLights[3].m_fPhi = (float)cos(XMConvertToRadians(90.0f));
	m_pLights[3].m_fTheta = (float)cos(XMConvertToRadians(30.0f));
}

void CScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	m_pDescriptorHeap = new CDescriptorHeap();
	CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, 507); 

	// 1. 광원 카메라 생성
	m_pLightCamera = new CCamera();
	m_pLightCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	XMFLOAT4X4 xmf4x4Projection;
	float shadowMapWidth = 3000.0f; // Increased for debugging frustum culling
	float shadowMapHeight = 3000.0f; // Increased for debugging frustum culling
	float shadowNearZ = 1.0f;
	float shadowFarZ = 3000.0f;
	XMStoreFloat4x4(&xmf4x4Projection, XMMatrixOrthographicOffCenterLH(-shadowMapWidth / 2.0f, shadowMapWidth / 2.0f, -shadowMapHeight / 2.0f, shadowMapHeight / 2.0f, shadowNearZ, shadowFarZ));
	m_pLightCamera->SetProjectionMatrix(xmf4x4Projection);


	// 2. 그림자 맵 텍스처 생성
	D3D12_RESOURCE_DESC d3dResourceDesc;
	::ZeroMemory(&d3dResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = SHADOW_MAP_WIDTH;
	d3dResourceDesc.Height = SHADOW_MAP_HEIGHT;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_R32_TYPELESS; // Typeless for DSV/SRV flexibility
	d3dResourceDesc.SampleDesc.Count = 1;
	d3dResourceDesc.SampleDesc.Quality = 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // Corrected Flag

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D32_FLOAT; // Actual DSV format
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES d3dHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	pd3dDevice->CreateCommittedResource(
		&d3dHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Initial state, will transition to DEPTH_WRITE later
		&d3dClearValue,
		__uuidof(ID3D12Resource),
		(void**)&m_pd3dShadowMap
	);

	// 3. 그림자 맵 DSV 힙 및 뷰 생성
	D3D12_DESCRIPTOR_HEAP_DESC d3dDsvHeapDesc;
	d3dDsvHeapDesc.NumDescriptors = 1;
	d3dDsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	d3dDsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // Not shader visible
	d3dDsvHeapDesc.NodeMask = 0;
	pd3dDevice->CreateDescriptorHeap(&d3dDsvHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dShadowDsvHeap);

	D3D12_DEPTH_STENCIL_VIEW_DESC d3dDsvDesc;
	::ZeroMemory(&d3dDsvDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	d3dDsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // Use the D32_FLOAT format for the DSV
	d3dDsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	
	m_d3dShadowDsvCpuHandle = m_pd3dShadowDsvHeap->GetCPUDescriptorHandleForHeapStart();
	pd3dDevice->CreateDepthStencilView(m_pd3dShadowMap, &d3dDsvDesc, m_d3dShadowDsvCpuHandle);


	// 4. 그림자 맵 SRV 생성 (기존 CBV/SRV 힙에)
	D3D12_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc;
	::ZeroMemory(&d3dSrvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
	d3dSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	d3dSrvDesc.Format = DXGI_FORMAT_R32_FLOAT; // SRV will read as R32_FLOAT
	d3dSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	d3dSrvDesc.Texture2D.MipLevels = 1;
	d3dSrvDesc.Texture2D.MostDetailedMip = 0;
	d3dSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	m_d3dShadowSrvGpuHandle = m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle; // Store GPU handle for later binding
	pd3dDevice->CreateShaderResourceView(m_pd3dShadowMap, &d3dSrvDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize; // Advance next handle pointers

	// 5. 뷰포트 및 시저 사각형 설정
	m_d3dShadowViewport.Width = (float)SHADOW_MAP_WIDTH;
	m_d3dShadowViewport.Height = (float)SHADOW_MAP_HEIGHT;
	m_d3dShadowViewport.MinDepth = 0.0f;
	m_d3dShadowViewport.MaxDepth = 1.0f;
	m_d3dShadowViewport.TopLeftX = 0;
	m_d3dShadowViewport.TopLeftY = 0;

	m_d3dShadowScissorRect.left = 0;
	m_d3dShadowScissorRect.top = 0;
	m_d3dShadowScissorRect.right = SHADOW_MAP_WIDTH;
	m_d3dShadowScissorRect.bottom = SHADOW_MAP_HEIGHT;

	BuildDefaultLightsAndMaterials();

	m_nShaders = 6; // Increased from 5 to 6 for ShadowShader
	m_ppShaders = new CShader * [m_nShaders];

	CObjectsShader* pObjectsShader = new CObjectsShader();
	pObjectsShader->AddRef();
	pObjectsShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	pObjectsShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_pTerrain);
	m_ppShaders[0] = pObjectsShader;

	// Add CShadowShader
    CShadowShader* pShadowShader = new CShadowShader();
    pShadowShader->AddRef();
    pShadowShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
    m_ppShaders[5] = pShadowShader; // Assign to index 5 (last available)

	m_pSkyBox = new CSkyBox(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	XMFLOAT3 xmf3Scale(18.0f, 6.0f, 18.0f);
	XMFLOAT4 xmf4Color(0.0f, 0.5f, 0.0f, 0.0f);

	m_pTerrainShader = new CTerrainShader();
	m_pTerrainShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_pTerrain = new CHeightMapTerrain(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, _T("Terrain/HeightMap.raw"), 257, 257, 257, 257, xmf3Scale, xmf4Color);
	m_pTerrain->SetShader(0, m_pTerrainShader); // Set the shader for the terrain
	m_pTerrain->BuildMaterials(pd3dDevice, pd3dCommandList);

	CWaterShader* pWaterShader = new CWaterShader();
	pWaterShader->AddRef();
	pWaterShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_ppShaders[3] = pWaterShader;

	float waterWidth = 257 * xmf3Scale.x;
	float waterLength = 257 * xmf3Scale.z;
	m_pWater = new CWaterObject(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pWaterShader, waterWidth, waterLength);
	m_pWater->SetPosition(920.0f, 545.0f, 1270.0f);

	CUIShader* pUIShader = new CUIShader();
	pUIShader->AddRef();
	pUIShader->CreateShader(pd3dDevice, pd3dCommandList);
	m_ppShaders[1] = pUIShader;

	m_pBackgroundObject = new CGameObject(1, 1);
	CScreenQuadMesh* pBackgroundMesh = new CScreenQuadMesh(pd3dDevice, pd3dCommandList);
	m_pBackgroundObject->SetMesh(0, pBackgroundMesh);

	CTexture* pBackgroundTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pBackgroundTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"UI/title.dds", RESOURCE_TEXTURE2D, 0);
	CScene::CreateShaderResourceView(pd3dDevice, pBackgroundTexture, 0);
	pBackgroundTexture->SetRootParameterIndex(0, 0);

	CMaterial* pBackgroundMaterial = new CMaterial();
	pBackgroundMaterial->SetTexture(pBackgroundTexture);
	pBackgroundMaterial->SetShader(pUIShader);
	m_pBackgroundObject->SetMaterial(0, pBackgroundMaterial);

	m_pStartButtonObject = new CGameObject(1, 1);
	CUIRectMesh* pStartButtonMesh = new CUIRectMesh(pd3dDevice, pd3dCommandList, 0.05f, 0.35f, 0.2f, 0.2f);
	m_pStartButtonObject->SetMesh(0, pStartButtonMesh);

	m_pStartButtonDefaultTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_pStartButtonDefaultTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"UI/start.dds", RESOURCE_TEXTURE2D, 0);
	CScene::CreateShaderResourceView(pd3dDevice, m_pStartButtonDefaultTexture, 0);
	m_pStartButtonDefaultTexture->SetRootParameterIndex(0, 0);

	CMaterial* pStartButtonMaterial = new CMaterial();
	pStartButtonMaterial->SetTexture(m_pStartButtonDefaultTexture);
	pStartButtonMaterial->SetShader(pUIShader);
	m_pStartButtonObject->SetMaterial(0, pStartButtonMaterial);

	m_pStartButtonSelectedObject = new CGameObject(1, 1);
	CUIRectMesh* pStartButtonHoverMesh = new CUIRectMesh(pd3dDevice, pd3dCommandList, 0.05f - (0.2f * 0.05f), 0.35f - (0.2f * 0.05f), 0.2f * 1.1f, 0.2f * 1.1f);
	m_pStartButtonSelectedObject->SetMesh(0, pStartButtonHoverMesh);
	CMaterial* pStartButtonHoverMaterial = new CMaterial();
	pStartButtonHoverMaterial->SetTexture(m_pStartButtonDefaultTexture);
	pStartButtonHoverMaterial->SetShader(pUIShader);
	m_pStartButtonSelectedObject->SetMaterial(0, pStartButtonHoverMaterial);

	m_pExitButtonObject = new CGameObject(1, 1);
	CUIRectMesh* pExitButtonMesh = new CUIRectMesh(pd3dDevice, pd3dCommandList, 0.75f, 0.35f, 0.2f, 0.2f);
	m_pExitButtonObject->SetMesh(0, pExitButtonMesh);

	m_pExitButtonDefaultTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_pExitButtonDefaultTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"UI/exit.dds", RESOURCE_TEXTURE2D, 0);
	CScene::CreateShaderResourceView(pd3dDevice, m_pExitButtonDefaultTexture, 0);
	m_pExitButtonDefaultTexture->SetRootParameterIndex(0, 0);

	CMaterial* pExitButtonMaterial = new CMaterial();
	pExitButtonMaterial->SetTexture(m_pExitButtonDefaultTexture);
	pExitButtonMaterial->SetShader(pUIShader);
	m_pExitButtonObject->SetMaterial(0, pExitButtonMaterial);

	m_pExitButtonSelectedObject = new CGameObject(1, 1);
	CUIRectMesh* pExitButtonHoverMesh = new CUIRectMesh(pd3dDevice, pd3dCommandList, 0.75f - (0.2f * 0.05f), 0.35f - (0.2f * 0.05f), 0.2f * 1.1f, 0.2f * 1.1f);
	m_pExitButtonSelectedObject->SetMesh(0, pExitButtonHoverMesh);
	CMaterial* pExitButtonHoverMaterial = new CMaterial();
	pExitButtonHoverMaterial->SetTexture(m_pExitButtonDefaultTexture);
	pExitButtonHoverMaterial->SetShader(pUIShader);
	m_pExitButtonSelectedObject->SetMaterial(0, pExitButtonHoverMaterial);

	// Create UI for enemy count
	// 1) Load number texture (0-9 texture sheet)
	m_pNumberTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_pNumberTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"UI/num.dds", RESOURCE_TEXTURE2D, 0);
	CScene::CreateShaderResourceView(pd3dDevice, m_pNumberTexture, 0);
	m_pNumberTexture->SetRootParameterIndex(0, 0);

	// 2) Create material for numbers
	m_pNumberMaterial = new CMaterial();
	m_pNumberMaterial->AddRef();
	m_pNumberMaterial->SetTexture(m_pNumberTexture);
	m_pNumberMaterial->SetShader(pUIShader); // Reuse the same CUIShader

	float digitWidth = 0.05f;
	float digitHeight = 0.08f;
	
	float baseX = 0.83f;
	float baseY = 0.05f;

	for (int i = 0; i < m_nMaxEnemyDigits; i++)
	{
		m_pEnemyCountDigits[i] = new CGameObject(1, 1);

		// Position each digit next to the previous one
		float x = baseX + (digitWidth * i);
		float y = baseY;

		CUIRectMesh* pDigitMesh = new CUIRectMesh(pd3dDevice, pd3dCommandList, x, y, digitWidth, digitHeight);
		m_pEnemyCountDigits[i]->SetMesh(0, pDigitMesh);
		m_pEnemyCountDigits[i]->SetMaterial(0, m_pNumberMaterial);

		// Set initial UV to display '0'. The SetDigitUV function will be implemented next.
		SetDigitUV(m_pEnemyCountDigits[i], 0);
	}

	m_pBillboardShader = new CBillboardShader();
	m_pBillboardShader->AddRef();
	m_pBillboardShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_ppShaders[2] = m_pBillboardShader;

	m_pBillboardTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_pBillboardTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Model/Textures/Flower02.dds", RESOURCE_TEXTURE2D, 0);
	CScene::CreateShaderResourceView(pd3dDevice, m_pBillboardTexture, 0, 12);

	int nBillboardsToGenerate = 100;
	m_nBillboardObjects = nBillboardsToGenerate;
	m_ppBillboardObjects = new CGameObject * [m_nBillboardObjects];

	XMFLOAT3 playerInitialPos = XMFLOAT3(920.0f, 745.0f, 1270.0f);
	float generationRadius = 600.0f;
	for (int i = 0; i < nBillboardsToGenerate; ++i)
	{
		float x = playerInitialPos.x + (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * generationRadius;
		float z = playerInitialPos.z + (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * generationRadius;
		float y = m_pTerrain->GetHeight(x, z) + 8.0f;

		XMFLOAT3 billboardPosition = XMFLOAT3(x, y, z);

		CBillboardObject* pBillboardObject = new CBillboardObject(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, billboardPosition, m_pBillboardShader, m_pBillboardTexture);
		m_ppBillboardObjects[i] = pBillboardObject;
	}

	m_pBulletMesh = new CCubeMesh(pd3dDevice, pd3dCommandList, 3.0f, 2.0f, 3.0f);
	m_pBulletMesh->AddRef();
	m_pBulletMaterial = new CMaterial();
	m_pBulletMaterial->AddRef();
	m_pBulletMaterial->m_xmf4AlbedoColor = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f); 
	m_pBulletMaterial->SetShader(pObjectsShader); 

	m_pExplosionShader = new CExplosionShader();
	m_pExplosionShader->AddRef();
	m_pExplosionShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_ppShaders[4] = m_pExplosionShader;

	CTexture* pExplosionTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pExplosionTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Effect.dds", RESOURCE_TEXTURE2D, 0);
	CScene::CreateShaderResourceView(pd3dDevice, pExplosionTexture, 0, 3); 

	m_pExplosionMaterial = new CMaterial();
	m_pExplosionMaterial->AddRef();
	m_pExplosionMaterial->SetTexture(pExplosionTexture);
	m_pExplosionMaterial->SetShader(m_pExplosionShader);

	// 1.3. Create single-point mesh
	XMFLOAT3 xmf3Position(0.0f, 0.0f, 0.0f);
	m_pExplosionMesh = new CPointMesh(pd3dDevice, pd3dCommandList, 1, &xmf3Position);
	m_pExplosionMesh->AddRef();

	// 1.4. Create explosion object pool
	m_vExplosions.resize(50);
	for (int i = 0; i < 50; ++i)
	{
		m_vExplosions[i] = new CExplosionObject();
		m_vExplosions[i]->SetMesh(0, m_pExplosionMesh);
		m_vExplosions[i]->SetMaterial(0, m_pExplosionMaterial);
		m_vExplosions[i]->m_bRender = false; // Initially inactive
		m_vExplosions[i]->AddRef();
	}

	// Create Building
	m_pBuildingObject = new CGameObject(1, 1);
	CCubeMesh* pBuildingMesh = new CCubeMesh(pd3dDevice, pd3dCommandList, 200.0f, 400.0f, 200.0f);
	m_pBuildingObject->SetMesh(0, pBuildingMesh);
	m_pBuildingObject->SetLocalAABB(XMFLOAT3(-100.0f, -200.0f, -100.0f), XMFLOAT3(100.0f, 200.0f, 100.0f));
	CMaterial* pBuildingMaterial = new CMaterial();
	pBuildingMaterial->m_xmf4AlbedoColor = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f); // Gray
	pBuildingMaterial->SetShader(pObjectsShader);
	m_pBuildingObject->SetMaterial(0, pBuildingMaterial);

	float fHeight = m_pTerrain->GetHeight(1000.0f, 1450.0f);
	m_pBuildingObject->SetPosition(1000.0f, fHeight + 200.0f, 1450.0f);

	m_pBuildingObject->UpdateTransform(NULL);

	// Create Mirror on the building wall
	CTexturedRectMesh* pMirrorMesh = new CTexturedRectMesh(pd3dDevice, pd3dCommandList, 80.0f, 80.0f, 0.0f);
	m_pMirrorObject = new CGameObject(1, 1);
	m_pMirrorObject->SetMesh(0, pMirrorMesh);

	CMaterial* pMirrorMaterial = new CMaterial();
	pMirrorMaterial->m_xmf4AmbientColor = XMFLOAT4(0.7f, 0.8f, 0.9f, 0.5f); // Green for debugging
	pMirrorMaterial->m_xmf4AlbedoColor = XMFLOAT4(0.8f, 0.8f, 0.9f, 0.3f); // Semi-transparent
	m_pMirrorObject->SetMaterial(0, pMirrorMaterial);
	m_pMirrorObject->SetPosition(1000.0f, 745.0f, 1349.9f); // On the Z- face of the building
	m_pMirrorObject->Rotate(0.0f, 180.0f, 0.0f); 
	m_pMirrorObject->UpdateTransform(NULL);

	m_pMirrorShader = new CMirrorShader(this, m_pMirrorObject);
	m_pMirrorShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	// Assign the correct shader to the mirror's material
	pMirrorMaterial->SetShader(m_pMirrorShader);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	xmf3Scale = XMFLOAT3(18.0f, 6.0f, 18.0f);
	m_pcbMappedTerrainTessellation->m_xmf4TessellationFactor = XMFLOAT4(4.0f, 32.0f, 20.0f, 200.0f);
	m_pcbMappedTerrainTessellation->m_fTerrainHeightScale = xmf3Scale.y;
}

void CScene::ReleaseObjects()
{
	if (m_pBuildingObject) m_pBuildingObject->Release();
	if (m_pMirrorObject) m_pMirrorObject->Release();

	if (m_pStartButtonObject) m_pStartButtonObject->Release();
	if (m_pExitButtonObject) m_pExitButtonObject->Release();
	if (m_pStartButtonSelectedObject) m_pStartButtonSelectedObject->Release();
	if (m_pExitButtonSelectedObject) m_pExitButtonSelectedObject->Release();
	if (m_pBackgroundObject) m_pBackgroundObject->Release();

	if (m_pMainMenuObject) m_pMainMenuObject->Release();

	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();

	ReleaseShaderVariables();

	if (m_ppShaders)
	{
		for (int i = 0; i < m_nShaders; i++)
		{
			if (m_ppShaders[i])
			{
				m_ppShaders[i]->ReleaseShaderVariables();
				m_ppShaders[i]->ReleaseObjects();
				m_ppShaders[i]->Release();
			}
		}
		delete[] m_ppShaders;
	}

	if (m_pTerrain) delete m_pTerrain;
	if (m_pWater) delete m_pWater;
	if (m_pSkyBox) delete m_pSkyBox;

	for (auto pBullet : m_vBullets)
	{
		if (pBullet) pBullet->Release();
	}
	m_vBullets.clear();

	if (m_pBulletMesh) m_pBulletMesh->Release();
	if (m_pBulletMaterial) m_pBulletMaterial->Release();

	
	for (auto pExplosion : m_vExplosions)
	{
		if (pExplosion) pExplosion->Release();
	}
	m_vExplosions.clear();
	if (m_pExplosionMesh) m_pExplosionMesh->Release();
	if (m_pExplosionMaterial) m_pExplosionMaterial->Release();

	if (m_ppBillboardObjects)
	{
		for (int i = 0; i < m_nBillboardObjects; i++) if (m_ppBillboardObjects[i]) m_ppBillboardObjects[i]->Release();
		delete[] m_ppBillboardObjects;
	}

	if (m_ppGameObjects)
	{
		for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Release();
		delete[] m_ppGameObjects;
	}

	if (m_pLights) delete[] m_pLights;

	if (m_pDescriptorHeap) delete m_pDescriptorHeap;

	// Shadow Map Resource Releases
    if (m_pLightCamera) delete m_pLightCamera;
    if (m_pd3dShadowMap) m_pd3dShadowMap->Release();
    if (m_pd3dShadowDsvHeap) m_pd3dShadowDsvHeap->Release();
}

ID3D12RootSignature* CScene::CreateGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
	ID3D12RootSignature* pd3dGraphicsRootSignature = NULL;

	D3D12_DESCRIPTOR_RANGE pd3dDescriptorRanges[15];

	pd3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[0].NumDescriptors = 1;
	pd3dDescriptorRanges[0].BaseShaderRegister = 24; //t24: gtxtAlbedoTexture
	pd3dDescriptorRanges[0].RegisterSpace = 0;
	pd3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[1].NumDescriptors = 1;
	pd3dDescriptorRanges[1].BaseShaderRegister = 25; //t25: gtxtSpecularTexture
	pd3dDescriptorRanges[1].RegisterSpace = 0;
	pd3dDescriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[2].NumDescriptors = 1;
	pd3dDescriptorRanges[2].BaseShaderRegister = 26; //t26: gtxtNormalTexture
	pd3dDescriptorRanges[2].RegisterSpace = 0;
	pd3dDescriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[3].NumDescriptors = 1;
	pd3dDescriptorRanges[3].BaseShaderRegister = 27; //t27: gtxtMetallicTexture
	pd3dDescriptorRanges[3].RegisterSpace = 0;
	pd3dDescriptorRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[4].NumDescriptors = 1;
	pd3dDescriptorRanges[4].BaseShaderRegister = 28; //t28: gtxtEmissionTexture
	pd3dDescriptorRanges[4].RegisterSpace = 0;
	pd3dDescriptorRanges[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[5].NumDescriptors = 1;
	pd3dDescriptorRanges[5].BaseShaderRegister = 29; //t29: gtxtDetailAlbedoTexture
	pd3dDescriptorRanges[5].RegisterSpace = 0;
	pd3dDescriptorRanges[5].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[6].NumDescriptors = 1;
	pd3dDescriptorRanges[6].BaseShaderRegister = 30; //t30: gtxtDetailNormalTexture
	pd3dDescriptorRanges[6].RegisterSpace = 0;
	pd3dDescriptorRanges[6].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[7].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[7].NumDescriptors = 1;
	pd3dDescriptorRanges[7].BaseShaderRegister = 13; //t13: gtxtSkyBoxTexture
	pd3dDescriptorRanges[7].RegisterSpace = 0;
	pd3dDescriptorRanges[7].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[8].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[8].NumDescriptors = 3;
	pd3dDescriptorRanges[8].BaseShaderRegister = 14; //t14~16: gtxtTerrainTexture
	pd3dDescriptorRanges[8].RegisterSpace = 0;
	pd3dDescriptorRanges[8].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[9].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[9].NumDescriptors = 3;
	pd3dDescriptorRanges[9].BaseShaderRegister = 17; //t17: gtxtBillboards[]
	pd3dDescriptorRanges[9].RegisterSpace = 0;
	pd3dDescriptorRanges[9].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[10].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[10].NumDescriptors = 3;
	pd3dDescriptorRanges[10].BaseShaderRegister = 6;
	pd3dDescriptorRanges[10].RegisterSpace = 0;
	pd3dDescriptorRanges[10].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[11].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[11].NumDescriptors = 1;
	pd3dDescriptorRanges[11].BaseShaderRegister = 23;
	pd3dDescriptorRanges[11].RegisterSpace = 0;
	pd3dDescriptorRanges[11].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// Shadow Map SRV
	pd3dDescriptorRanges[12].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[12].NumDescriptors = 1;
	pd3dDescriptorRanges[12].BaseShaderRegister = 31; //t31: gtxtShadowMap
	pd3dDescriptorRanges[12].RegisterSpace = 0;
	pd3dDescriptorRanges[12].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER pd3dRootParameters[19];

	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[0].Descriptor.ShaderRegister = 1; //Camera
	pd3dRootParameters[0].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[1].Constants.Num32BitValues = 33; // 16 for matrix + 16 for material + 1 for mask = 33
	pd3dRootParameters[1].Constants.ShaderRegister = 2; //GameObject
	pd3dRootParameters[1].Constants.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[2].Descriptor.ShaderRegister = 4; //Lights
	pd3dRootParameters[2].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[3].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[0]);
	pd3dRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[4].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[1]);
	pd3dRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[5].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[2]);
	pd3dRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[6].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[6].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[3]);
	pd3dRootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[7].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[7].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[4]);
	pd3dRootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[8].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[8].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[5]);
	pd3dRootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[9].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[9].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[6]);
	pd3dRootParameters[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[10].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[10].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[7]);
	pd3dRootParameters[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[11].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[11].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[11].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[8]);
	pd3dRootParameters[11].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[12].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[12].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[12].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[9]);
	pd3dRootParameters[12].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[13].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[13].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[13].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[10]);
	pd3dRootParameters[13].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[14].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[14].Descriptor.ShaderRegister = 3;
	pd3dRootParameters[14].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[14].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[15].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[15].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[15].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[11]);
	pd3dRootParameters[15].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[16].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[16].Descriptor.ShaderRegister = 8; //Tessellation
	pd3dRootParameters[16].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[16].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Root Parameter 17: Shadow Map SRV Descriptor Table (t31)
	pd3dRootParameters[17].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[17].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[17].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[12]); // Use pd3dDescriptorRanges[12] for t31
	pd3dRootParameters[17].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Root Parameter 18: Shadow Info CBV (b9)
	pd3dRootParameters[18].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[18].Descriptor.ShaderRegister = 9; // Shadow Info (b9)
	pd3dRootParameters[18].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[18].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC pd3dSamplerDescs[3]; // Increase count to 3

	pd3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].MipLODBias = 0;
	pd3dSamplerDescs[0].MaxAnisotropy = 1;
	pd3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[0].MinLOD = 0;
	pd3dSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[0].ShaderRegister = 0;
	pd3dSamplerDescs[0].RegisterSpace = 0;
	pd3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].MipLODBias = 0;
	pd3dSamplerDescs[1].MaxAnisotropy = 1;
	pd3dSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[1].MinLOD = 0;
	pd3dSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[1].ShaderRegister = 1;
	pd3dSamplerDescs[1].RegisterSpace = 0;
	pd3dSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Shadow Map Comparison Sampler (s2)
	pd3dSamplerDescs[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; // PCF-like
	pd3dSamplerDescs[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER; // Use border for out of bounds
	pd3dSamplerDescs[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[2].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE; // Corrected
	pd3dSamplerDescs[2].MipLODBias = 0;
	pd3dSamplerDescs[2].MaxAnisotropy = 1;
	pd3dSamplerDescs[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // Comparison function
	pd3dSamplerDescs[2].MinLOD = 0;
	pd3dSamplerDescs[2].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[2].ShaderRegister = 2; // s2
	pd3dSamplerDescs[2].RegisterSpace = 0;
	pd3dSamplerDescs[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Only visible to pixel shader

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters); // Update count
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = _countof(pd3dSamplerDescs); // Update count
	d3dRootSignatureDesc.pStaticSamplers = pd3dSamplerDescs;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	HRESULT hr = D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);

	if (FAILED(hr)) {
		OutputDebugString(L"D3D12SerializeRootSignature FAILED!\n");
		if (pd3dErrorBlob) {
			pd3dErrorBlob->Release();
		}
		return NULL; // Or handle more gracefully
	}

	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	// if (pd3dErrorBlob) pd3dErrorBlob->Release(); // Already released if it existed and failed

	return(pd3dGraphicsRootSignature);
}

void CScene::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(LIGHTS) + 255) & ~255); //256 
	m_pd3dcbLights = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbLights->Map(0, NULL, (void**)&m_pcbMappedLights);

	ncbElementBytes = ((sizeof(VS_CB_WATER_ANIMATION) + 255) & ~255); //256
	m_pd3dcbWaterAnimation = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbWaterAnimation->Map(0, NULL, (void**)&m_pcbMappedWaterAnimation);

	ncbElementBytes = ((sizeof(TERRAIN_TESSELLATION_INFO) + 255) & ~255); //256
	m_pd3dcbTerrainTessellation = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbTerrainTessellation->Map(0, NULL, (void**)&m_pcbMappedTerrainTessellation);

    // Create and map Shadow Info constant buffer
    ncbElementBytes = ((sizeof(SHADOW_INFO) + 255) & ~255); //256
    m_pd3dcbShadowInfo = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
    m_pd3dcbShadowInfo->Map(0, NULL, (void**)&m_pcbMappedShadowInfo);
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	::memcpy(m_pcbMappedLights->m_pLights, m_pLights, sizeof(LIGHT) * m_nLights);
	::memcpy(&m_pcbMappedLights->m_xmf4GlobalAmbient, &m_xmf4GlobalAmbient, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedLights->m_nLights, &m_nLights, sizeof(int));

	XMStoreFloat4x4(&m_pcbMappedWaterAnimation->m_xmf4x4TextureAnimation, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4WaterAnimation)));
	pd3dCommandList->SetGraphicsRootConstantBufferView(14, m_pd3dcbWaterAnimation->GetGPUVirtualAddress());

    // Update Shadow Info constant buffer
    if (m_pcbMappedShadowInfo && m_pLightCamera && m_pPlayer && m_pLights[2].m_bEnable) // Check if directional light is enabled
    {
        XMMATRIX lightView = XMLoadFloat4x4(&m_pLightCamera->GetViewMatrix());
        XMMATRIX lightProj = XMLoadFloat4x4(&m_pLightCamera->GetProjectionMatrix());
        XMMATRIX lightVP = lightView * lightProj;

        // Bias matrix to transform from NDC [-1,1] to texture space [0,1]
        XMMATRIX texScaleBiasMatrix = XMMatrixSet(
            0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f, // Y-axis flip because DirectX texture coords are top-down
            0.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.0f,  1.0f
        );

        XMStoreFloat4x4(&m_pcbMappedShadowInfo->m_xmf4x4ShadowTransform, XMMatrixTranspose(lightVP * texScaleBiasMatrix));
        m_pcbMappedShadowInfo->m_fShadowBias = 0.005f; // Initial shadow bias, might need tuning
    }
    pd3dCommandList->SetGraphicsRootConstantBufferView(18, m_pd3dcbShadowInfo->GetGPUVirtualAddress()); // Root parameter 18 (b9)
}

void CScene::ReleaseShaderVariables()
{
	if (m_pd3dcbLights)
	{
		m_pd3dcbLights->Unmap(0, NULL);
		m_pd3dcbLights->Release();
	}

	if (m_pd3dcbWaterAnimation)
	{
		m_pd3dcbWaterAnimation->Unmap(0, NULL);
		m_pd3dcbWaterAnimation->Release();
	}

	if (m_pd3dcbTerrainTessellation)
	{
		m_pd3dcbTerrainTessellation->Unmap(0, NULL);
		m_pd3dcbTerrainTessellation->Release();
	}

    // Release Shadow Info constant buffer
    if (m_pd3dcbShadowInfo)
    {
        m_pd3dcbShadowInfo->Unmap(0, NULL);
        m_pd3dcbShadowInfo->Release();
    }

	if (m_pTerrain) m_pTerrain->ReleaseShaderVariables();
	if (m_pSkyBox) m_pSkyBox->ReleaseShaderVariables();
}

void CScene::ReleaseUploadBuffers()
{
	if (m_pTerrain) m_pTerrain->ReleaseUploadBuffers();
	if (m_pSkyBox) m_pSkyBox->ReleaseUploadBuffers();
	if (m_pWater) m_pWater->ReleaseUploadBuffers();

	for (int i = 0; i < m_nShaders; i++) m_ppShaders[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nGameObjects; i++) m_ppGameObjects[i]->ReleaseUploadBuffers();
}

bool CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pGameFramework->GetGameState() == GameState::MainMenu)
	{
		switch (nMessageID)
		{
		case WM_LBUTTONDOWN:
		{
			if (m_pSelectedObject == m_pStartButtonObject)
			{
				m_pGameFramework->SetGameState(GameState::InGame);
			}
			else if (m_pSelectedObject == m_pExitButtonObject)
			{
				PostQuitMessage(0);
			}
			return true;
		}
		case WM_MOUSEMOVE:
		{
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);

			float ndcX = (2.0f * x) / m_pGameFramework->GetCamera()->GetViewport().Width - 1.0f;
			float ndcY = 1.0f - (2.0f * y) / m_pGameFramework->GetCamera()->GetViewport().Height;

			XMFLOAT3 pickPos = XMFLOAT3(ndcX, ndcY, 0.0f);

			m_pSelectedObject = PickObjectByRayIntersection(pickPos, m_pGameFramework->GetCamera()->GetViewMatrix());
			return true;
		}
		default:
			break;
		}
	}
	return(false);
}

bool CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		default:
			break;
		}
		break;
	default:
		break;
	}
	return(false);
}

bool CScene::ProcessInput(UCHAR* pKeysBuffer)
{
	return(false);
}

void CScene::AnimateObjects(float fTimeElapsed)
{
	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Animate(fTimeElapsed, NULL);
	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->UpdateTransform(NULL);

	for (int i = 0; i < m_nShaders; i++) if (m_ppShaders[i]) m_ppShaders[i]->AnimateObjects(fTimeElapsed);

	if (m_pLights)
	{
		m_pLights[1].m_xmf3Position = m_pPlayer->GetPosition();
		m_pLights[1].m_xmf3Direction = m_pPlayer->GetLookVector();
	}


	AnimateBullets(fTimeElapsed);
	AnimateExplosions(fTimeElapsed);
	CheckBulletCollisions();

	UpdateUIButtons(fTimeElapsed);

	m_xmf4x4WaterAnimation._32 += fTimeElapsed * 0.00125f;
}

void CScene::UpdateUIButtons(float fTimeElapsed)
{
	if (m_pGameFramework->GetGameState() == GameState::MainMenu)
	{
	}
}

CGameObject* CScene::PickObjectByRayIntersection(XMFLOAT3& xmf3PickPosition, XMFLOAT4X4& xmf4x4View)
{
	auto IsPointInBox = [](const XMFLOAT3& point, const CGameObject* pObject) -> bool {
		CMesh* pMesh = pObject->GetMesh(0);
		if (!pMesh) return false;

		CUIRectMesh* pUIMesh = dynamic_cast<CUIRectMesh*>(pMesh);
		if (!pUIMesh) return false;

		float normalizedX = pUIMesh->GetNormalizedX();
		float normalizedY = pUIMesh->GetNormalizedY();
		float normalizedWidth = pUIMesh->GetNormalizedWidth();
		float normalizedHeight = pUIMesh->GetNormalizedHeight();

		float ndcLeft = (normalizedX * 2.0f) - 1.0f;
		float ndcRight = ndcLeft + (normalizedWidth * 2.0f);
		float ndcTop = 1.0f - (normalizedY * 2.0f);
		float ndcBottom = ndcTop - (normalizedHeight * 2.0f);

		const float epsilon = 0.0001f;
		if (point.x >= (ndcLeft - epsilon) && point.x <= (ndcRight + epsilon) &&
			point.y >= (ndcBottom - epsilon) && point.y <= (ndcTop + epsilon)) {
			return true;
		}
		return false;
		};

	if (m_pStartButtonObject && IsPointInBox(xmf3PickPosition, m_pStartButtonObject))
	{
		return m_pStartButtonObject;
	}

	if (m_pExitButtonObject && IsPointInBox(xmf3PickPosition, m_pExitButtonObject))
	{
		return m_pExitButtonObject;
	}

	return nullptr;
}

CBullet* CScene::SpawnBullet(const XMFLOAT3& xmf3Position, XMFLOAT3& xmf3Direction)
{
	CBullet* pBullet = new CBullet();
	pBullet->AddRef();

	pBullet->SetMesh(0, m_pBulletMesh);
	pBullet->SetMaterial(0, m_pBulletMaterial);

	pBullet->SetPosition(xmf3Position);
	pBullet->SetDirection(xmf3Direction);
	pBullet->SetSpeed(250.0f);
	pBullet->SetLifeTime(5.0f);

	pBullet->UpdateTransform(NULL);

	m_vBullets.push_back(pBullet);
	return(pBullet);
}

void CScene::AnimateBullets(float fTimeElapsed)
{
	for (auto it = m_vBullets.begin(); it != m_vBullets.end(); )
	{
		CBullet* pBullet = *it;
		if (pBullet && pBullet->IsAlive())
		{
			pBullet->Animate(fTimeElapsed, NULL);
			pBullet->UpdateTransform(NULL);
			++it;
		}
		else
		{
			if (pBullet) pBullet->Release();
			it = m_vBullets.erase(it);
		}
	}
}

void CScene::CheckBulletCollisions()
{
	CObjectsShader* pObjectsShader = dynamic_cast<CObjectsShader*>(m_ppShaders[0]);
	if (!pObjectsShader) return;

	int nEnemies = pObjectsShader->GetNumberOfObjects();

	for (auto* pBullet : m_vBullets)
	{
		if (!pBullet || !pBullet->IsAlive())
			continue;

		const BoundingBox& bulletBox = pBullet->GetWorldAABB();

		for (int i = 0; i < nEnemies; i++)
		{
			CGameObject* pEnemy = pObjectsShader->GetObject(i);
			if (!pEnemy || !pEnemy->m_bRender) continue;

			const BoundingBox& enemyBox = pEnemy->GetWorldAABB();

			if (bulletBox.Intersects(enemyBox))
			{
				SpawnExplosion(pEnemy->GetPosition());
				pEnemy->m_bRender = false;
				pBullet->Kill();
				break;
			}
		}
	}
}

int CScene::GetRemainingEnemyCount()
{
	// Assuming m_ppShaders[0] is the CObjectsShader that manages enemy objects.
	CObjectsShader* pObjectsShader = dynamic_cast<CObjectsShader*>(m_ppShaders[0]);
	if (!pObjectsShader)
	{
		return 0;
	}

	int nEnemies = pObjectsShader->GetNumberOfObjects();
	int nAlive = 0;

	for (int i = 0; i < nEnemies; i++)
	{
		CGameObject* pEnemy = pObjectsShader->GetObject(i);
		// Count only enemies that are set to be rendered.
		if (pEnemy && pEnemy->m_bRender)
		{
			nAlive++;
		}
	}
	return nAlive;
}

void CScene::SetDigitUV(CGameObject* pDigit, int digit)
{
	if (!pDigit) return;

	
	if (digit < 0) digit = 0;
	if (digit > 9) digit = 9;

	int col = digit % 5;
	int row = digit / 5;

	const float cellW = 1.0f / 5.0f;
	const float cellH = 1.0f / 2.0f;

	float u0 = col * cellW;
	float u1 = u0 + cellW;
	float v0 = row * cellH;
	float v1 = v0 + cellH;

	if (row == 1)
	{
		const float vBias = 0.057f;
		v0 += vBias;
		v1 += vBias;
	}

	const float epsU = 0.002f;
	const float epsV = 0.002f;
	u0 += epsU; u1 -= epsU;
	v0 += epsV; v1 -= epsV;

	CUIRectMesh* pMesh = dynamic_cast<CUIRectMesh*>(pDigit->GetMesh(0));
	if (pMesh) pMesh->SetUVRect(u0, v0, u1, v1);
}

void CScene::UpdateEnemyCountUI()
{
	int nCount = GetRemainingEnemyCount();

	if (nCount < 0) nCount = 0;
	if (nCount > 999) nCount = 999;

	int d2 = (nCount / 100) % 10; 
	int d1 = (nCount / 10) % 10; 
	int d0 = nCount % 10;

	if (m_pEnemyCountDigits[0]) SetDigitUV(m_pEnemyCountDigits[0], d2);
	if (m_pEnemyCountDigits[1]) SetDigitUV(m_pEnemyCountDigits[1], d1);
	if (m_pEnemyCountDigits[2]) SetDigitUV(m_pEnemyCountDigits[2], d0);
}



void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (m_pGameFramework->GetGameState() == GameState::MainMenu)
	{
		if (m_pGameFramework->GetCamera()) m_pGameFramework->GetCamera()->SetViewportsAndScissorRects(pd3dCommandList);

		if (m_pBackgroundObject)
		{
			CUIShader* pUIShader = (CUIShader*)m_ppShaders[1];
			pd3dCommandList->SetGraphicsRootSignature(pUIShader->GetGraphicsRootSignature());
			ID3D12DescriptorHeap* ppHeaps[] = { m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap };
			pd3dCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			m_pBackgroundObject->Render(pd3dCommandList, NULL);
		}

		if (m_ppShaders[1])
		{
			CUIShader* pUIShader = (CUIShader*)m_ppShaders[1];
			pd3dCommandList->SetGraphicsRootSignature(pUIShader->GetGraphicsRootSignature());

			if (m_pSelectedObject == m_pStartButtonObject)
			{
				if (m_pStartButtonSelectedObject) m_pStartButtonSelectedObject->Render(pd3dCommandList, NULL);
			}
			else
			{
				if (m_pStartButtonObject) m_pStartButtonObject->Render(pd3dCommandList, NULL);
			}

			if (m_pSelectedObject == m_pExitButtonObject)
			{
				if (m_pExitButtonSelectedObject) m_pExitButtonSelectedObject->Render(pd3dCommandList, NULL);
			}
			else
			{
				if (m_pExitButtonObject) m_pExitButtonObject->Render(pd3dCommandList, NULL);
			}
		}
	}
	else 
	{
		pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
		ID3D12DescriptorHeap* ppHeaps[] = { m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap };
		pd3dCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		pCamera->SetViewportsAndScissorRects(pd3dCommandList);
		pCamera->UpdateShaderVariables(pd3dCommandList);
		UpdateShaderVariables(pd3dCommandList); 
		
		// Bind Shadow Map and Shadow Info CBV for main pass
		pd3dCommandList->SetGraphicsRootDescriptorTable(17, m_d3dShadowSrvGpuHandle);

		D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
		pd3dCommandList->SetGraphicsRootConstantBufferView(2, d3dcbLightsGpuVirtualAddress);

		if (m_pSkyBox) m_pSkyBox->Render(pd3dCommandList, pCamera);

		pd3dCommandList->SetGraphicsRootConstantBufferView(16, m_pd3dcbTerrainTessellation->GetGPUVirtualAddress());
		if (m_pTerrain)
			m_pTerrain->Render(pd3dCommandList, pCamera);

		if (m_pBuildingObject)
		{
			if (pCamera->IsInFrustum(m_pBuildingObject->GetWorldAABB()))
			{
				m_pBuildingObject->Render(pd3dCommandList, pCamera);
			}
		}
		if (m_ppShaders[0]) m_ppShaders[0]->RenderCulling(pd3dCommandList, pCamera);
		if (m_pPlayer) m_pPlayer->Render(pd3dCommandList, pCamera);
		for (int i = 0; i < m_nBillboardObjects; i++)
		{
			if (m_ppBillboardObjects[i])
			{
				if (pCamera->IsInFrustum(m_ppBillboardObjects[i]->GetWorldAABB()))
				{
					m_ppBillboardObjects[i]->Render(pd3dCommandList, pCamera);
				}
			}
		}
		RenderBullets(pd3dCommandList, pCamera);
		if (m_pWater) m_pWater->Render(pd3dCommandList, pCamera);
		RenderExplosions(pd3dCommandList, pCamera);

		if (m_pMirrorShader)
		{
	
			m_pMirrorShader->PreRender(pd3dCommandList, pCamera);

			m_pMirrorShader->RenderReflectedObjects(pd3dCommandList, pCamera);

			m_pMirrorShader->PostRender(pd3dCommandList, pCamera);
		}
		UpdateEnemyCountUI();
		CUIShader* pUIShader = dynamic_cast<CUIShader*>(m_ppShaders[1]);
		if (pUIShader)
		{
			pd3dCommandList->SetGraphicsRootSignature(pUIShader->GetGraphicsRootSignature());
			pUIShader->Render(pd3dCommandList, nullptr, 0);
			if (m_pNumberTexture && m_pNumberTexture->m_pd3dSrvGpuDescriptorHandles[0].ptr != 0)
			{
				pd3dCommandList->SetGraphicsRootDescriptorTable(0, m_pNumberTexture->m_pd3dSrvGpuDescriptorHandles[0]);
			}
			for (int i = 0; i < m_nMaxEnemyDigits; i++)
			{
				if (m_pEnemyCountDigits[i])
				{
					CMesh* pMesh = m_pEnemyCountDigits[i]->GetMesh(0);
					if (pMesh) pMesh->Render(pd3dCommandList, 0);
				}
			}
		}
		pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	}
}

void CScene::RenderShadowMap(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pMainCamera)
{

    // 1. 뷰포트 및 시저 사각형 설정
    pd3dCommandList->RSSetViewports(1, &m_d3dShadowViewport);
    pd3dCommandList->RSSetScissorRects(1, &m_d3dShadowScissorRect);

    // 2. DSV 초기화
    pd3dCommandList->ClearDepthStencilView(m_d3dShadowDsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

    // 3. 렌더 타겟 설정
    pd3dCommandList->OMSetRenderTargets(0, NULL, FALSE, &m_d3dShadowDsvCpuHandle);

    // 4. 전역 루트 시그니처 및 디스크립터 힙 설정
    pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
    ID3D12DescriptorHeap* ppHeaps[] = { m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap };
	pd3dCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // 5. 광원 카메라 업데이트
    XMVECTOR lightDir = XMLoadFloat3(&m_pLights[2].m_xmf3Direction);
    lightDir = XMVector3Normalize(lightDir);
    XMFLOAT3 playerPos = m_pPlayer->GetPosition();
	XMVECTOR lightPosition = XMLoadFloat3(&playerPos) - (lightDir * 500.0f);
    XMVECTOR lookAt = XMLoadFloat3(&playerPos);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMFLOAT4X4 xmf4x4View;
	XMStoreFloat4x4(&xmf4x4View, XMMatrixLookAtLH(lightPosition, lookAt, up));
    m_pLightCamera->SetViewMatrix(xmf4x4View);
    m_pLightCamera->UpdateShaderVariables(pd3dCommandList);

    // 6. 지형 렌더링 (그림자 패스 전용 PSO 사용)
    if (m_pTerrain && m_pTerrainShader)
    {
        // 지형은 테셀레이션을 사용하므로, CTerrainShader에 있는 그림자 전용 PSO(인덱스 3)를 사용해야 합니다.
        pd3dCommandList->SetPipelineState(m_pTerrainShader->GetPipelineState(3));

		m_pTerrain->UpdateShaderVariable(pd3dCommandList, &m_pTerrain->m_xmf4x4World);
		for (int i = 0; i < m_pTerrain->m_nMeshes; i++)
			if(m_pTerrain->m_ppMeshes[i]) m_pTerrain->m_ppMeshes[i]->Render(pd3dCommandList, 0); // 메시는 PSO 변경 없이 드로우 콜만
    }

                    // 7. 기타 오브젝트 렌더링 (일반 그림자 셰이더 PSO 사용)

                    CShadowShader* pShadowShader = dynamic_cast<CShadowShader*>(m_ppShaders[5]);

                    if (!pShadowShader) return;

                    pd3dCommandList->SetPipelineState(pShadowShader->GetPipelineState(0));

                

                    // 플레이어 렌더링

                    if (m_pPlayer)

                    {

                		m_pPlayer->RenderShadow(pd3dCommandList);

                    }

                    // 빌딩 렌더링

                    if (m_pBuildingObject)

                    {

                		m_pBuildingObject->RenderShadow(pd3dCommandList);

                    }

                    // 기타 오브젝트(헬리콥터 등) 렌더링

                    CObjectsShader* pObjectsShader = dynamic_cast<CObjectsShader*>(m_ppShaders[0]);

                	if (pObjectsShader)

                	{

                		for (int i = 0; i < pObjectsShader->GetNumberOfObjects(); i++)

                		{

                			CGameObject *pObject = pObjectsShader->GetObject(i);

                			if (pObject && pObject->m_bRender)

                			{
								pObject->Animate(0.0f, NULL);
								pObject->UpdateTransform(NULL);
                				pObject->RenderShadow(pd3dCommandList);

                			}

                		}

                	}
}



void CScene::SpawnExplosion(const XMFLOAT3& position)
{
	// Find an inactive explosion from the pool
	for (int i = 0; i < m_vExplosions.size(); ++i)
	{
		int index = (m_nNextExplosion + i) % m_vExplosions.size();
		CExplosionObject* pExplosion = m_vExplosions[index];
		if (pExplosion && !pExplosion->IsAlive())
		{
			pExplosion->Start(position);
			m_nNextExplosion = (index + 1) % m_vExplosions.size();
			return; // Found and started an explosion, so exit
		}
	}
}



void CScene::AnimateExplosions(float fTimeElapsed)

{

	for (auto& pExplosion : m_vExplosions)

	{

		if (pExplosion->IsAlive())

		{

			pExplosion->Animate(fTimeElapsed, nullptr);

			pExplosion->UpdateTransform(nullptr);

		}

	}

}



void CScene::RenderExplosions(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)

{

	for (auto& pExplosion : m_vExplosions)

	{

		if (pExplosion && pExplosion->m_bRender)

		{

			pExplosion->Render(pd3dCommandList, pCamera);

		}

	}

}

void CScene::RenderExplosionsReflect(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, const XMMATRIX& xmmtxReflection)

{

	for (auto& pExplosion : m_vExplosions)

	{

		if (pExplosion && pExplosion->m_bRender)

		{

			pExplosion->Render(pd3dCommandList, pCamera, xmmtxReflection, 1);

		}

	}
	

}

void CScene::RenderBullets(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	for (auto* pBullet : m_vBullets)
	{
		if (pBullet && pBullet->IsAlive())
		{
			pBullet->Render(pd3dCommandList, pCamera);
		}
	}
}

void CScene::RenderBulletssReflect(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, const XMMATRIX& xmmtxReflection)
{
	for (auto* pBullet : m_vBullets)
	{
		if (pBullet && pBullet->IsAlive())
		{
			pBullet->Render(pd3dCommandList, pCamera, xmmtxReflection,1);
		}
	}
}