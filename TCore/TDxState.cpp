#include "TDxState.h"
// Core.Init()에서 TDxState가 호출되면서 모든 포인터변수가 초기화된다.
ID3D11BlendState* TDxState::m_AlphaBlend = nullptr;
ID3D11BlendState* TDxState::m_AlphaBlendDisable = nullptr;
// 확대 축소에 대해 선형 보간을 사용할지에 대한 결정
// 일반적인 경우 선형보간을 사용하지만 경계의 경우 선형보간 x
// --> 경계선이 보이기때문에 
ID3D11SamplerState* TDxState::m_pSSLinear = nullptr;
ID3D11SamplerState* TDxState::m_pSSPoint = nullptr;
// 페이스의 면을 채울것인지 선으로 표현할 것인지 결정
// cull한 면의 경우 표시하지 않고 날린다.
// ex) cull_back : 뒷면을 표시하지 않음.
ID3D11RasterizerState* TDxState::g_pRSBackCullSolid =nullptr;
ID3D11RasterizerState* TDxState::g_pRSNoneCullSolid = nullptr;
ID3D11RasterizerState* TDxState::g_pRSBackCullWireFrame = nullptr;
ID3D11RasterizerState* TDxState::g_pRSNoneCullWireFrame = nullptr;
// 깊이 값의 사용여부와 깊이 스텐실 버퍼의 사용여부
// 나무는 z버퍼와 Write를 사용
ID3D11DepthStencilState* TDxState::g_pDSSDepthEnable=nullptr;		// 일반적으로 사용
// 알파 블랜딩과 조합하여, 투명이 겹치는 경우 투명한 곳을 표현 
// ex) 나뭇잎과 가지
ID3D11DepthStencilState* TDxState::g_pDSSDepthDisable = nullptr;	
// z버퍼는 비교를 하되 z버퍼 기입은 하지 말라.
// 나뭇잎, 풀, 이팩트 등에 사용
// z값이 더가까운 타겟이 먼저 랜더링 되더라도 없는 것 처럼 만들어준다
// 앞에가 뿌려지고 그위에 뒤에것이 뿌려지는 경우도 만들어진다.
// z값이 기준이 아니라 마지막 출력을 기준으로 뿌려진다
// 식물같은 경우 마지막에 별도로 랜더링
ID3D11DepthStencilState*  TDxState::g_pDSSDepthEnableWriteDisable = nullptr;
ID3D11DepthStencilState* TDxState::g_pDSSDepthDisableWriteDisable = nullptr;

