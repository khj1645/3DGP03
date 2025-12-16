//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once

#include "Shader.h"
#include "Player.h"
#include "BillboardShader.h"
#include "PointMesh.h"
#include "BillboardObject.h" // Added for CBillboardObject
#include "UIRectMesh.h" // Added for UI Rect Mesh
#include "ScreenQuadMesh.h" // Added for Screen Quad Mesh (for background)
#include "UIShader.h" // Added for UI Shader
#include "WaterObject.h" // Added for CWaterObject
#include "CubeMesh.h"
#include <vector>

class CBullet;

struct VS_CB_WATER_ANIMATION
{
	XMFLOAT4X4					m_xmf4x4TextureAnimation;
};

#define MAX_LIGHTS			16 
#define SHADOW_MAP_WIDTH	2048
#define SHADOW_MAP_HEIGHT	2048

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

struct LIGHT
{
	XMFLOAT4				m_xmf4Ambient;
	XMFLOAT4				m_xmf4Diffuse;
	XMFLOAT4				m_xmf4Specular;
	XMFLOAT3				m_xmf3Position;
	float 					m_fFalloff;
	XMFLOAT3				m_xmf3Direction; // Corrected: Added m_xmf3Direction
	float 					m_fTheta; //cos(m_fTheta)
	XMFLOAT3				m_xmf3Attenuation;
	float					m_fPhi; //cos(m_fPhi)
	bool					m_bEnable;
	int						m_nType;
	float					m_fRange;
	float					padding;
};

struct LIGHTS
{
	LIGHT					m_pLights[MAX_LIGHTS];
	XMFLOAT4				m_xmf4GlobalAmbient;
	int						m_nLights;
};

struct TERRAIN_TESSELLATION_INFO
{
	XMFLOAT4				m_xmf4TessellationFactor;
	float					m_fTerrainHeightScale;
};

struct SHADOW_INFO
{
    XMFLOAT4X4              m_xmf4x4ShadowTransform; // World -> Light Clip space -> Texture space
    float                   m_fShadowBias;
	XMFLOAT3				padding;
};

class CDescriptorHeap
{
public:
	CDescriptorHeap();
	~CDescriptorHeap();

