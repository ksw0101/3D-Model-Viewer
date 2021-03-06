#include "TDevice.h"
ID3D11Device* g_pd3dDevice = nullptr;
bool TDevice::CreateDetphStencilView()
{
	HRESULT hr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pDSTexture = nullptr;
	D3D11_TEXTURE2D_DESC DescDepth;
	DescDepth.Width = m_SwapChainDesc.BufferDesc.Width;
	DescDepth.Height = m_SwapChainDesc.BufferDesc.Height;
	DescDepth.MipLevels = 1;
	DescDepth.ArraySize = 1;
	DescDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
	DescDepth.SampleDesc.Count = 1;
	DescDepth.SampleDesc.Quality = 0;
	DescDepth.Usage = D3D11_USAGE_DEFAULT;

	// ?? ???? ???? ?? ???ٽ? ???? ????
	DescDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	if (DescDepth.Format == DXGI_FORMAT_R24G8_TYPELESS || DescDepth.Format == DXGI_FORMAT_D32_FLOAT)
	{
		// ???̸? ???? ???̸? ????
		DescDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	}

	DescDepth.CPUAccessFlags = 0;
	DescDepth.MiscFlags = 0;
	if (FAILED(hr = m_pd3dDevice->CreateTexture2D(&DescDepth, NULL, &pDSTexture)))
	{
		return false;
	}

	///// ???̴? ???ҽ? ???? : ???? ?? ?????쿡?? ?????Ѵ?. ///
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&dsvDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

	switch (DescDepth.Format)
	{
	case DXGI_FORMAT_R32_TYPELESS:
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		break;
	case DXGI_FORMAT_R24G8_TYPELESS:
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		break;
	}
	if (srvDesc.Format == DXGI_FORMAT_R32_FLOAT || srvDesc.Format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS)
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		if (FAILED(hr = m_pd3dDevice->CreateShaderResourceView(pDSTexture.Get(), &srvDesc, &m_pDsvSRV)))
		{
			return false;
		}
	}

	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	if (FAILED(hr = m_pd3dDevice->CreateDepthStencilView(pDSTexture.Get(), &dsvDesc, 
		m_pDepthStencilView.GetAddressOf())))
	{
		return false;
	}
	//m_pDepthStencilView->GetDesc(&m_DepthStencilDesc);
	return true;
}

