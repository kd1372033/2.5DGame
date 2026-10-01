#include "Ground.h"

void Ground::Init()
{
	// モデルの読み込み
	m_model = std::make_shared<KdModelData>();
	m_model->Load("Asset/Models/Ground/Ground.gltf");

	// 1. 床の設定
	Math::Matrix scaleMat = Math::Matrix::CreateScale(100.0f);
	m_mWorld = scaleMat;

	// 2. 壁の設定
	Math::Matrix wallRotMat = Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(90.0f));
	Math::Matrix wallScaleMat = Math::Matrix::CreateScale(20.0f);
	m_wallWorld = wallScaleMat * wallRotMat;

	// --- 3. コライダーの設定 ---
	if (m_model)
	{
		// 地面用コライダー
		m_pCollider = std::make_unique<KdCollider>();
		// メッシュ（ポリゴン判定）として登録
		m_pCollider->RegisterCollisionShape("GroundModel", m_model, KdCollider::TypeGround);

		// 壁用コライダー
		m_pWallCollider = std::make_unique<KdCollider>();
		// メッシュ（ポリゴン判定）として登録
		m_pWallCollider->RegisterCollisionShape("WallModel", m_model, KdCollider::TypeBump);
	}
}

bool Ground::Intersects(const KdCollider::RayInfo& rayInfo, std::list<KdCollider::CollisionResult>* pResults)
{
	bool isHit = false;

	// 1. RayInfoのタイプが TypeGround を含んでいる場合、床（m_mWorld）と判定
	if (rayInfo.m_type & KdCollider::TypeGround)
	{
		if (m_pCollider && m_pCollider->Intersects(rayInfo, m_mWorld, pResults))
		{
			isHit = true;
		}
	}

	// 2. RayInfoのタイプが TypeBump を含んでいる場合、壁（m_wallWorld）と判定
	if (rayInfo.m_type & KdCollider::TypeBump)
	{
		if (m_pWallCollider && m_pWallCollider->Intersects(rayInfo, m_wallWorld, pResults))
		{
			isHit = true;
		}
	}

	return isHit;
}

void Ground::DrawLit()
{
	if (m_model)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_wallWorld);
	}
}