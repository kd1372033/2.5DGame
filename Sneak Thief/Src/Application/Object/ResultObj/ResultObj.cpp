#include "ResultObj.h"
#include "../../Scene/SceneManager.h"

void ResultObj::Init()
{
	bool isClear = SceneManager::Instance().GetClearFlag();

	if (isClear)
	{
		// CONGRATULATION ! の範囲を切り取る設定
		// m_polygon->SetUVRect( ... );
	}
	else
	{
		// OH...MY...GOD... の範囲を切り取る設定
		// m_polygon->SetUVRect( ... );
	}
}

void ResultObj::Update()
{

}

void ResultObj::PostUpdate()
{

}

void ResultObj::DrawSprite()
{

}
