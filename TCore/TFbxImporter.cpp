#define  _CRT_SECURE_NO_WARNINGS
#include "TFbxImporter.h"
// TMatrix는 행백터 행렬, TFbx는 열백터용 행렬이므로 전치.
TMatrix     TFbxImporter::DxConvertMatrix(TMatrix m)
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
// FbxMatrix를 double형 포인터로 변환해서 float형 포인터로 형변환된 TMatrix에 대입
TMatrix     TFbxImporter::ConvertMatrix(FbxMatrix& m)
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
TMatrix     TFbxImporter::ConvertAMatrix(FbxAMatrix& m)
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
void		TFbxImporter::ParseAnimation()
{
	// 초당 30프레임의 트랙을 가져올수 있게 세팅함
	FbxTime::SetGlobalTimeMode(FbxTime::eFrames30);
	// 씬에서 스택을 가져온후 스택의 이름을 이용해서 스택에 대한 정보를 확인
	// fbx 파일은 복수의 오브젝트가 포함 될 수있으므로 현재 씬에서 어떤 오브젝트에대한
	// 애니메이션 스택을 원하는지 선택하는 작업.
	FbxAnimStack* stack = m_pFbxScene->GetSrcObject<FbxAnimStack>(0);
	if (stack == nullptr) return;

	FbxString TakeName = stack->GetName();
	FbxTakeInfo* TakeInfo = m_pFbxScene->GetTakeInfo(TakeName);
	FbxTimeSpan LocalTimeSpan = TakeInfo->mLocalTimeSpan;
	// 스타트부터 앤드까지의 시간이 흘러가는 것을 설정
	FbxTime start		= LocalTimeSpan.GetStart();
	FbxTime end			= LocalTimeSpan.GetStop();
	// start부터 end까지 시간이 흐르는동안 그 시간의 간격을 설정한다.
	FbxTime Duration	= LocalTimeSpan.GetDuration();
	// FbxTime을 이용해서 프레임을 구한다.
	// ex) 0 ~ 50
	FbxTime::EMode TimeMode = FbxTime::GetGlobalTimeMode();
	FbxLongLong s = start.GetFrameCount(TimeMode);
	FbxLongLong n = end.GetFrameCount(TimeMode);
	m_Scene.iStart = s;
	m_Scene.iEnd = n;
	m_Scene.iFrameSpeed = 30;
	// 1초에 30 frame 
	// 1Frame = 160 Tick
	// 50 Frame 
	FbxTime time;
	TTrack tTrack;
	for (FbxLongLong t = s; t <= n; t++)
	{
		time.SetFrame(t, TimeMode);
		for (int iObj = 0; iObj < m_TreeList.size(); iObj++)
		{
			// -> 부모 노드가 필요한 것도 아닌데 왜 부모노드들을 모델형태로 저장했을까?
			// 특정시간대의 최종변환 행렬을 구한다. 
			// 본인의 행렬과 부모의 행렬까지 모두 결합되어있음.
			FbxAMatrix matGlobal =	m_TreeList[iObj]->m_pFbxNode->EvaluateGlobalTransform(time);			
			tTrack.iFrame = t;
			tTrack.matTrack = DxConvertMatrix(ConvertAMatrix(matGlobal));
			// 행렬분해
			// 행렬을 분해(SRT)
			T::D3DXMatrixDecompose(&tTrack.s, &tTrack.r, &tTrack.t,	&tTrack.matTrack);
			m_TreeList[iObj]->m_AnimTrack.push_back(tTrack);		
		}
	}	
}
// 노드에서 트리(모델), 매쉬를 읽어오는 재귀함수
// 이름을 이용하여 같은 오브젝트 소속의 트리(모델)을 찾을수 있게 맵형태로 저장.
void    TFbxImporter::PreProcess(FbxNode* node, TFbxModel* fbxParent)
{
	TFbxModel* fbx = nullptr;
	if (node != nullptr)
	{
		fbx = new TFbxModel;
		fbx->m_pFbxParent = node->GetParent();
		fbx->m_pFbxNode = node;
		fbx->m_csName = to_mw(node->GetName());
		fbx->m_pParentObj = fbxParent;
		fbx->m_iIndex = m_TreeList.size();
		m_TreeList.push_back(fbx);
		m_pFbxNodeMap.insert(std::make_pair(node, fbx->m_iIndex));
		m_pFbxModelMap.insert(std::make_pair(fbx->m_csName, fbx));
		//ParseAnimation에서 프레임마다 월드행렬을 저장하게 변경되면서 로컬행렬의 저장삭제
	}
	// camera, light, mesh, shape, animation
	FbxMesh* pMesh = node->GetMesh();	
	if (pMesh)
	{
		m_DrawList.push_back(fbx);		
	}
	int iNumChild = node->GetChildCount();
	for (int iNode = 0; iNode < iNumChild; iNode++)
	{
		FbxNode* child = node->GetChild(iNode);
		PreProcess(child, fbx);
	}
}
bool	TFbxImporter::Load(std::string filename)
{
	Init();
		bool bRet = m_pFbxImporter->Initialize(filename.c_str());
		bRet = m_pFbxImporter->Import(m_pFbxScene);
		m_pRootNode = m_pFbxScene->GetRootNode();
		// 기존에는 PreProcess, ParseMesh에서 로컬 행렬을 저장했으나,
		// 애니메이션 기능을 추가하면서 로컬 행렬 저장기능을 모두 삭제
		// ParseAnimation이 로컬행렬 저장기능을 완전히 대체하였다.
		PreProcess(m_pRootNode, nullptr);
		ParseAnimation();
		for (int iObj = 0; iObj < m_DrawList.size(); iObj++)
		{
			ParseMesh(m_DrawList[iObj]);		
		}
		
	Release();
	return true;
}
bool	TFbxImporter::ParseMeshSkinning(
					FbxMesh* pFbxMesh, 
					TFbxModel* pObject)
{
	int iDeformerCount = pFbxMesh->GetDeformerCount(FbxDeformer::eSkin);
	if (iDeformerCount == 0)
	{
		return false;
	}
	// 정점의 개수와 동일한다.
	int iVertexCount = pFbxMesh->GetControlPointsCount();
	pObject->m_WeightList.resize(iVertexCount);

	for (int dwDeformerIndex = 0; dwDeformerIndex < iDeformerCount; dwDeformerIndex++)
	{
		auto pSkin = reinterpret_cast<FbxSkin*>(pFbxMesh->GetDeformer(dwDeformerIndex, FbxDeformer::eSkin));
		DWORD dwClusterCount = pSkin->GetClusterCount();
		// dwClusterCount의 행렬이 전체 정점에 영향을 주었다.
		for (int dwClusterIndex = 0; dwClusterIndex < dwClusterCount; dwClusterIndex++)
		{
			auto pCluster = pSkin->GetCluster(dwClusterIndex);
			////
			FbxAMatrix matXBindPose;
			FbxAMatrix matReferenceGlobalInitPosition;
			pCluster->GetTransformLinkMatrix(matXBindPose);
			pCluster->GetTransformMatrix(matReferenceGlobalInitPosition);
			FbxMatrix matBindPose = matReferenceGlobalInitPosition.Inverse() * matXBindPose;

			TMatrix matInvBindPos = DxConvertMatrix(ConvertMatrix(matBindPose));
			matInvBindPos = matInvBindPos.Invert();
			int  iBoneIndex = m_pFbxNodeMap.find(pCluster->GetLink())->second;
			std::wstring name = m_TreeList[iBoneIndex]->m_csName;
			pObject->m_dxMatrixBindPoseMap.insert(make_pair(name, matInvBindPos));

			int  dwClusterSize = pCluster->GetControlPointIndicesCount();			
			// 영향을 받는 정점들의 인덱스
			int* pIndices = pCluster->GetControlPointIndices();
			double* pWeights = pCluster->GetControlPointWeights();
			// iBoneIndex의 영향을 받는 정점들이 dwClusterSize개 있다.
			for (int i = 0; i < dwClusterSize; i++)
			{
				// n번 정점(pIndices[i])은 iBoneIndex의 행렬에 
				// pWeights[i]의 가중치로 적용되었다.
				int iVertexIndex = pIndices[i];
				float fWeight = pWeights[i];
				pObject->m_WeightList[iVertexIndex].InsertWeight(iBoneIndex, fWeight);
			}
		}
	}
	return true;
}
// 1. 매쉬 안에 레이어 정보(UV, Materials)를 확인 
// 2. 매쉬안의 정보 중 위치값을 오브젝트3d에 버택스리스트에 넣는다.
void	TFbxImporter::ParseMesh(TFbxModel* pObject)
{
	FbxMesh* pFbxMesh = pObject->m_pFbxNode->GetMesh();

	pObject->m_bSkinned = ParseMeshSkinning(pFbxMesh, pObject);
	// 기하행렬(초기 정점 위치를 변환할 때 사용)
	// transform 로컬 정점을 만들기 위해 사용
	// fbx는 열우선 방식
	FbxAMatrix geom;

	// FbxNode::eSourcePivot 소스의 원점으로부터의 변환
	FbxVector4 trans = pObject->m_pFbxNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	FbxVector4 rot = pObject->m_pFbxNode->GetGeometricRotation(FbxNode::eSourcePivot);
	FbxVector4 scale = pObject->m_pFbxNode->GetGeometricScaling(FbxNode::eSourcePivot);
	geom.SetT(trans);
	geom.SetR(rot);
	geom.SetS(scale);

	// dot (u, n) = 0; u 접선벡터(탄젠트백터), n 노말 벡터
	// u * n^T = 0; 내적을 곱으로 치환
	// u * W * W^-1 * n^T = 0; 월드 변환
	// uW*((W^-1*n^T)^T)^T = 0;
	// uW*(n*(W^-1)^T))^T = 0;
	// dot(uW, n(W^-1)^T)) = 0;
	// 즉, 노말의 변환은 월드행렬의 역행렬의 전치
	FbxAMatrix normalMatrix = geom;
	normalMatrix = normalMatrix.Inverse();
	normalMatrix = normalMatrix.Transpose();

	// 레이어 ( 1번에 랜더링, 여러번에 걸쳐서 랜더링 개념)
	// 레이어는 일반적으로 하나이나 여러개가 들어갈 수도있다.
	std::vector<FbxLayerElementUV*> VertexUVSet;
	std::vector<FbxLayerElementVertexColor*> VertexColorSet;
	// 여러번에 걸쳐서 랜더링 하는 것이 아니다
	// 레이어에서 가지고 오는 머테리얼임. 노드에서 가지고오는 머테리얼과 구분
	// 머테리얼이 하나일땐 노드에서 가지고오나 레이어에서 가지고 오나 같은 정보
	std::vector<FbxLayerElementMaterial*> MaterialSet;
	std::vector<FbxLayerElementNormal*>		VertexNormalSets;
	int iLayerCount = pFbxMesh->GetLayerCount();

	if (iLayerCount == 0 || pFbxMesh->GetLayer(0)->GetNormals() == nullptr)
	{
		pFbxMesh->InitNormals();
#if (FBXSDK_VERSION_MAJOR >= 2015)
		pFbxMesh->GenerateNormals();
#else
		pFbxMesh->ComputeVertexNormals();
#endif
	}

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
		if (pFbxLayer->GetNormals() != NULL)
		{
			VertexNormalSets.push_back(pFbxLayer->GetNormals());
		}
		if (pFbxLayer->GetMaterials() != nullptr)
		{
			MaterialSet.push_back(pFbxLayer->GetMaterials());
		}
	}

	// 머테리얼은 매쉬단위가 아니라 페이스 단위로 적용된다.

	//  1개의 오브젝트가 여러장의 텍스처를 사용한다.
	//  각각의 텍스처를 이름을 얻고 저장한다.
	//  어떤 페이스(폴리곤)가 어떤 텍스처를 사용하니?
	//  같은 텍스처를 사용하는 폴리곤들 끼리 저장한다.
	int iNumMtrl = pObject->m_pFbxNode->GetMaterialCount();
	for (int iMtrl = 0; iMtrl < iNumMtrl; iMtrl++)
	{
		// 노드에서 텍스쳐의 이름을 불러온 뒤 텍스쳐 매니저로 SRV를 만든다.
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
	if(pObject->m_szTexFileList.size() > 0)
	{
		pObject->m_szTexFileName = pObject->m_szTexFileList[0];
	}
	if (iNumMtrl > 0)
	{
		pObject->m_pSubVertexList.resize(iNumMtrl);
		pObject->m_pSubIWVertexList.resize(iNumMtrl);
	}
	else // 메모리 참조 오류를 막기위해 최소 1개의 배열을 만든다.
	{
		pObject->m_pSubVertexList.resize(1);
		pObject->m_pSubIWVertexList.resize(1);
	}

	int iBasePolyIndex = 0;
	// 폴리곤(다각형)의 수를 가져옴 (정육면체의 경우 6개)
	int iNumPolyCount = pFbxMesh->GetPolygonCount();
	// 위치 정보 (정육면체의 경우 8개의 정점 position정보)
	FbxVector4* pVertexPositions = pFbxMesh->GetControlPoints();
	int iNumFace = 0;
	// 1. iNumPolyCount만큼의 폴리곤 개수가 존재한다.
	// 2. iNumFace만큼 iPoly번째 폴리곤에 페이스가 존재한다.
	// 3. Dx에서 표현하기 위해 무조건 정점3개로 만들어 버텍스 리스트에 집어 넣는다.
	for (int iPoly = 0; iPoly < iNumPolyCount; iPoly++)
	{
		int iPolySize = pFbxMesh->GetPolygonSize(iPoly);
		iNumFace = iPolySize - 2;

		int iSubMtrl = 0;
		if (iNumMtrl >= 1 && MaterialSet[0] != nullptr)
		{
			// 레이어에서 머테리얼 정보를 얻어서 MaterialSet에 저장후 
			// 해당 폴리곤이 몇번째 머테리얼을 사용하는지 알아낸다
			iSubMtrl = GetSubMaterialIndex(iPoly, MaterialSet[0]);
		}
		for (int iFace = 0; iFace < iNumFace; iFace++)
		{
			// 1  2
			// 0  3
			// (max)021,032 ->  (dx)012, 023
			int VertexIndex[3] = { 0, iFace + 2, iFace + 1 };
				 
			int CornerIndex[3]; // 전체 매쉬에서 몇번째 버텍스인지 나타낸다
			CornerIndex[0] = pFbxMesh->GetPolygonVertex(iPoly, 0);
			CornerIndex[1] = pFbxMesh->GetPolygonVertex(iPoly, iFace+2);
			CornerIndex[2] = pFbxMesh->GetPolygonVertex(iPoly, iFace+1);

			// uv
			int u[3];
			u[0] = pFbxMesh->GetTextureUVIndex(iPoly, 0);
			u[1] = pFbxMesh->GetTextureUVIndex(iPoly, iFace + 2);
			u[2] = pFbxMesh->GetTextureUVIndex(iPoly, iFace + 1);

			for (int iIndex = 0; iIndex < 3; iIndex++)
			{
				int DCCIndex = CornerIndex[iIndex];
				TVertex tVertex;	
				// Max(x,z,y) ->(dx)x,y,z    		
				auto v = geom.MultT(pVertexPositions[DCCIndex]);
				tVertex.p.x = v.mData[0];
				tVertex.p.y = v.mData[2];
				tVertex.p.z = v.mData[1];

				
				if (VertexUVSet.size() > 0)
				{
					FbxLayerElementUV* pUVSet = VertexUVSet[0];
					FbxVector2 uv;
					ReadTextureCoord(
						pFbxMesh,
						pUVSet,
						DCCIndex,
						u[iIndex],
						_Out_ uv);
					tVertex.t.x = uv.mData[0];
					tVertex.t.y = 1.0f - uv.mData[1];
				}

				FbxColor color = FbxColor(1, 1, 1, 1);
				if (VertexColorSet.size() > 0)
				{
					color = ReadColor(pFbxMesh,
						VertexColorSet.size(),
						VertexColorSet[0],
						DCCIndex,
						iBasePolyIndex + VertexIndex[iIndex]);
				}
				tVertex.c.x = color.mRed;
				tVertex.c.y = color.mGreen;
				tVertex.c.z = color.mBlue;
				tVertex.c.w = pObject->m_iIndex;

				if (VertexNormalSets.size() <=0)
				{
					FbxVector4 normal = ReadNormal(pFbxMesh,
						DCCIndex,
						iBasePolyIndex + VertexIndex[iIndex]);
					normal = normalMatrix.MultT(normal);
					normal.Normalize();
					tVertex.n.x = normal.mData[0]; // x
					tVertex.n.y = normal.mData[2]; // z
					tVertex.n.z = normal.mData[1]; // y		
					D3DXVec3Normalize(&tVertex.n, &tVertex.n);
				}
				else
				{
					// Store vertex normal
					FbxVector4 finalNorm = ReadNormal(pFbxMesh, VertexNormalSets.size(),
						VertexNormalSets[0],
						DCCIndex,
						iBasePolyIndex + VertexIndex[iIndex]);// dwTriangleIndex * 3 + dwCornerIndex);
					finalNorm.mData[3] = 0.0;
					finalNorm = normalMatrix.MultT(finalNorm);
					finalNorm.Normalize();
					tVertex.n.x = finalNorm.mData[0];
					tVertex.n.y = finalNorm.mData[2];
					tVertex.n.z = finalNorm.mData[1];	
					D3DXVec3Normalize(&tVertex.n, &tVertex.n);
				}


				// 가중치
				TVertexIW iwVertex;
				if (pObject->m_bSkinned)
				{
					TWeight* weight = &pObject->m_WeightList[DCCIndex];
					for (int i = 0; i < 4; i++)
					{
						iwVertex.i[i] = weight->Index[i];
						iwVertex.w[i] = weight->Weight[i];
					}
				}
				else
				{
					// 일반오브젝트 에니메이션을 스키닝 케릭터 화 작업.
					iwVertex.i[0] = pObject->m_iIndex;
					iwVertex.w[0] = 1.0f;
				}
				//pObject->m_VertexList.push_back(tVertex);//36
				pObject->m_pSubVertexList[iSubMtrl].push_back(tVertex);
				pObject->m_pSubIWVertexList[iSubMtrl].push_back(iwVertex);
			}
		}

		iBasePolyIndex += iPolySize;
	}
	
}
bool	TFbxImporter::CreateConstantBuffer(ID3D11Device* pDevice)
{
	HRESULT hr;
	//gpu메모리에 버퍼 할당(원하는 할당 크기)
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
	bd.ByteWidth = sizeof(TBoneWorld);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	//D3D11_SUBRESOURCE_DATA sd;
	//ZeroMemory(&sd, sizeof(D3D11_SUBRESOURCE_DATA));
	//sd.pSysMem = &m_matBoneArray;
	
	// 매프레임마다 사용하는 데이터가 다르기때문에 할당만하고 sd데이터를 넘기지는 않음
	if (FAILED(hr = pDevice->CreateBuffer(&bd, NULL,
		&m_pBoneCB)))
	{
		return false;
	}
	return true;
}

