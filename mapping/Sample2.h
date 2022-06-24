#pragma once
#include "TCore.h"
#include "TFbxLoader.h"
class TViewRT
{

};
class TViewDs
{

};
class Sample : public TCore
{
	std::vector<TFbxLoader>		m_FbxObj;
public:

	virtual bool Init()		override;
	virtual bool Frame()	override;
	virtual bool Render()	override;
	virtual bool Release() override;

};
//3.26
