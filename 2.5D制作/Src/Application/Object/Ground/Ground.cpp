#include "Ground .h"
void Ground::Init()
{
	// m_modelを実体化
	m_model = std::make_shared<KdModelData>();
	m_model->Load("Asset/Models/Stage/Stage.gltf");

	Math::Vector3 scale(1.0f, 1.0f, 1.0f);
	Math::Matrix scalemat = Math::Matrix::CreateScale(scale);
	Math::Matrix rotatemat = Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(-10));
	Math::Matrix transmat = Math::Matrix::CreateTranslation(0.0f, 0.0f, 4.0f);

	// ↓全オブジェクトに必要なので親クラスがメンバー変数として保持
	m_mWorld = scalemat * rotatemat * transmat;


	// 「当てられる側」の処理======

	// 当たり判定を付けたいから実体化
	m_pCollider = std::make_unique<KdCollider>();
	// モデルの形状で当たり判定を登録
	m_pCollider->RegisterCollisionShape
	(
		"GroundCollision",				// 識別用の名前
		m_model,						// 登録したいモデルの形状
		KdCollider::Type::TypeGround	//当たり判定の種類
	);
	// ============================

}

void Ground::DrawLit()
{
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);
}
