#include "Sample.h"
struct Triangle
{
	unsigned short index[3];
};
//void createTangentSpaceVectors(
//	TVector3* v1, TVector3* v2, TVector3* v3,
//	float v1u, float v1v,
//	float v2u, float v2v,
//	float v3u, float v3v,
//	TVector3* vTangent, TVector3* vBiNomal, TVector3* vNomal
//) 
//{
//	TVector3 vEdge1 = *v2 - *v1;
//	TVector3 vEdge2 = *v3 - *v1;
//	float fEdge1_U = v2u - v1u;
//	float fEdge1_V = v2v - v1v;
//	float fEdge2_U = v3u - v1u;
//	float fEdge2_V = v3v - v1v;
//
//	float fDenominator = fEdge1_U * fEdge2_V - fEdge2_U * fEdge1_V;
//	if (fDenominator < 0.00001f && fDenominator > -0.00001f)
//	{
//		*vTangent = TVector3(1.0f, 0.0f, 0.0f);
//		*vBiNomal = TVector3(0.0f, 1.0f, 0.0f);
//		*vNomal = TVector3(0.0f, 0.0f, 1.0f);
//	}
//	else
//	{
//		float fScale1 = 1.0f / fDenominator;
//		TVector3 T;
//		TVector3 B;
//		TVector3 N;
//
//		T = TVector3(
//			(fEdge2_V * vEdge1.x - fEdge1_V * vEdge2.x) * fScale1,
//			(fEdge2_V * vEdge1.y - fEdge1_V * vEdge2.y) * fScale1, 
//			(fEdge2_V * vEdge1.z - fEdge1_V * vEdge2.z) * fScale1);
//		B = TVector3(
//			(-fEdge2_U * vEdge1.x + fEdge1_U * vEdge2.x) * fScale1, 
//			(-fEdge2_U * vEdge1.y + fEdge1_U * vEdge2.y) * fScale1, 
//			(-fEdge2_U * vEdge1.z + fEdge1_U * vEdge2.z) * fScale1);
//
//		D3DXVec3Cross(&N, &T, &B);
//		T::D3DXMatrixtrnapose
//		float fScale2 = 1.0f / ();
//		loadallpath
//
//	}
//
//}