HRESULT TDevice::InitDeivice()
{
	HRESULT hr = S_OK; 
	CreateDevice();
	CreateRenderTargetView();
	CreateDetphStencilView();
	SetViewport();
	return hr;
}
bool	TDevice::CreateDevice()
{
	//D2DWIRETE ???? ?ʼ?
	UINT Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL fl[]
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};

	ZeroMemory(&m_SwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	m_SwapChainDesc.BufferDesc.Width = m_rtClient.right;
	m_SwapChainDesc.BufferDesc.Height = m_rtClient.bottom;
	m_SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	m_SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	m_SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_SwapChainDesc.SampleDesc.Count = 1;
	m_SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	m_SwapChainDesc.BufferCount = 1;
	m_SwapChainDesc.OutputWindow = m_hWnd;
	m_SwapChainDesc.Windowed = true;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		Flags,
		fl,
		1,
		D3D11_SDK_VERSION,
		&m_SwapChainDesc,
		m_pSwapChain.GetAddressOf(),
		m_pd3dDevice.GetAddressOf(),
		&m_FeatureLevel,
		m_pImmediateContext.GetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}	
	DXGI_SWAP_CHAIN_DESC scd;
	m_pSwapChain->GetDesc(&scd);

	g_pd3dDevice = m_pd3dDevice.Get();
	return true;
}
bool	TDevice::CreateRenderTargetView()
{
	ComPtr<ID3D11Texture2D> backBuffer = nullptr;
	m_pSwapChain->GetBuffer(0,
		__uuidof(ID3D11Texture2D),
		(LPVOID*)&backBuffer);
	m_pd3dDevice->CreateRenderTargetView(
		backBuffer.Get(),
		NULL,
		m_pRenderTargetView.GetAddressOf());

	m_pImmediateContext->OMSetRenderTargets(
		1,
		m_pRenderTargetView.GetAddressOf(), NULL);

	D3D11_RENDER_TARGET_VIEW_DESC rtvd;
	m_pRenderTargetView->GetDesc(&rtvd);
	return true;
}
bool	TDevice::SetViewport()
{	
	// ????Ʈ ????
	//DXGI_SWAP_CHAIN_DESC swapDesc;
	//m_pSwapChain->GetDesc(&swapDesc);

	m_ViewPort.TopLeftX = 0;
	m_ViewPort.TopLeftY = 0;
	m_ViewPort.Width = m_SwapChainDesc.BufferDesc.Width;
	m_ViewPort.Height = m_SwapChainDesc.BufferDesc.Height;
	m_ViewPort.MinDepth = 0.0f;
	m_ViewPort.MaxDepth = 1.0f;
	m_pImmediateContext->RSSetViewports(1, &m_ViewPort);
	return true;
}
void     TDevice::ResizeDevice(UINT iWidth, UINT iHeight)
{
	m_pImmediateContext->OMSetRenderTargets(0,NULL, NULL);
	if (m_pRenderTargetView)m_pRenderTargetView->Release();
	if (m_pDepthStencilView)m_pDepthStencilView->Release();

	HRESULT hr = m_pSwapChain->ResizeBuffers(m_SwapChainDesc.BufferCount,
								iWidth, iHeight,
								m_SwapChainDesc.BufferDesc.Format,
								m_SwapChainDesc.Flags);
	if( SUCCEEDED(hr))
	{
		m_pSwapChain->GetDesc(&m_SwapChainDesc);

	}
	CreateRenderTargetView();
	CreateDetphStencilView();
	SetViewport();

	GetClientRect(m_hWnd, &m_rtClient);
	GetWindowRect(m_hWnd, &m_rtWindow);
	g_rtClient = m_rtClient;
}
bool	TDevice::CleapupDevice()
{
	////if (m_pd3dDevice)m_pd3dDevice->Release();	// ?????̽? ??ü
	//if (m_pImmediateContext)m_pImmediateContext->Release();// ?ٺ??̽? ???ؽ?Ʈ ??ü
	//if (m_pSwapChain)m_pSwapChain->Release();	// ????ü?? ??ü
	//if (m_pRenderTargetView)m_pRenderTargetView->Release();
	////m_pd3dDevice = nullptr;	// ?????̽? ??ü
	//m_pImmediateContext = nullptr;// ?ٺ??̽? ???ؽ?Ʈ ??ü
	//m_pSwapChain = nullptr;	// ????ü?? ??ü
	//m_pRenderTargetView = nullptr;
	return true;
}
void TDevice::ClearD3D11DeviceContext(ID3D11DeviceContext* pd3dDeviceContext)
{
	if (pd3dDeviceContext == NULL) return;

	ID3D11ShaderResourceView* pSRVs[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	ID3D11RenderTargetView* pRTVs[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	ID3D11DepthStencilView* pDSV = NULL;
	ID3D11Buffer* pBuffers[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	ID3D11SamplerState* pSamplers[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	UINT StrideOffset[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	// Shaders
	pd3dDeviceContext->VSSetShader(NULL, NULL, 0);
	pd3dDeviceContext->HSSetShader(NULL, NULL, 0);
	pd3dDeviceContext->DSSetShader(NULL, NULL, 0);
	pd3dDeviceContext->GSSetShader(NULL, NULL, 0);
	pd3dDeviceContext->PSSetShader(NULL, NULL, 0);

	// IA clear
	pd3dDeviceContext->IASetVertexBuffers(0, 16, pBuffers, StrideOffset, StrideOffset);
	pd3dDeviceContext->IASetIndexBuffer(NULL, DXGI_FORMAT_R32_UINT, 0);
	pd3dDeviceContext->IASetInputLayout(NULL);

	// Constant buffers
	pd3dDeviceContext->VSSetConstantBuffers(0, 14, pBuffers);
	pd3dDeviceContext->HSSetConstantBuffers(0, 14, pBuffers);
	pd3dDeviceContext->DSSetConstantBuffers(0, 14, pBuffers);
	pd3dDeviceContext->GSSetConstantBuffers(0, 14, pBuffers);
	pd3dDeviceContext->PSSetConstantBuffers(0, 14, pBuffers);

	// Resources
	pd3dDeviceContext->VSSetShaderResources(0, 16, pSRVs);
	pd3dDeviceContext->HSSetShaderResources(0, 16, pSRVs);
	pd3dDeviceContext->DSSetShaderResources(0, 16, pSRVs);
	pd3dDeviceContext->GSSetShaderResources(0, 16, pSRVs);
	pd3dDeviceContext->PSSetShaderResources(0, 16, pSRVs);

	// Samplers
	pd3dDeviceContext->VSSetSamplers(0, 16, pSamplers);
	pd3dDeviceContext->HSSetSamplers(0, 16, pSamplers);
	pd3dDeviceContext->DSSetSamplers(0, 16, pSamplers);
	pd3dDeviceContext->GSSetSamplers(0, 16, pSamplers);
	pd3dDeviceContext->PSSetSamplers(0, 16, pSamplers);

	// Render targets
	pd3dDeviceContext->OMSetRenderTargets(8, pRTVs, pDSV);

	// States
	FLOAT blendFactor[4] = { 0,0,0,0 };
	pd3dDeviceContext->OMSetBlendState(NULL, blendFactor, 0xFFFFFFFF);
	pd3dDeviceContext->OMSetDepthStencilState(NULL, 0);
	pd3dDeviceContext->RSSetState(NULL);
}
TDevice::TDevice()
{
	m_pd3dDevice = nullptr;	// ?????̽? ??ü
	m_pImmediateContext = nullptr;// ?ٺ??̽? ???ؽ?Ʈ ??ü
	m_pSwapChain = nullptr;	// ????ü?? ??ü
	m_pRenderTargetView = nullptr;
}
TDevice::~TDevice()
{}