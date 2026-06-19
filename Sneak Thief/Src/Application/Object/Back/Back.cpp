#include "Back.h"

void Back::Init()
{
	m_polygon = std::make_shared<KdSquarePolygon>();
	m_polygon->SetMaterial("Asset/Textures/Back.png");
	//300,100,1で拡大
	Math::Matrix scalemat = Math::Matrix::CreateScale(300.0f, 100.0f, 1.0f);
	//0,0,50に移動
	Math::Matrix transmat = Math::Matrix::CreateTranslation(0.0f, -20.0f, 50.0f);

	// 行列合成（基本的な合成はSRT）
	m_mWorld = scalemat * transmat;
}

void Back::DrawLit()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}
