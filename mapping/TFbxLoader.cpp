#define _CRT_SECURE_NO_WARNINGS
#include "TFbxLoader.h"
TMatrix     TFbxLoader::ConvertAMatrix(FbxAMatrix& m)
{
	TMatrix mat;
	float* pMatArray = reinterpret_cast<float*>(&mat);
	double* pSrcArray = reinterpret_cast<double*>(&m);
	for (int i = 0; i < 16; i++)
	{
		pMatArray[i] = pSrcArray[i];
	}
	return mat;
}
// TMatrix�� ����� ���, TFbx�� �����Ϳ� ����̹Ƿ� ��ġ.
TMatrix     TFbxLoader::DxConvertMatrix(TMatrix m)
{
	TMatrix mat;
	mat._11 = m._11; mat._12 = m._13; mat._13 = m._12;
	mat._21 = m._31; mat._22 = m._33; mat._23 = m._32;
	mat._31 = m._21; mat._32 = m._23; mat._33 = m._22;
	mat._41 = m._41; mat._42 = m._43; mat._43 = m._42;
	mat._14 = mat._24 = mat._34 = 0.0f;
	mat._44 = 1.0f;
	return mat;
}
// FbxMatrix�� double�� �����ͷ� ��ȯ�ؼ� float�� �����ͷ� ����ȯ�� TMatrix�� ����
TMatrix     TFbxLoader::ConvertMatrix(FbxMatrix& m)
{
	TMatrix mat;
	float* pMatArray = reinterpret_cast<float*>(&mat);
	double* pSrcArray = reinterpret_cast<double*>(&m);
	for (int i = 0; i < 16; i++)
	{
		pMatArray[i] = pSrcArray[i];
	}
	return mat;
}
void		TFbxLoader::ParseAnimation()
{
	// �ʴ� 30�������� Ʈ���� �����ü� �ְ� ������
	FbxTime::SetGlobalTimeMode(FbxTime::eFrames30);
	// ������ ������ �������� ������ �̸��� �̿��ؼ� ���ÿ� ���� ������ Ȯ��
	// fbx ������ ������ ������Ʈ�� ���� �� �������Ƿ� ���� ������ � ������Ʈ������
	// �ִϸ��̼� ������ ���ϴ��� �����ϴ� �۾�.
	FbxAnimStack* stack = m_pFbxScene->GetSrcObject<FbxAnimStack>(0);
	if (stack == nullptr) return;

	FbxString TakeName = stack->GetName();
	FbxTakeInfo* TakeInfo = m_pFbxScene->GetTakeInfo(TakeName);

	FbxTimeSpan LocalTimeSpan = TakeInfo->mLocalTimeSpan;
	// ��ŸƮ���� �ص������ �ð��� �귯���� ���� ����
	FbxTime start = LocalTimeSpan.GetStart();
	FbxTime end = LocalTimeSpan.GetStop();
	// start���� end���� �ð��� �帣�µ��� �� �ð��� ������ �����Ѵ�.
	FbxTime Duration = LocalTimeSpan.GetDuration();
	// FbxTime�� �̿��ؼ� �������� ���Ѵ�.
	// ex) 0 ~ 50
	FbxTime::EMode TimeMode = FbxTime::GetGlobalTimeMode();
	FbxLongLong s = start.GetFrameCount(TimeMode);
	FbxLongLong n = end.GetFrameCount(TimeMode);
	m_Scene.iStart = s;
	m_Scene.iEnd = n;
	m_Scene.iFrameSpeed = 30;
	// 1�ʿ� 30 frame 
	// 1Frame = 160 Tick
	// 50 Frame 
	FbxTime time;
	TTrack tTrack;
	for (FbxLongLong t = s; t <= n; t++)
	{
		time.SetFrame(t, TimeMode);
		for (int iObj = 0; iObj < m_TreeList.size(); iObj++)
		{
			// Ư���ð����� ������ȯ ����� ���Ѵ�. 
			// ������ ��İ� �θ��� ��ı��� ��� ���յǾ�����.
			FbxAMatrix matGlobal = m_TreeList[iObj]->m_pFbxNode->EvaluateGlobalTransform(time);
			tTrack.iFrame = t;
			tTrack.matTrack = DxConvertMatrix(ConvertAMatrix(matGlobal));
			// Ʈ������Ʈ���� ���� �ð��� �ش��ϴ� ��� �ִϸ��̼� Ʈ���� �־��ش�.
			m_TreeList[iObj]->m_AnimTrack.push_back(tTrack);
		}
	}
}
// ��忡�� �޽������� �о���� ����Լ�
// ���̵������� �ǹ��� �б� ���ؼ� ��尡 ��� �д� ������� ����
void TFbxLoader::PreProcess(FbxNode* Node, TFbxModel* fbxParent)
{
	TFbxModel* fbx = nullptr;
	if (Node != nullptr)
	{
		fbx = new TFbxModel;
		fbx->m_pFbxParent = Node->GetParent();
		fbx->m_pFbxNode = Node;
		fbx->m_csName = to_mw(Node->GetName());
		fbx->m_pParentObj = fbxParent;
		fbx->m_iIndex = m_TreeList.size();
		// Ʈ���� ��� ���� ��ȯ ����� ����
		m_TreeList.push_back(fbx); 
	}
	// ī�޶�, ����Ʈ, �Ž�, �ִϸ��̼� in Node
	FbxMesh* pMesh = Node->GetMesh();
	if (pMesh)
	{
		/*	TFbxModel* fbxDraw = new TFbxModel;
			fbxDraw->m_pFbxParent = Node->GetParent();
			fbxDraw->m_pFbxNode = Node;
			fbxDraw->m_csName = to_mw(Node->GetName());*/
			//fbxDraw->m_pParentObj = parent;
		m_DrawList.push_back(fbx);
	}
	int iNumChild = Node->GetChildCount();
	for (int iNode = 0; iNode < iNumChild; iNode++)
	{
		FbxNode* pChildeNode = Node->GetChild(iNode);
		PreProcess(pChildeNode, fbx);
	}
}
// ������ -> �� -> ��Ʈ��� -> [��� -> �Ž�Ȯ�� -> �Ž� 
// -> ������Ʈ����Ʈ] -> node���� GetMesh
bool TFbxLoader::Load(std::string filename)
{

	bool bReturn;
	bReturn = m_pFbxImporter->Initialize(filename.c_str());
	bReturn = m_pFbxImporter->Import(m_pFbxScene);
	m_pRootNode = m_pFbxScene->GetRootNode();
	// ��忡�� �Ž� ������ �о���� ����Լ�
	PreProcess(m_pRootNode, nullptr);
	ParseAnimation();

	// ������Ʈ�� ��ġ ���� ä���
	for (int iObj = 0; iObj < m_DrawList.size(); iObj++)
	{
		// ����� �о���� ���� ���� ����. ���� PreProcess���� ó���� ��ü
		ParseMesh(m_DrawList[iObj]);
	}
	//m_matBoneArray.resize(m_TreeList.size());	

	return true;
}
// 1. �Ž� �ȿ� ���̾� ����(UV, Materials)�� Ȯ�� 
// 2. �Ž����� ���� �� ��ġ���� ������Ʈ3d�� ���ý�����Ʈ�� �ִ´�.
void TFbxLoader::ParseMesh(TFbxModel* pObject)
{

	FbxMesh* pFbxMesh = pObject->m_pFbxNode->GetMesh();
	// �Ž��� �ִ� ��츸 ���뿡�� �Ž��� ���� ��쵵 �������� ����
	
	// �������(�ʱ� ���� ��ġ�� ��ȯ�� �� ���)
	// transform ���� ������ ����� ���� ���
	// fbx�� ��켱 ���
	FbxAMatrix geom;
	// �ҽ��� �ǹ����� ������ SRT ����
	// FbxNode::eSourcePivot �ҽ��� �������κ����� ��ȯ
	FbxVector4 trans = pObject->m_pFbxNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	FbxVector4 rot = pObject->m_pFbxNode->GetGeometricRotation(FbxNode::eSourcePivot);
	FbxVector4 scale = pObject->m_pFbxNode->GetGeometricScaling(FbxNode::eSourcePivot);
	geom.SetT(trans);
	geom.SetR(rot);
	geom.SetS(scale);

	// dot (u, n) = 0; u ��������(ź��Ʈ����), n �븻 ����
	// u * n^T = 0; ������ ������ ġȯ
	// u * W * W^-1 * n^T = 0; ���� ��ȯ
	// uW*((W^-1*n^T)^T)^T = 0;
	// uW*(n*(W^-1)^T))^T = 0;
	// dot(uW, n(W^-1)^T)) = 0;
	// ��, �븻�� ��ȯ�� ��������� ������� ��ġ
	FbxAMatrix normalMatrix = geom;
	normalMatrix = normalMatrix.Inverse();
	normalMatrix = normalMatrix.Transpose();

	// ���̾� (1���� ������, �������� ���ļ� ������ ����)
	// ���̾�� �Ϲ������� �ϳ��̳� �������� �� �����ִ�.
	std::vector<FbxLayerElementUV*> VertexUVSet;
	std::vector<FbxLayerElementVertexColor*> VertexColorSet;
	// �������� ���ļ� ������ �ϴ� ���� �ƴϴ�
	// ���̾�� ������ ���� ���׸�����. ��忡�� ��������� ���׸���� ����
	// ���׸����� �ϳ��϶� ��忡�� ��������� ���̾�� ������ ���� ���� ����
	std::vector<FbxLayerElementMaterial*> MaterialSet;
	// ���̾�� ������ ������ ����
	int iLayerCount = pFbxMesh->GetLayerCount();
	for (int iLayer = 0; iLayer < iLayerCount; iLayer++)
	{
		FbxLayer* pFbxLayer = pFbxMesh->GetLayer(iLayer);
		if (pFbxLayer->GetUVs() != nullptr)
		{
			VertexUVSet.push_back(pFbxLayer->GetUVs());
		}
		if (pFbxLayer->GetVertexColors() != nullptr)
		{
			VertexColorSet.push_back(pFbxLayer->GetVertexColors());
		}
			//uv�� �����Ѵٸ� ���׸��� ����
		if (pFbxLayer->GetMaterials() != nullptr)
		{
			MaterialSet.push_back(pFbxLayer->GetMaterials());
		}
	}
	// ���׸����� �Ž������� �ƴ϶� ���̽� ������ ����ȴ�.
	// �׷��� mesh���ƴ϶� node���� ���׸��� ������ �����µ�
	// ���� �ؽ��ĸ� ����ϴ� �����ﳢ�� ����
	int iNumMaterials = pObject->m_pFbxNode->GetMaterialCount();

	for (int iMtrl = 0; iMtrl < iNumMaterials; iMtrl++)
	{
		// �ؽ����� �̸��� ������ �ü� �ִ�.
		FbxSurfaceMaterial* pSurface = pObject->m_pFbxNode->GetMaterial(iMtrl);
		if (pSurface)
		{
			std::string texturename = ParseMaterial(pSurface);
			std::wstring szTexFileName = L"../../data/fbx/";
			szTexFileName += to_mw(texturename);
			pObject->m_szTexFileList.push_back(szTexFileName);
			pObject->m_pTextureList.push_back(
				I_Texture.Load(pObject->m_szTexFileList[iMtrl]));
		}
	}
	//pObject->m_szTexFileName = pObject->m_szTexFileList[0]; // �̰� �־�?
	if (iNumMaterials > 0) // ���׸����� �����ϴ� ���
	{
		pObject->m_pSubVertexList.resize(iNumMaterials);
	}
	// ���׸����� �������� �ʴ��� �ϳ��� �Ҵ��ؾ��� �޸� ���� ������ ���� �ʴ¤���.
	else
	{
		pObject->m_pSubVertexList.resize(1);
	}
	int iBasePolyIndex = 0;
	// ������(�ٰ���)�� ���� ������ (6)
	int iPolyCount = pFbxMesh->GetPolygonCount();
	// ������ �� (8)
	int iVertexCount = pFbxMesh->GetControlPointsCount();
	// ��ġ ���� (8���� ��������)
	FbxVector4* pVertexPositions = pFbxMesh->GetControlPoints();
	int* pIndex = pFbxMesh->GetPolygonVertices();
	int iNumFace = 0;

	// 1. iPolyCount��ŭ�� ������ ������ �����Ѵ�.
	// 2. iNumFace��ŭ iPoly��° �����￡ ���̽��� �����Ѵ�.
	// 3. Dx���� ǥ���ϱ� ���� ������ ����3���� ����� ���ؽ� ����Ʈ�� ���� �ִ´�.
	for (int iPoly = 0; iPoly < iPolyCount; iPoly++) 
	{
		// iPoly ��° �������� ������ ������ �����´�
		// iPolySize = 3 :: �ﰢ��
		// iPolySize = 4 :: �簢��
		int iPolySize = pFbxMesh->GetPolygonSize(iPoly);

		// ������ 1���� ���̽��� ���� (3-2 : 1 �ﰢ��, 4-2 : 2 �簢�� �̹Ƿ�
		iNumFace = iPolySize - 2;

		// ���̾�� ���׸��� ������ �� MaterialSet�� ������ 
		// �ش� �������� ���° ���׸����� ����ϴ��� �˾Ƴ���
		// ���׸����� 0�̶���ؼ� �ش� ���� ���׸��� 0�̶�� ������ ���⶧���� ������
		int iSubMtrl = 0;
		if (iNumMaterials >= 1 && MaterialSet[0] != nullptr)
		{
			iSubMtrl = GetSubMaterialIndex(iPoly, MaterialSet[0]);
		}
		for (int iFace = 0; iFace < iNumFace; iFace++)  // ���̽������� �ݺ���
		{
			// fbx�� dx�� �ո��� �ݴ��̱� ������ �ݽð�� ����
			// 0 1 ->	0 2
			//   2		  1
			int VertexIndex[3] = { 0, iFace + 2, iFace + 1 };
			// case DX)
			// ���ؽ��� ���ؽ� ���ۿ� �� �ִ�.
			// �ε����� �ε��� ���ۿ� �� �ִ�.
			// case Max)
			// ������ �ε��� ����
			// ������ ����Ʈ ����
			// �ؽ��� ��ǥ����
			// �ؽ��� �ε��� ��ǥ����
			int CornerIndex[3];
			CornerIndex[0] = pFbxMesh->GetPolygonVertex(iPoly, VertexIndex[0]);
			CornerIndex[1] = pFbxMesh->GetPolygonVertex(iPoly, VertexIndex[1]);
			CornerIndex[2] = pFbxMesh->GetPolygonVertex(iPoly, VertexIndex[2]);

			for (int iIndex = 0; iIndex < 3; iIndex++) // ������ ���� �ݺ���
			{
				TVertex tVertex;
				// �� Index���� ���� ������� ������ ������ �´�.
				FbxVector4 v = pVertexPositions[CornerIndex[iIndex]];
				v = geom.MultT(v);
				tVertex.p.x = v.mData[0];
				tVertex.p.y = v.mData[2];
				tVertex.p.z = v.mData[1];
				//6���� ������ -> 2���� ���̽� -> 3���� �ε���

				int u[3];
				u[0] = pFbxMesh->GetTextureUVIndex(iPoly, VertexIndex[0]);
				u[1] = pFbxMesh->GetTextureUVIndex(iPoly, VertexIndex[1]);
				u[2] = pFbxMesh->GetTextureUVIndex(iPoly, VertexIndex[2]);

				// �ؽ��� ������ �ִٸ� t�߰�
				if (VertexUVSet.size() > 0)
				{
					FbxLayerElementUV* pUVSet = VertexUVSet[0];
					FbxVector2 uv;
					// [out]uv
					ReadTextureCoord(pFbxMesh, pUVSet, CornerIndex[iIndex], u[iIndex],_Out_ uv);
					tVertex.t.x = uv.mData[0];
					// ���̷�Ʈ x�� �ؽ��İ� ���� ����� 0, 0 ������
					// fbx�� �ؽ��İ� ���� �ϴ��� 0, 0�̴�.
					tVertex.t.y = 1.0f - uv.mData[1];
				}

				// ���� �÷� �߰�
				// ����Ʈ ������ ����� ������ �������� ���� ��ȭ�� ���� �ʱ� ���ؼ�
				FbxColor color = FbxColor(1, 1, 1, 1);
				if (VertexColorSet.size() > 0)
				{
					color = ReadColor(pFbxMesh, VertexColorSet.size(),
						VertexColorSet[0],
						CornerIndex[iIndex], iBasePolyIndex + VertexIndex[iIndex]);
				}
				tVertex.c.x = color.mRed;
				tVertex.c.y = color.mGreen;
				tVertex.c.z = color.mBlue;
				tVertex.c.w = pObject->m_iIndex;

				FbxVector4 normal = ReadNormal(pFbxMesh, CornerIndex[iIndex], 
					iBasePolyIndex + VertexIndex[iIndex]);
				normal = normalMatrix.MultT(normal); // ���켱 ���, �̵������ �����ش�
				tVertex.n.x = normal.mData[0];
				tVertex.n.y = normal.mData[2];
				tVertex.n.z = normal.mData[1];

				// ���׸����� ������ ���
				//pObject->m_VertexList.push_back(tVertex);// 36���� ����
				// ���׸����� �������ϰ�� ���� ���׸����� �����ϴ� ���ؽ����� ����
				pObject->m_pSubVertexList[iSubMtrl].push_back(tVertex);
			}
		}
		iBasePolyIndex += iPolySize;

	}

}
bool	TFbxLoader::CreateConstantBuffer(ID3D11Device* pDevice)
{
	HRESULT hr;
	//gpu�޸𸮿� ���� �Ҵ�(���ϴ� �Ҵ� ũ��)
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
	bd.ByteWidth = sizeof(TBoneWorld);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	//D3D11_SUBRESOURCE_DATA sd;
	//ZeroMemory(&sd, sizeof(D3D11_SUBRESOURCE_DATA));
	//sd.pSysMem = &m_matBoneArray;

	// �������Ӹ��� ����ϴ� �����Ͱ� �ٸ��⶧���� �Ҵ縸�ϰ� �����͸� �ѱ����� ����
	if (FAILED(hr = pDevice->CreateBuffer(&bd, NULL,
		&m_pBoneCB)))
	{
		return false;
	}
	return true;
}


bool TFbxLoader::Init()
{
	m_pFbxManager	= FbxManager::Create();
	//FbxIOSettings* ios = FbxIOSettings::Create(m_pFbxManager, IOSROOT);
	//if (ios == nullptr)
	//{
	//	return false;
	//}
	//m_pFbxManager->SetIOSettings(ios);
	//bool bTanget = ios->GetBoolProp(IMP_FBX_TANGENT, true);
	//ios->SetBoolProp(IMP_FBX_TANGENT, true);
	m_pFbxImporter	= FbxImporter::Create(m_pFbxManager, "");
	m_pFbxScene		= FbxScene::Create(m_pFbxManager, "");
	return true;
}

bool TFbxLoader::Frame()
{
	return true;
}

bool TFbxLoader::Render()
{
	return true;
}

bool TFbxLoader::Release()
{
	if (m_pBoneCB)m_pBoneCB->Release();
	m_pBoneCB = nullptr;
	for (int iObj = 0; iObj < m_DrawList.size(); iObj++)
	{
		m_DrawList[iObj]->Release();
	}
	m_pFbxScene->Destroy();
	m_pFbxImporter->Destroy();
	m_pFbxManager->Destroy();
	return true;
}
