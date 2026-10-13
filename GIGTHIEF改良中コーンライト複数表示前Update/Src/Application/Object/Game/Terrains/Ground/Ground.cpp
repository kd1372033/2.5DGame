#include "Ground .h"

void Ground::Init()
{
	Math::Vector3 scale(1.0f, 1.0f, 1.0f);
	Math::Matrix scalemat = Math::Matrix::CreateScale(scale);
	Math::Matrix rotatemat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(90));
	Math::Matrix transmat = Math::Matrix::CreateTranslation(0.0f, 0.0f, -1.0f);

	m_mWorld = scalemat * rotatemat * transmat;

	m_pCollider = std::make_unique<KdCollider>();

	// --- 床のロードとコライダー登録 ---
	m_front = std::make_shared<KdModelData>();
	if (m_front->Load("Asset/Models/Stage/Front.gltf")) {
		m_pCollider->RegisterCollisionShape("Front", m_front, KdCollider::TypeGround);
	}

	m_stage1 = std::make_shared<KdModelData>();
	if (m_stage1->Load("Asset/Models/Stage/Stage1.gltf")) {
		m_pCollider->RegisterCollisionShape("Stage1", m_stage1, KdCollider::TypeGround);
	}

	m_stage2 = std::make_shared<KdModelData>();
	if (m_stage2->Load("Asset/Models/Stage/Stage2.gltf")) {
		m_pCollider->RegisterCollisionShape("Stage2", m_stage2, KdCollider::TypeGround);
	}

	m_stage3 = std::make_shared<KdModelData>();
	if (m_stage3->Load("Asset/Models/Stage/Stage3.gltf")) {
		m_pCollider->RegisterCollisionShape("Stage3", m_stage3, KdCollider::TypeGround);
	}

	// --- 壁のロードとコライダー登録 ---
	m_frontwall = std::make_shared<KdModelData>();
	if (m_frontwall->Load("Asset/Models/Stage/Frontwall.gltf")) {
		m_pCollider->RegisterCollisionShape("Frontwall", m_frontwall, KdCollider::TypeGround | KdCollider::TypeBump);
	}

	m_stagewall1 = std::make_shared<KdModelData>();
	if (m_stagewall1->Load("Asset/Models/Stage/Stagewall1.gltf")) {
		m_pCollider->RegisterCollisionShape("Stagewall1", m_stagewall1, KdCollider::TypeGround | KdCollider::TypeBump);
	}

	m_stagewall2 = std::make_shared<KdModelData>();
	if (m_stagewall2->Load("Asset/Models/Stage/Stagewall2.gltf")) {
		m_pCollider->RegisterCollisionShape("Stagewall2", m_stagewall2, KdCollider::TypeGround | KdCollider::TypeBump);
	}

	m_stagewall3 = std::make_shared<KdModelData>();
	if (m_stagewall3->Load("Asset/Models/Stage/Stagewall3.gltf")) {
		m_pCollider->RegisterCollisionShape("Stagewall3", m_stagewall3, KdCollider::TypeGround | KdCollider::TypeBump);
	}
}

void Ground::Update()
{
	auto spPlayer = m_wpPlayer.lock();
	if (!spPlayer) return;

	float playerZ = spPlayer->GetPos().z;

	// 壁の半透明フラグ（SetColorEnable）のリセット
	m_isFrontWallColorEnable = false;
	m_isStageWall1ColorEnable = false;
	m_isStageWall2ColorEnable = false;
	m_isStageWall3ColorEnable = false;

	// プレイヤーのZ座標に応じて「現在いる部屋」と「手前で透けさせる壁」を更新
	if (playerZ <= -2.25f)
	{
		m_currentRoomIndex = 0; // 1部屋目の床が該当
	}
	else if (playerZ <= 0.2f)
	{
		m_currentRoomIndex = 1; // 2部屋目の床が該当
		m_isFrontWallColorEnable = true; // 1部屋目の壁を透けさせる
	}
	else if (playerZ <= 2.7f)
	{
		m_currentRoomIndex = 2; // 3部屋目の床が該当
		m_isFrontWallColorEnable = true;
		m_isStageWall1ColorEnable = true;
	}
	else
	{
		m_currentRoomIndex = 3; // 4部屋目の床が該当
		m_isFrontWallColorEnable = true;
		m_isStageWall1ColorEnable = true;
		m_isStageWall2ColorEnable = true;
	}
}

void Ground::DrawLit()
{
	// 描画補助ラムダ関数
	auto drawModel = [](std::shared_ptr<KdModelData>& model, const Math::Matrix& world, bool isColorEnable)
		{
			if (!model) return;

			// シェーダー側へフラグを設定
			KdShaderManager::Instance().m_StandardShader.SetColorEnable(isColorEnable);

			// 半透明処理が必要な場合（isColorEnable == true）はブレンドステートを切り替え
			if (isColorEnable)
			{

				KdShaderManager::Instance().m_StandardShader.DrawModel(*model, world);
			}
			else
			{
				KdShaderManager::Instance().m_StandardShader.DrawModel(*model, world);
			}
		};

	// 1. 床の描画（床は常に SetColorEnable(false) で不透明描画）
	drawModel(m_front, m_mWorld, false);
	drawModel(m_stage1, m_mWorld, false);
	drawModel(m_stage2, m_mWorld, false);
	drawModel(m_stage3, m_mWorld, false);

	// 2. 壁の描画（フラグに応じて SetColorEnable を切り替え）
	drawModel(m_frontwall, m_mWorld, m_isFrontWallColorEnable);
	drawModel(m_stagewall1, m_mWorld, m_isStageWall1ColorEnable);
	drawModel(m_stagewall2, m_mWorld, m_isStageWall2ColorEnable);
	drawModel(m_stagewall3, m_mWorld, m_isStageWall3ColorEnable);

	// シェーダーのフラグをデフォルト（false）に戻しておく
	KdShaderManager::Instance().m_StandardShader.SetColorEnable(false);
}

// 2. 他の部屋の半透明壁の描画（半透明パス）
void Ground::DrawUnLit()
{
	//// アルファブレンドと ZWriteDisable（深度書き込みオフ）を有効化
	//KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	//KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);
	//KdShaderManager::Instance().m_StandardShader.SetColorEnable(true);
	//if (m_frontColor.w < 1.0f && m_front)  KdShaderManager::Instance().m_StandardShader.DrawModel(*m_front, m_mWorld, m_frontColor);
	//if (m_stage1Color.w < 1.0f && m_stage1) KdShaderManager::Instance().m_StandardShader.DrawModel(*m_stage1, m_mWorld, m_stage1Color);
	//if (m_stage2Color.w < 1.0f && m_stage2) KdShaderManager::Instance().m_StandardShader.DrawModel(*m_stage2, m_mWorld, m_stage2Color);
	//if (m_stage3Color.w < 1.0f && m_stage3) KdShaderManager::Instance().m_StandardShader.DrawModel(*m_stage3, m_mWorld, m_stage3Color);

	//KdShaderManager::Instance().UndoDepthStencilState();
	//KdShaderManager::Instance().UndoBlendState();
}