#include "Visibility.h"

void Visibility::Init()
{
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();
	// m_modelを実体化
	m_model = std::make_shared<KdModelData>();
	m_model->Load("Asset/Models/EnemyVisibility/EnemyVisibility/EnemyVisibility.gltf");


	// 「当てられる側」の処理======

	// 当たり判定を付けたいから実体化
	m_pCollider = std::make_unique<KdCollider>();
	// モデルの形状で当たり判定を登録
	m_pCollider->RegisterCollisionShape
	(
		"EnemyVisibility",				// 識別用の名前
		m_model,						// 登録したいモデルの形状
		KdCollider::Type::TypeEvent		//当たり判定の種類
	);
	// ============================

}

void Visibility::Update()
{
	Math::Matrix scalemat = Math::Matrix::CreateScale(1.0);
	Math::Matrix rotatemat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_deg));
	Math::Matrix transmat = Math::Matrix::CreateTranslation(0.0f, 0.5f, 0.0f);

	// ↓全オブジェクトに必要なので親クラスがメンバー変数として保持
	m_mWorld = scalemat * rotatemat * transmat;
}

void Visibility::PostUpdate()
{
	// =======================
// 当たり判定（Box）
// =======================
	KdCollider::BoxInfo box;
	box.m_Obox.Center = GetPos();

	//m_pDebugWire->AddDebugBox()
}

void Visibility::DrawLit()
{
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);
}
