#include "GameObj.h"

void Furniture::Init()
{
	m_pCollider = std::make_unique<KdCollider>();

	Math::Vector3 scale(1.0f, 1.0f, 1.0f);
	Math::Matrix scalemat = Math::Matrix::CreateScale(scale);
	Math::Matrix rotatemat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(90));
	Math::Matrix transmat = Math::Matrix::CreateTranslation(0.0f, 0.0f, -1.0f);

	m_mWorld = scalemat * rotatemat * transmat;

	// --- 家具モデル1のロードとコライダー登録 ---
	m_model1 = std::make_shared<KdModelData>();
	if (m_model1->Load("Asset/Models/Kagu/Kagu1.gltf")) {
		m_pCollider->RegisterCollisionShape("Furniture1", m_model1, KdCollider::TypeGround | KdCollider::TypeBump);
	}

	// --- 家具モデル2のロードとコライダー登録 ---
	m_model2 = std::make_shared<KdModelData>();
	if (m_model2->Load("Asset/Models/Kagu/Kagu2.gltf")) {
		m_pCollider->RegisterCollisionShape("Furniture2", m_model2, KdCollider::TypeGround | KdCollider::TypeBump);
	}

	// --- 家具モデル3のロードとコライダー登録 ---
	m_model3 = std::make_shared<KdModelData>();
	if (m_model3->Load("Asset/Models/Kagu/Kagu3.gltf")) {
		m_pCollider->RegisterCollisionShape("Furniture3", m_model3, KdCollider::TypeGround | KdCollider::TypeBump);
	}
}

void Furniture::Update()
{
	auto spPlayer = m_wpPlayer.lock();
	if (!spPlayer) return;

	float playerZ = spPlayer->GetPos().z;

	// 半透明フラグ（SetColorEnable）のリセット
	m_isModel1ColorEnable = false;
	m_isModel2ColorEnable = false;
	m_isModel3ColorEnable = false;

	// Groundと同じ判定閾値で家具を透けさせる
	if (playerZ <= -2.25f)
	{
		// 1部屋目にいるとき
	}
	else if (playerZ <= 0.2f)
	{
		m_isModel1ColorEnable = false;
		m_isModel2ColorEnable = false;
		m_isModel3ColorEnable = false;
	}
	else if (playerZ <= 2.7f)
	{
		m_isModel1ColorEnable = true;
		m_isModel2ColorEnable = false;
		m_isModel3ColorEnable = false;
	}
	else
	{
		m_isModel1ColorEnable = true;
		m_isModel2ColorEnable = true;
		m_isModel3ColorEnable = false;
	}
}

void Furniture::DrawLit()
{
	// Ground と完全に同じラムダ関数構造
	auto drawModel = [](std::shared_ptr<KdModelData>& model, const Math::Matrix& world, bool isColorEnable)
		{
			if (!model) return;

			// シェーダー側へフラグを設定
			KdShaderManager::Instance().m_StandardShader.SetColorEnable(isColorEnable);

			if (isColorEnable)
			{
				KdShaderManager::Instance().m_StandardShader.DrawModel(*model, world);
			}
			else
			{
				KdShaderManager::Instance().m_StandardShader.DrawModel(*model, world);
			}
		};

	// 各モデルの描画呼び出し
	drawModel(m_model1, m_mWorld, m_isModel1ColorEnable);
	drawModel(m_model2, m_mWorld, m_isModel2ColorEnable);
	drawModel(m_model3, m_mWorld, m_isModel3ColorEnable);

	// シェーダーのフラグをデフォルト（false）に戻しておく
	KdShaderManager::Instance().m_StandardShader.SetColorEnable(false);
}