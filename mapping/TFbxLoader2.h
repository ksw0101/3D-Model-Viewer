#pragma once
#include "TObject3D.h"
#include <fbxsdk.h>
// 캐릭터 애니메이션과 강체애니메이션의 가장 큰차이는
// 강체 애니메이션은 하나의 월드 행렬을 사용해서 모두 다 같이 곱하지만
// 캐릭터 애니메이션은 매 버택스마다 다른 월드행렬을 곱할 수가 있어야한다.
struct TTrack
{
	UINT iFrame;
	TMatrix matTrack;
};
struct TScene
{
	UINT   iStart;
	UINT   iEnd;
	UINT   iFrameSpeed;
};
class TFbxModel : public TObject3D
{
public:
	int		 m_iIndex = -1;
	TMatrix m_matLocal;
	TMatrix m_matAnim; // 월드 변환 행렬
	FbxNode* m_pFbxParent = nullptr;
	FbxNode* m_pFbxNode = nullptr;
	TFbxModel* m_pParentObj = nullptr;
	std::vector<std::wstring> m_szTexFileList;
	using TSubVertex = std::vector<TVertex>;
	std::vector<TSubVertex> m_pSubVertexList;
	std::vector<ID3D11Buffer*>	m_pVBList;
	std::vector<TTexture*>	m_pTextureList;
	
	std::wstring m_szTexFileName;

	std::vector<TTrack>	m_AnimTrack;
public:
	virtual bool    SetVertexData() override;
	virtual bool	CreateVertexBuffer() override;
	virtual bool	SetIndexData() override;
	virtual bool	PostRender() override;
	virtual bool	Release() override;
};
class TFbxLoader : public TObject3D
{
public:
	TScene		m_Scene;
	float m_fDir = 1.0f;
	float m_fTime = 0.0f;
	float m_fSpeed = 1.0f;
	TBoneWorld	  m_matBoneArray;
public:
	FbxManager*		m_pFbxManager;
	FbxImporter*	m_pFbxImporter;
	FbxScene*		m_pFbxScene;
	FbxNode*		m_pRootNode;
	std::vector<TFbxModel*>	m_DrawList;
	std::vector<TFbxModel*>	m_TreeList;
	ID3D11Buffer* m_pBoneCB = nullptr;
public:
	virtual bool Load(std::string filename);
	//노드에서 메쉬정보를 읽어오는 재귀함수
	virtual void PreProcess(FbxNode* Node, TFbxModel* fbxParent = nullptr);
	// 매쉬 안의 레이어 정보(UV, Materials), 
	// 버택스 Position 정보를 TFbxModel형태에 맞게 재가공 
	virtual void ParseMesh(TFbxModel* pObject);
	
private:
	void ReadTextureCoord(FbxMesh* pFbxMesh, FbxLayerElementUV* pUVSet,	int vertexIndex, int uvIndex, FbxVector2& uv);
	std::string ParseMaterial(FbxSurfaceMaterial* pMtrl);
	FbxColor ReadColor(const FbxMesh* mesh, DWORD dwVertexColorCount,
		FbxLayerElementVertexColor* pVertexColorSet,
		DWORD dwDCCIndex, DWORD dwVertexIndex);
	FbxVector4 ReadNormal(const FbxMesh* mesh, int controlPointIndex, int vertexCounter);
	int GetSubMaterialIndex(int iPoly, FbxLayerElementMaterial* pMaterialSetList);
public:
	TMatrix		DxConvertMatrix(TMatrix m);
	TMatrix		ConvertMatrix(FbxMatrix& m);
	TMatrix     ConvertAMatrix(FbxAMatrix& m);
	void		ParseAnimation();
public:

	virtual bool Init()	;
	virtual bool Frame();
	virtual bool Render();
	virtual bool Release();
	
	virtual bool	CreateConstantBuffer(ID3D11Device* pDevice);

};