	ID3D12DescriptorHeap* m_pd3dCbvSrvDescriptorHeap = NULL;

	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dCbvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dCbvGPUDescriptorStartHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dSrvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dSrvGPUDescriptorStartHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dCbvCPUDescriptorNextHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dCbvGPUDescriptorNextHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dSrvCPUDescriptorNextHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dSrvGPUDescriptorNextHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() { return(m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() { return(m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorStartHandle() { return(m_d3dCbvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorStartHandle() { return(m_d3dCbvGPUDescriptorStartHandle); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorStartHandle() { return(m_d3dSrvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorStartHandle() { return(m_d3dSrvGPUDescriptorStartHandle); }
};

class CGameFramework;
class CMirrorShader;

#include "Mesh.h"
#include "MirrorObject.h"

extern bool g_bEnableMirrorReflection; // Add this

class CScene
{
	friend class CMirrorShader;

public:
    CScene(CGameFramework *pGameFramework);
    ~CScene();

	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	void BuildDefaultLightsAndMaterials();
	void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void ReleaseObjects();

	ID3D12RootSignature *CreateGraphicsRootSignature(ID3D12Device *pd3dDevice);
	ID3D12RootSignature *GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }
	CCamera* GetLightCamera() { return m_pLightCamera; }

	bool ProcessInput(UCHAR *pKeysBuffer);
    void AnimateObjects(float fTimeElapsed);
    void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera=NULL);
	void RenderShadowMap(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera); // 그림자 패스 렌더링 함수
	void RenderSpotlightShadowMap(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	ID3D12Resource* GetShadowMap() { return m_pd3dShadowMap; } // GameFramework에서 그림자 맵 리소스에 접근하기 위한 getter
	ID3D12Resource* GetSpotlightShadowMap() { return m_pd3dSpotlightShadowMap; }

	void ReleaseUploadBuffers();

	CPlayer								*m_pPlayer = NULL;

	CGameObject* m_pStartButtonObject = NULL;
	CGameObject* m_pExitButtonObject = NULL;
	CGameObject* m_pStartButtonSelectedObject = NULL; // New: Larger object for start button hover
	CGameObject* m_pExitButtonSelectedObject = NULL; // New: Larger object for exit button hover
	CGameObject* m_pSelectedObject = NULL; // New: To track currently hovered UI object
	CTexture* m_pStartButtonDefaultTexture = NULL; // New: Default texture for start button
	CTexture* m_pExitButtonDefaultTexture = NULL; // New: Default texture for exit button
	CGameObject* m_pBackgroundObject = NULL; // New: Background object for main menu

	CGameObject* PickObjectByRayIntersection(XMFLOAT3& xmf3PickPosition, XMFLOAT4X4& xmf4x4View);
	void UpdateUIButtons(float fTimeElapsed); // New: Function to update UI button states

	CBullet* SpawnBullet(const XMFLOAT3& xmf3Position, XMFLOAT3& xmf3Direction);
	void AnimateBullets(float fTimeElapsed);
	void RenderBullets(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void CheckBulletCollisions();

public:
	bool m_bWireframe = false;

protected:
	// UI for enemy count
	CTexture* m_pNumberTexture = nullptr;
	CMaterial* m_pNumberMaterial = nullptr;
	static const int m_nMaxEnemyDigits = 3;
	CGameObject* m_pEnemyCountDigits[m_nMaxEnemyDigits] = { nullptr, };

	int GetRemainingEnemyCount();
	void SetDigitUV(CGameObject* pDigit, int digit);
	void UpdateEnemyCountUI();

protected:
	CGameFramework* m_pGameFramework = NULL;

	CGameObject* m_pMainMenuObject = NULL;

	std::vector<CBullet*> m_vBullets;
	CCubeMesh* m_pBulletMesh = NULL;
	CMaterial* m_pBulletMaterial = NULL;

	// Explosion effect resources
	CPointMesh* m_pExplosionMesh = nullptr;
	CMaterial* m_pExplosionMaterial = nullptr;
	CShader* m_pExplosionShader = nullptr; // CExplosionShader

	// Explosion object pool
	std::vector<CExplosionObject*> m_vExplosions;
	int m_nNextExplosion = 0;

public:
	void SpawnExplosion(const XMFLOAT3& position);
	void RenderExplosionsReflect(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, const XMMATRIX& xmmtxReflection);
	void RenderBulletssReflect(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, const XMMATRIX& xmmtxReflection);
	CGameObject* m_pBuildingObject = NULL;
protected:
	void AnimateExplosions(float fTimeElapsed);
	void RenderExplosions(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

protected:
	ID3D12RootSignature*					m_pd3dGraphicsRootSignature = NULL;

	// Shadow Map Resources
	ID3D12Resource*             m_pd3dShadowMap = nullptr;
	ID3D12DescriptorHeap*       m_pd3dShadowDsvHeap = nullptr; // 그림자 맵 전용 DSV 힙
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dShadowDsvCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dShadowSrvGpuHandle; // SRV는 기존 힙에 저장

	D3D12_VIEWPORT              m_d3dShadowViewport;
	D3D12_RECT                  m_d3dShadowScissorRect;
    
	// 광원 카메라 (그림자 맵 생성을 위해)
	CCamera*                    m_pLightCamera = nullptr;

	// Spotlight Shadow Map Resources
	ID3D12Resource*             m_pd3dSpotlightShadowMap = nullptr;
	ID3D12DescriptorHeap*       m_pd3dSpotlightShadowDsvHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dSpotlightShadowDsvCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dSpotlightShadowSrvGpuHandle;

	D3D12_VIEWPORT              m_d3dSpotlightShadowViewport;
	D3D12_RECT                  m_d3dSpotlightShadowScissorRect;
    
	// Spotlight Camera
	CCamera*                    m_pSpotlightCamera = nullptr;

	int									m_nGameObjects = 0;
	CGameObject							**m_ppGameObjects = NULL;

	int									m_nBillboardObjects = 0;
	CGameObject							**m_ppBillboardObjects = NULL;

	int									m_nShaders = 0;
	CShader								**m_ppShaders = NULL;


// ... (existing code)

	CSkyBox								*m_pSkyBox = NULL;
	CHeightMapTerrain*					m_pTerrain = NULL;


	CWaterObject*						m_pWater = NULL; // 물 객체
	XMFLOAT4X4							m_xmf4x4WaterAnimation; // 물 텍스처 애니메이션 매트릭스

	CBillboardShader*					m_pBillboardShader = NULL; // Added for billboard shader
	CTexture*							m_pBillboardTexture = NULL; // Added for billboard texture
	CTerrainShader*						m_pTerrainShader = NULL;

	LIGHT								*m_pLights = NULL;
	int									m_nLights = 0;

	XMFLOAT4							m_xmf4GlobalAmbient;

	ID3D12Resource						*m_pd3dcbLights = NULL;
	LIGHTS								*m_pcbMappedLights = NULL;

	ID3D12Resource						*m_pd3dcbTerrainTessellation = NULL;
	TERRAIN_TESSELLATION_INFO			*m_pcbMappedTerrainTessellation = NULL;

	ID3D12Resource*						m_pd3dcbWaterAnimation = NULL;
	VS_CB_WATER_ANIMATION*				m_pcbMappedWaterAnimation = NULL;

	ID3D12Resource						*m_pd3dcbShadowInfo = NULL;
	SHADOW_INFO							*m_pcbMappedShadowInfo = NULL;

	ID3D12Resource						*m_pd3dcbSpotlightShadowInfo = NULL;
	SHADOW_INFO							*m_pcbMappedSpotlightShadowInfo = NULL;

public:
	CMirrorShader*						m_pMirrorShader = NULL;
	CGameObject*						m_pMirrorObject = NULL;
	CGameObject*						m_pMirrorBackObject = NULL;

public:
	static CDescriptorHeap*				m_pDescriptorHeap;

	static void CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews);
	static void CreateConstantBufferViews(ID3D12Device* pd3dDevice, int nConstantBufferViews, ID3D12Resource* pd3dConstantBuffers, UINT nStride);
	static D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(ID3D12Device* pd3dDevice, ID3D12Resource* pd3dConstantBuffer, UINT nStride);
	static D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(ID3D12Device* pd3dDevice, D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress, UINT nStride);
	static void CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex);
	static void CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex, UINT nRootParameterStartIndex);
	static void CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex);

	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() { return(m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() { return(m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); }

	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle); }
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle); }
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorNextHandle() { return(m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle); }
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle); }
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorStartHandle() { return(m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle); }
};
