#pragma once

#define FRAME_BUFFER_WIDTH		1200
#define FRAME_BUFFER_HEIGHT		800

#include "Timer.h"
#include "Player.h"
#include "Scene.h"

enum class GameState
{
	MainMenu,
	InGame
};

class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	void CreateRtvAndDsvDescriptorHeaps();

	void CreateRenderTargetViews();
	void CreateDepthStencilView();

	void ChangeSwapChainState();

    void BuildObjects();
    void ReleaseObjects();

    void ProcessInput();
    void AnimateObjects();
    void FrameAdvance();

	void WaitForGpuComplete();
	void MoveToNextFrame();

	GameState GetGameState() { return m_GameState; }
	void SetGameState(GameState gameState) { m_GameState = gameState; }
	CPlayer* GetPlayer() { return m_pPlayer; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvCPUDescriptorHandle() { return m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(); }

public:
	CCamera* GetCamera() { return m_pCamera; }

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE					m_hInstance;
	HWND						m_hWnd; 

	int							m_nWndClientWidth;
	int							m_nWndClientHeight;
        
	IDXGIFactory4				*m_pdxgiFactory = NULL;
	IDXGISwapChain3				*m_pdxgiSwapChain = NULL;
	ID3D12Device				*m_pd3dDevice = NULL;

	bool						m_bMsaa4xEnable = false;
	UINT						m_nMsaa4xQualityLevels = 0;

	static const UINT			m_nSwapChainBuffers = 2;
	UINT						m_nSwapChainBufferIndex;

	ID3D12Resource				*m_ppd3dSwapChainBackBuffers[m_nSwapChainBuffers];
	ID3D12DescriptorHeap		*m_pd3dRtvDescriptorHeap = NULL;

	ID3D12Resource				*m_pd3dDepthStencilBuffer = NULL;
	ID3D12DescriptorHeap		*m_pd3dDsvDescriptorHeap = NULL;

	ID3D12CommandAllocator		*m_pd3dCommandAllocator = NULL;
	ID3D12CommandQueue			*m_pd3dCommandQueue = NULL;
	ID3D12GraphicsCommandList	*m_pd3dCommandList = NULL;

	ID3D12Fence					*m_pd3dFence = NULL;
	UINT64						m_nFenceValues[m_nSwapChainBuffers];
	HANDLE						m_hFenceEvent;

#if defined(_DEBUG)
	ID3D12Debug					*m_pd3dDebugController;
#endif

	CGameTimer					m_GameTimer;

	GameState					m_GameState = GameState::MainMenu;
	bool						m_bDebugFrustumTight = false;

	CScene						*m_pScene = NULL;
		CPlayer						*m_pPlayer = NULL;
		CCamera						*m_pCamera = NULL;

	// Motion Blur Resources
		CMotionBlurShader* m_pMotionBlurShader = NULL;
	
		ID3D12Resource* m_pSceneTexture = NULL;
		D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dSceneTextureRtvCPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dSceneTextureSrvGPUHandle;
	
		ID3D12Resource* m_pBlurTexture = NULL;
		D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dBlurTextureUavGPUHandle;
	
		POINT			m_ptOldCursorPos;
	
		float			m_fTimeElapsed = 0.0f;
	
		_TCHAR			m_pszFrameRate[70];
	
	private:
		void CreateSceneTexture();
		void CreateBlurTexture();
};