bool Sample::Init()
{
	std::vector<std::string> listname;
	//listname.push_back("../../data/fbx/SM_Barrel.fbx");
	//listname.push_back("../../data/fbx/MultiCameras.fbx");
	//listname.push_back("../../data/fbx/st00sc00.fbx");
	//listname.push_back("../../data/fbx/SM_Tree_Var01.fbx");
	listname.push_back("../../data/fbx/Turret_Deploy1/Turret_Deploy1.fbx");
	listname.push_back("../../data/fbx/Turret_Deploy1/Turret_Deploy1.fbx");

	m_FbxObj.resize(listname.size());
	for (int iObj = 0; iObj < m_FbxObj.size(); iObj++)
	{
		TFbxLoader* pFbx = &m_FbxObj[iObj];
		pFbx->SetPosition(T::TVector3(iObj * 100.0f, 0, 0));

		pFbx->m_fSpeed = (iObj + 1) * 2.0f * 0.5f;
		pFbx->Init();
		pFbx->Load(listname[iObj]);
		pFbx->CreateConstantBuffer(m_pd3dDevice.Get());

		TTexture* pTex = I_Texture.Load(L"../../data/ui/main_start_nor.png");
		TShader* pVShader = I_Shader.CreateVertexShader(m_pd3dDevice.Get(), L"Box.hlsl", "VS");
		TShader* pPShader = I_Shader.CreatePixelShader(m_pd3dDevice.Get(), L"Box.hlsl", "PS");

		for (int iObj = 0; iObj < pFbx->m_DrawList.size(); iObj++)
		{
			// �Ž����� ���ý� ������ �о�´�.
			pFbx->m_DrawList[iObj]->Init();
			pFbx->m_DrawList[iObj]->m_pColorTex =
				I_Texture.Load(pFbx->m_DrawList[iObj]->m_szTexFileName);
			pFbx->m_DrawList[iObj]->m_pVShader = pVShader;
			pFbx->m_DrawList[iObj]->m_pPShader = pPShader;
			// �ִϸ��̼� ������ �ִٸ� �ʱ���ġ���� x
			//pFbx->m_DrawList[iObj]->SetPosition(T::TVector3(0.0f, 400.0f, 50.0f));
			if (!pFbx->m_DrawList[iObj]->Create(m_pd3dDevice.Get(), m_pImmediateContext.Get()))
			{
				return false;
			}
		}
	}
	return true;
}
bool Sample::Frame()
{
	for (int iObj = 0; iObj < m_FbxObj.size(); iObj++)
	{
		TFbxLoader* pFbx = &m_FbxObj[iObj];
		pFbx->m_fTime += g_fSecPerFrame * pFbx->m_Scene.iFrameSpeed * pFbx->m_fDir * pFbx->m_fSpeed;
		if (pFbx->m_fTime >= pFbx->m_Scene.iEnd)
		{
			pFbx->m_fDir *= -1.0f;
		}
		if (pFbx->m_fTime <= pFbx->m_Scene.iStart)
		{
			pFbx->m_fDir *= -1.0f;
		}
		int iFrame = pFbx->m_fTime;
		iFrame = max(0, min(pFbx->m_Scene.iEnd, iFrame));

		for (int iObj = 0; iObj < pFbx->m_TreeList.size(); iObj++)
		{
			TFbxModel* pObject = pFbx->m_TreeList[iObj];
			if (pObject->m_AnimTrack.size() > 0)
			{
				//Ʈ������ �ش��ϴ� �����ӽ����� �ִϸ��̼� ����� ���Ѵ�.
				pFbx->m_TreeList[iObj]->m_matAnim = pObject->m_AnimTrack[iFrame].matTrack;
			}
			if (pObject->m_AnimTrack.size() > 0)
			{
				// �� = Ʈ��
				pFbx->m_matBoneArray.matBoneWorld[iObj] = pObject->m_AnimTrack[iFrame].matTrack;
			}
			// ���������� dx���� ����� ���߱⶧���� ��ġ�� ��������
			// �������� ���̴����� �ִϸ��̼� ����� ���Ұ��̱⶧���� ��ġ�Ѵ�.
			// -> ������ �ִϸ��̼��� ���� ���̴������� ������ ���̶�� 
			// �Ź� �ǽð� �ٲٴ°� ���� �ε��Ҷ� �ٲ㼭 ���� �־��ָ� ���� �ʳ�?
			T::D3DXMatrixTranspose(&pFbx->m_matBoneArray.matBoneWorld[iObj],
				&pFbx->m_matBoneArray.matBoneWorld[iObj]);
		}
		// bufferdesc���� �̹� �������� ũ�⸦ �����߱� ������ 0���� ��ü����.
		m_pImmediateContext.Get()->UpdateSubresource(pFbx->m_pBoneCB, 0, NULL,
			&pFbx->m_matBoneArray, 0, 0);
		//for (int iObj = 0; iObj < m_FbxObj.m_TreeList.size(); iObj++)
		//{
		//	
		//	TMatrix matWorld = m_FbxObj.m_TreeList[iObj]->m_matWorld;
		//	if (m_FbxObj.m_TreeList[iObj]->m_pParentObj != nullptr)
		//	{
		//		TMatrix matParentWorld = m_FbxObj.m_TreeList[iObj]->m_pParentObj->m_matWorld;
		//		matWorld = matWorld * matParentWorld;
		//	}
		//	m_FbxObj.m_TreeList[iObj]->m_matWorld = matWorld;
		//	
		//}
	}
	return true;
}
bool Sample::Render()
{
	for (int iObj = 0; iObj < m_FbxObj.size(); iObj++)
	{
		TFbxLoader* pFbx = &m_FbxObj[iObj];

		m_pImmediateContext.Get()->VSSetConstantBuffers(2, 1, &pFbx->m_pBoneCB);
		for (int iObj = 0; iObj < pFbx->m_DrawList.size(); iObj++)
		{
			TFbxModel* pFbxObj = pFbx->m_DrawList[iObj];
			T::TVector3 vLight(cosf(g_fGameTimer) * 100.0f,
				100,
				sinf(g_fGameTimer) * 100.0f);

			T::D3DXVec3Normalize(&vLight, &vLight);
			vLight = vLight * -1.0f;
			pFbxObj->m_LightConstantList.vLightDir.x = vLight.x;
			pFbxObj->m_LightConstantList.vLightDir.y = vLight.y;
			pFbxObj->m_LightConstantList.vLightDir.z = vLight.z;
			pFbxObj->m_LightConstantList.vLightDir.w = 1.0f;
			m_pImmediateContext->OMSetDepthStencilState(TDxState::g_pDSSDepthDisableWriteDisable, 0x00);
			pFbxObj->m_bAlphaBlend = false;
			// D3DXMatrixIdentity ������ķ� �������ִ� �Լ�
			//T::D3DXMatrixRotationY(&pFbxObj->m_matWorld, g_fGameTimer);
			pFbxObj->SetMatrix(
				// null�� �θ� Tree���� ���� �����δ�
				// FbxLoader�� m_matWorld�� �����ϸ� ���� ������Ʈ������ ��������� �����Ѵ�.
				&pFbx->m_matWorld,		
				&m_pMainCamera->m_matView,
				&m_pMainCamera->m_matProj);
			pFbxObj->Render();
		}
	}

	return true;
}
bool Sample::Release()
{

	if (m_FbxObj.size() > 0)
	{
		for (int iObj = 0; iObj < m_FbxObj.size(); iObj++)
		{
			m_FbxObj[iObj].Release();
		}

	}
	return true;
}



RUN();