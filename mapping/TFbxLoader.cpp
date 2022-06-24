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
// TMatrix는 행백터 행렬, TFbx는 열백터용 행렬이므로 전치.
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
// FbxMatrix를 double형 포인터로 변환해서 float형 포인터로 형변환된 TMatrix에 대입
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
	FbxTime start = LocalTimeSpan.GetStart();
	FbxTime end = LocalTimeSpan.GetStop();
	// start부터 end까지 시간이 흐르는동안 그 시간의 간격을 설정한다.
	FbxTime Duration = LocalTimeSpan.GetDuration();
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
			// 특정시간대의 최종변환 행렬을 구한다. 
			// 본인의 행렬과 부모의 행렬까지 모두 결합되어있음.
			FbxAMatrix matGlobal = m_TreeList[iObj]->m_pFbxNode->EvaluateGlobalTransform(time);
			tTrack.iFrame = t;
			tTrack.matTrack = DxConvertMatrix(ConvertAMatrix(matGlobal));
			// 트리리스트마다 현재 시간에 해당하는 모든 애니메이션 트랙을 넣어준다.
			m_TreeList[iObj]->m_AnimTrack.push_back(tTrack);
		}
	}
}
// 노드에서 메쉬정보를 읽어오는 재귀함수
// 더미데이터의 피벗을 읽기 위해서 노드가 없어도 읽는 방식으로 변경
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
		// 트리의 모든 로컬 변환 행렬을 제거
		m_TreeList.push_back(fbx); 
	}
	// 카메라, 라이트, 매쉬, 애니메이션 in Node
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
// 임포터 -> 씬 -> 루트노드 -> [노드 -> 매쉬확인 -> 매쉬 
// -> 오브젝트리스트] -> node에서 GetMesh
bool TFbxLoader::Load(std::string filename)
{

	bool bReturn;
	bReturn = m_pFbxImporter->Initialize(filename.c_str());
	bReturn = m_pFbxImporter->Import(m_pFbxScene);
	m_pRootNode = m_pFbxScene->GetRootNode();
	// 노드에서 매쉬 정보를 읽어오는 재귀함수
	PreProcess(m_pRootNode, nullptr);
	ParseAnimation();

	// 오브젝트의 위치 값을 채운다
	for (int iObj = 0; iObj < m_DrawList.size(); iObj++)
	{
		// 행렬을 읽어오는 것을 전부 뺐다. 전부 PreProcess에서 처리로 대체
		ParseMesh(m_DrawList[iObj]);
	}
	//m_matBoneArray.resize(m_TreeList.size());	

	return true;
}
// 1. 매쉬 안에 레이어 정보(UV, Materials)를 확인 
// 2. 매쉬안의 정보 중 위치값을 오브젝트3d에 버택스리스트에 넣는다.
void TFbxLoader::ParseMesh(TFbxModel* pObject)
{

	FbxMesh* pFbxMesh = pObject->m_pFbxNode->GetMesh();
	// 매쉬가 있는 경우만 적용에서 매쉬가 없는 경우도 적용으로 변경
	
	// 기하행렬(초기 정점 위치를 변환할 때 사용)
	// transform 로컬 정점을 만들기 위해 사용
	// fbx는 행우선 방식
	FbxAMatrix geom;
	// 소스의 피벗에서 부터의 SRT 정보
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

	// 레이어 (1번에 랜더링, 여러번에 걸쳐서 랜더링 개념)
	// 레이어는 일반적으로 하나이나 여러개가 들어갈 수도있다.
	std::vector<FbxLayerElementUV*> VertexUVSet;
	std::vector<FbxLayerElementVertexColor*> VertexColorSet;
	// 여러번에 걸쳐서 랜더링 하는 것이 아니다
	// 레이어에서 가지고 오는 머테리얼임. 노드에서 가지고오는 머테리얼과 구분
	// 머테리얼이 하나일땐 노드에서 가지고오나 레이어에서 가지고 오나 같은 정보
	std::vector<FbxLayerElementMaterial*> MaterialSet;
	// 레이어에선 포지션 정보가 없다
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
			//uv가 존재한다면 마테리얼 존재
		if (pFbxLayer->GetMaterials() != nullptr)
		{
			MaterialSet.push_back(pFbxLayer->GetMaterials());
		}
	}
	// 머테리얼은 매쉬단위가 아니라 페이스 단위로 적용된다.
	// 그래서 mesh가아니라 node에서 머테리얼 정보를 얻어오는듯
	// 같은 텍스쳐를 사용하는 폴리곤끼리 저장
	int iNumMaterials = pObject->m_pFbxNode->GetMaterialCount();

	for (int iMtrl = 0; iMtrl < iNumMaterials; iMtrl++)
	{
		// 텍스쳐의 이름을 가지고 올수 있다.
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
	//pObject->m_szTexFileName = pObject->m_szTexFileList[0]; // 이거 왜씀?
	if (iNumMaterials > 0) // 마테리얼이 존재하는 경우
	{
		pObject->m_pSubVertexList.resize(iNumMaterials);
	}
	// 마테리얼이 존재하지 않더라도 하나는 할당해야지 메모리 참조 오류가 나지 않는ㄷ다.
	else
	{
		pObject->m_pSubVertexList.resize(1);
	}
	int iBasePolyIndex = 0;
	// 폴리곤(다각형)의 수를 가져옴 (6)
	int iPolyCount = pFbxMesh->GetPolygonCount();
	// 제어점 수 (8)
	int iVertexCount = pFbxMesh->GetControlPointsCount();
	// 위치 정보 (8개의 정점정보)
	FbxVector4* pVertexPositions = pFbxMesh->GetControlPoints();
	int* pIndex = pFbxMesh->GetPolygonVertices();
	int iNumFace = 0;

	// 1. iPolyCount만큼의 폴리곤 개수가 존재한다.
	// 2. iNumFace만큼 iPoly번째 폴리곤에 페이스가 존재한다.
	// 3. Dx에서 표현하기 위해 무조건 정점3개로 만들어 버텍스 리스트에 집어 넣는다.
	for (int iPoly = 0; iPoly < iPolyCount; iPoly++) 
	{
		// iPoly 번째 폴리곤의 정점의 개수를 가져온다
		// iPolySize = 3 :: 삼각형
		// iPolySize = 4 :: 사각형
		int iPolySize = pFbxMesh->GetPolygonSize(iPoly);

		// 폴리곤 1개당 페이스의 갯수 (3-2 : 1 삼각형, 4-2 : 2 사각형 이므로
		iNumFace = iPolySize - 2;

		// 레이어에서 머테리얼 정보를 얻어서 MaterialSet에 저장후 
		// 해당 폴리곤이 몇번째 머테리얼을 사용하는지 알아낸다
		// 머테리얼이 0이라고해서 해당 서브 머테리얼도 0이라는 보장이 없기때문에 재정의
		int iSubMtrl = 0;
		if (iNumMaterials >= 1 && MaterialSet[0] != nullptr)
		{
			iSubMtrl = GetSubMaterialIndex(iPoly, MaterialSet[0]);
		}
		for (int iFace = 0; iFace < iNumFace; iFace++)  // 페이스에대한 반복문
		{
			// fbx는 dx와 앞면이 반대이기 때문에 반시계로 설정
			// 0 1 ->	0 2
			//   2		  1
			int VertexIndex[3] = { 0, iFace + 2, iFace + 1 };
			// case DX)
			// 버텍스가 버텍스 버퍼에 들어가 있다.
			// 인덱스는 인덱스 버퍼에 들어가 있다.
			// case Max)
			// 포지션 인덱스 따로
			// 포지션 리스트 따로
			// 텍스쳐 좌표따로
			// 텍스쳐 인덱스 좌표따로
			int CornerIndex[3];
			CornerIndex[0] = pFbxMesh->GetPolygonVertex(iPoly, VertexIndex[0]);
			CornerIndex[1] = pFbxMesh->GetPolygonVertex(iPoly, VertexIndex[1]);
			CornerIndex[2] = pFbxMesh->GetPolygonVertex(iPoly, VertexIndex[2]);

			for (int iIndex = 0; iIndex < 3; iIndex++) // 정점에 대한 반복문
			{
				TVertex tVertex;
				// 위 Index에서 구한 순서대로 정보를 가지고 온다.
				FbxVector4 v = pVertexPositions[CornerIndex[iIndex]];
				v = geom.MultT(v);
				tVertex.p.x = v.mData[0];
				tVertex.p.y = v.mData[2];
				tVertex.p.z = v.mData[1];
				//6개의 폴리곤 -> 2개의 페이스 -> 3개의 인덱스

				int u[3];
				u[0] = pFbxMesh->GetTextureUVIndex(iPoly, VertexIndex[0]);
				u[1] = pFbxMesh->GetTextureUVIndex(iPoly, VertexIndex[1]);
				u[2] = pFbxMesh->GetTextureUVIndex(iPoly, VertexIndex[2]);

				// 텍스쳐 정보가 있다면 t추가
				if (VertexUVSet.size() > 0)
				{
					FbxLayerElementUV* pUVSet = VertexUVSet[0];
					FbxVector2 uv;
					// [out]uv
					ReadTextureCoord(pFbxMesh, pUVSet, CornerIndex[iIndex], u[iIndex],_Out_ uv);
					tVertex.t.x = uv.mData[0];
					// 다이렉트 x는 텍스쳐가 좌측 상단이 0, 0 이지만
					// fbx는 텍스쳐가 좌측 하단이 0, 0이다.
					tVertex.t.y = 1.0f - uv.mData[1];
				}

				// 정점 컬러 추가
				// 디폴트 값으로 흰색인 이유는 곱했을때 색의 변화를 주지 않기 위해서
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
				normal = normalMatrix.MultT(normal); // 열우선 방식, 이동행렬을 곱해준다
				tVertex.n.x = normal.mData[0];
				tVertex.n.y = normal.mData[2];
				tVertex.n.z = normal.mData[1];

				// 머테리얼이 단일일 경우
				//pObject->m_VertexList.push_back(tVertex);// 36개가 들어간다
				// 머테리얼이 여러개일경우 같은 머테리얼을 공유하는 버텍스끼리 묶음
				pObject->m_pSubVertexList[iSubMtrl].push_back(tVertex);
			}
		}
		iBasePolyIndex += iPolySize;

	}

}
bool	TFbxLoader::CreateConstantBuffer(ID3D11Device* pDevice)
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

	// 매프레임마다 사용하는 데이터가 다르기때문에 할당만하고 데이터를 넘기지는 않음
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