bool TDxState::SetState(ID3D11Device* pd3dDevice)
{
	HRESULT hr;
	// (소스컬러*D3D11_BLEND_SRC_ALPHA) 
	//                  + 
	// (대상컬러*D3D11_BLEND_INV_SRC_ALPHA)
	// 컬러   =  투명컬러값 = (1,1,1,1)
	// 마스크 =  1.0 - 투명컬러값 = (1,1,1,1)

	// FinalColor = SrcColor*SrcAlpha + DestColor*(1.0f- SrcAlpha) 	    
	// if SrcAlpha == 0 완전투명
	//           FinalColor() = SrcColor*0 + DestColor*(1-0)
	//                FinalColor = DestColor;
	// if SrcAlpha == 1 완전불투명
	//           FinalColor() = SrcColor*1 + DestColor*(1-1)
	//                FinalColor = SrcColor;
	// 혼합상태 = 소스(지금드로우객체 픽셀) (연산) 대상(백버퍼 객체:픽셀)
	// 혼합상태 = 픽셀쉐이더 출력 컬러  (연산:사칙연산) 출력버퍼의 컬러
	D3D11_BLEND_DESC  blenddesc;
	ZeroMemory(&blenddesc, sizeof(D3D11_BLEND_DESC));
	/*bd.AlphaToCoverageEnable;
	bd.IndependentBlendEnable;*/
	blenddesc.RenderTarget[0].BlendEnable = TRUE;
	blenddesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blenddesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blenddesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	//// A 연산 저장
	blenddesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blenddesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blenddesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blenddesc.RenderTarget[0].RenderTargetWriteMask =
		D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = pd3dDevice->CreateBlendState(&blenddesc, &m_AlphaBlend);

	blenddesc.RenderTarget[0].BlendEnable = FALSE;
	hr = pd3dDevice->CreateBlendState(&blenddesc, &m_AlphaBlendDisable);


	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(D3D11_SAMPLER_DESC));
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.MinLOD = FLT_MAX;
	sd.MaxLOD = FLT_MIN;
	hr = pd3dDevice->CreateSamplerState(&sd, &m_pSSLinear);
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = pd3dDevice->CreateSamplerState(&sd, &m_pSSPoint);

	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(rsDesc));
	rsDesc.DepthClipEnable = TRUE;
	rsDesc.FillMode = D3D11_FILL_SOLID;
	// cull_none : 앞뒤 모두 채움, cull_front : 앞면을 지움, cull_back : 뒷면을 지움
	rsDesc.CullMode = D3D11_CULL_BACK;
	if (FAILED(hr = pd3dDevice->CreateRasterizerState(&rsDesc, &TDxState::g_pRSBackCullSolid))) return hr;

	rsDesc.DepthClipEnable = TRUE;
	// 연결된 삼각형을 채움
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_NONE;
	if (FAILED(hr = pd3dDevice->CreateRasterizerState(&rsDesc, 
		&TDxState::g_pRSNoneCullSolid))) return hr;


	rsDesc.DepthClipEnable = TRUE;
	// 연결된 삼각형을 채우지 않고 선으로만 연결
	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	rsDesc.CullMode = D3D11_CULL_BACK;
	if (FAILED(hr = pd3dDevice->CreateRasterizerState(
		&rsDesc, &TDxState::g_pRSBackCullWireFrame))) return hr;

	rsDesc.DepthClipEnable = TRUE;
	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	rsDesc.CullMode = D3D11_CULL_NONE;
	if (FAILED(hr = pd3dDevice->CreateRasterizerState(
		&rsDesc, &TDxState::g_pRSNoneCullWireFrame))) return hr;

	
	D3D11_DEPTH_STENCIL_DESC dsDescDepth;
	ZeroMemory(&dsDescDepth, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsDescDepth.DepthEnable = TRUE;
	// 깊이 스텐실 버퍼에 대한 쓰기를 켜기
	dsDescDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDescDepth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsDescDepth.StencilEnable = FALSE;
	dsDescDepth.StencilReadMask = 1;
	dsDescDepth.StencilWriteMask = 1;
	dsDescDepth.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsDescDepth.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	dsDescDepth.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDescDepth.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	// 디폴트 값
	dsDescDepth.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsDescDepth.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDescDepth.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDescDepth.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	if (FAILED(hr = pd3dDevice->CreateDepthStencilState(&dsDescDepth, &g_pDSSDepthEnable)))
	{
		return hr;
	}

	dsDescDepth.DepthEnable = FALSE;
	if (FAILED(hr = pd3dDevice->CreateDepthStencilState(&dsDescDepth, 
		&g_pDSSDepthDisable)))
	{
		return hr;
	}
	// 깊이 스텐실 버퍼에 대한 쓰기를 끄기
	dsDescDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	if (FAILED(hr = pd3dDevice->CreateDepthStencilState(&dsDescDepth,
		&g_pDSSDepthDisableWriteDisable)))
	{
		return hr;
	}
	dsDescDepth.DepthEnable = TRUE;
	dsDescDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	if (FAILED(hr = pd3dDevice->CreateDepthStencilState(&dsDescDepth,
		&g_pDSSDepthEnableWriteDisable)))
	{
		return hr;
	}

	return true;
}
bool TDxState::Release()
{
	if (g_pRSBackCullSolid) g_pRSBackCullSolid->Release();
	if (g_pRSNoneCullSolid) g_pRSNoneCullSolid->Release();
	if (g_pRSBackCullWireFrame) g_pRSBackCullWireFrame->Release();
	if (g_pRSNoneCullWireFrame) g_pRSNoneCullWireFrame->Release();

	if (g_pDSSDepthEnable) g_pDSSDepthEnable->Release();
	if (g_pDSSDepthDisable) g_pDSSDepthDisable->Release();
	if (g_pDSSDepthEnableWriteDisable) g_pDSSDepthEnableWriteDisable->Release();
	if (g_pDSSDepthDisableWriteDisable) g_pDSSDepthDisableWriteDisable->Release();
	if (m_AlphaBlend) m_AlphaBlend->Release();
	if (m_AlphaBlendDisable) m_AlphaBlendDisable->Release();
	m_AlphaBlend = nullptr;
	m_AlphaBlendDisable = nullptr;

	if (m_pSSLinear)m_pSSLinear->Release();
	if (m_pSSPoint)m_pSSPoint->Release();
	return true;
}