bool	TFbxImporter::Init()
{
	m_pFbxManager = FbxManager::Create();
	m_pFbxImporter = FbxImporter::Create(m_pFbxManager, "");
	m_pFbxScene = FbxScene::Create(m_pFbxManager, "");	
	
	FbxAxisSystem	 m_SceneAxisSystem = m_pFbxScene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem::MayaZUp.ConvertScene(m_pFbxScene);
	m_SceneAxisSystem = m_pFbxScene->GetGlobalSettings().GetAxisSystem();

	FbxSystemUnit	m_SceneSystemUnit = m_pFbxScene->GetGlobalSettings().GetSystemUnit();
	if (m_SceneSystemUnit.GetScaleFactor() != 1.0f)
	{
		FbxSystemUnit::cm.ConvertScene(m_pFbxScene);
	}


	return true;
}
bool	TFbxImporter::Frame()
{
	return true;
}
bool	TFbxImporter::Render()
{
	return true;
}
bool	TFbxImporter::Release()
{
	if(m_pBoneCB)m_pBoneCB->Release();
	m_pBoneCB = nullptr;
	for (int iObj = 0; iObj < m_DrawList.size(); iObj++)
	{
		m_DrawList[iObj]->Release();
	}
	if(m_pFbxScene)m_pFbxScene->Destroy();
	if (m_pFbxImporter)m_pFbxImporter->Destroy();
	if (m_pFbxManager)m_pFbxManager->Destroy();
	m_pFbxScene = nullptr;
	m_pFbxImporter = nullptr;
	m_pFbxManager = nullptr;
	return true;
}

bool	TFbxImporter::Load(ID3D11Device* pd3dDevice, std::wstring filename)
{
	if (Load(to_wm(filename).c_str()))
	{
		CreateConstantBuffer(pd3dDevice);
		TShader* pVShader = I_Shader.CreateVertexShader(pd3dDevice, L"../../data/shader/Character.hlsl", "VS");
		TShader* pPShader = I_Shader.CreatePixelShader(pd3dDevice, L"../../data/shader/Character.hlsl", "PS");
		for (int iObj = 0; iObj < m_DrawList.size(); iObj++)
		{
			m_DrawList[iObj]->Init();
			m_DrawList[iObj]->m_pVShader = pVShader;
			m_DrawList[iObj]->m_pPShader = pPShader;
			if (!m_DrawList[iObj]->Create(pd3dDevice,	m_pContext))
			{
				return false;
			}
		}
	}
	return true;
}