#include "Ground .h"

void Ground::Init()
{
	Math::Vector3 scale(1.0f, 1.0f, 1.0f);
	Math::Matrix scalemat = Math::Matrix::CreateScale(scale);
	Math::Matrix rotatemat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(90));
	Math::Matrix transmat = Math::Matrix::CreateTranslation(0.0f, 0.0f, -1.0f);

	m_mWorld = scalemat * rotatemat * transmat;

	m_pCollider = std::make_unique<KdCollider>();

	// モデル読み込み
	m_front = std::make_shared<KdModelData>();
	if (m_front->Load("Asset/Models/Stagea/Front.gltf"))
	{
		m_pCollider->RegisterCollisionShape("Front", m_front, KdCollider::Type::TypeGround);
	}

	m_stage1 = std::make_shared<KdModelData>();
	if (m_stage1->Load("Asset/Models/Stagea/Stage1.gltf"))
	{
		m_pCollider->RegisterCollisionShape("Stage1", m_stage1, KdCollider::Type::TypeGround);
	}

	m_stage2 = std::make_shared<KdModelData>();
	if (m_stage2->Load("Asset/Models/Stagea/Stage2.gltf"))
	{
		m_pCollider->RegisterCollisionShape("Stage2", m_stage2, KdCollider::Type::TypeGround);
	}

	m_stage3 = std::make_shared<KdModelData>();
	if (m_stage3->Load("Asset/Models/Stagea/Stage3.gltf"))
	{
		m_pCollider->RegisterCollisionShape("Stage3", m_stage3, KdCollider::Type::TypeGround);
	}
}

void Ground::Update()
{
	// 1. 今いる部屋（明るい不透明）
	const Math::Color activeColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	// 2. カメラ〜プレイヤー間にある手前の壁（半透明スケルトン）
	const Math::Color frontColor = { 0.5f, 0.5f, 0.5f, 0.3f };

	// 3. プレイヤーより奥にある部屋（暗め・不透明）
	const Math::Color darkRoomColor = { 0.3f, 0.3f, 0.3f, 1.0f };

	auto spPlayer = m_wpPlayer.lock();
	if (!spPlayer) return;

	float playerZ = spPlayer->GetPos().z;

	// Z軸判定：プレイヤーより手前にある部屋はすべて frontColor（半透明）にする
	if (playerZ <= -2.25f)
	{
		// 1部屋目（一番手前）にいる時
		m_frontColor = activeColor;
		m_stage1Color = darkRoomColor;
		m_stage2Color = darkRoomColor;
		m_stage3Color = darkRoomColor;
	}
	else if (playerZ <= 0.2f)
	{
		// 2部屋目にいる時：1部屋目と手前の壁を半透明にする
		m_frontColor = frontColor;
		m_stage1Color = activeColor;
		m_stage2Color = darkRoomColor;
		m_stage3Color = darkRoomColor;
	}
	else if (playerZ <= 2.7f)
	{
		// 3部屋目にいる時：1〜2部屋目までの壁をすべて半透明にする
		m_frontColor = frontColor;
		m_stage1Color = frontColor; // ★不透明から半透明に変更
		m_stage2Color = activeColor;
		m_stage3Color = darkRoomColor;
	}
	else
	{
		// 4部屋目（一番奥）にいる時：1〜3部屋目の壁をすべて半透明にする
		m_frontColor = frontColor;
		m_stage1Color = frontColor; // ★不透明から半透明に変更
		m_stage2Color = frontColor; // ★不透明から半透明に変更
		m_stage3Color = activeColor;
	}
}

void Ground::DrawLit()
{
	// 描画用の内部関数（ラムダ）
	auto drawRoom = [](std::shared_ptr<KdModelData>& model, const Math::Matrix& world, const Math::Color& color, bool drawAlpha)
		{
			if (!model) return;

			bool isAlpha = (color.w < 1.0f);

			// 半透明を描くターンで、かつ対象が半透明カラーの場合
			if (drawAlpha && isAlpha)
			{
				KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
				KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

				KdShaderManager::Instance().m_StandardShader.DrawModel(*model, world, color);

				KdShaderManager::Instance().UndoDepthStencilState();
				KdShaderManager::Instance().UndoBlendState();
			}
			// 不透明を描くターンで、かつ対象が不透明カラーの場合
			else if (!drawAlpha && !isAlpha)
			{
				KdShaderManager::Instance().m_StandardShader.DrawModel(*model, world, color);
			}
		};

	// ★ Pass 1: まず「不透明な部屋（プレイヤーがいる部屋）」だけを普通に描画
	drawRoom(m_front, m_mWorld, m_frontColor, false);
	drawRoom(m_stage1, m_mWorld, m_stage1Color, false);
	drawRoom(m_stage2, m_mWorld, m_stage2Color, false);
	drawRoom(m_stage3, m_mWorld, m_stage3Color, false);

	// ★ Pass 2: その後に「半透明な部屋（手前の壁など）」を ZWriteDisable で上書き描画
	drawRoom(m_front, m_mWorld, m_frontColor, true);
	drawRoom(m_stage1, m_mWorld, m_stage1Color, true);
	drawRoom(m_stage2, m_mWorld, m_stage2Color, true);
	drawRoom(m_stage3, m_mWorld, m_stage3Color, true);
}

// 2. 他の部屋の半透明壁の描画（半透明パス）
void Ground::DrawUnLit()
{
	// アルファブレンドと ZWriteDisable（深度書き込みオフ）を有効化
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	if (m_frontColor.w < 1.0f && m_front)  KdShaderManager::Instance().m_StandardShader.DrawModel(*m_front, m_mWorld, m_frontColor);
	if (m_stage1Color.w < 1.0f && m_stage1) KdShaderManager::Instance().m_StandardShader.DrawModel(*m_stage1, m_mWorld, m_stage1Color);
	if (m_stage2Color.w < 1.0f && m_stage2) KdShaderManager::Instance().m_StandardShader.DrawModel(*m_stage2, m_mWorld, m_stage2Color);
	if (m_stage3Color.w < 1.0f && m_stage3) KdShaderManager::Instance().m_StandardShader.DrawModel(*m_stage3, m_mWorld, m_stage3Color);

	KdShaderManager::Instance().UndoDepthStencilState();
	KdShaderManager::Instance().UndoBlendState();
}

//void Ground::Update()
//{
//	// アクティブ部屋：完全不透明
//	const Math::Color activeColor = { 1.0f, 1.0f, 1.0f, 1.0f };
//
//	// 非アクティブ部屋：原色のままアルファ値だけ下げる（黒く潰れるのを防止）
//	const Math::Color inactiveColor = { 0.3f, 0.3f, 0.3f, 0.5f };
//
//	auto spPlayer = m_wpPlayer.lock();
//
//	if (!spPlayer)
//	{
//		m_frontColor = activeColor;
//		m_stage1Color = activeColor;
//		m_stage2Color = activeColor;
//		m_stage3Color = activeColor;
//		return;
//	}
//
//	float playerZ = spPlayer->GetPos().z;
//
//	// Z軸判定
//	if (playerZ <= -2.25f)
//	{
//		m_frontColor = activeColor;
//		m_stage1Color = inactiveColor;
//		m_stage2Color = inactiveColor;
//		m_stage3Color = inactiveColor;
//	}
//	else if (playerZ <= 0.2f)
//	{
//		m_frontColor = inactiveColor;
//		m_stage1Color = activeColor;
//		m_stage2Color = inactiveColor;
//		m_stage3Color = inactiveColor;
//	}
//	else if (playerZ <= 2.7f)
//	{
//		m_frontColor = inactiveColor;
//		m_stage1Color = inactiveColor;
//		m_stage2Color = activeColor;
//		m_stage3Color = inactiveColor;
//	}
//	else
//	{
//		m_frontColor = inactiveColor;
//		m_stage1Color = inactiveColor;
//		m_stage2Color = inactiveColor;
//		m_stage3Color = activeColor;
//	}
//}
//
//void Ground::DrawLit()
//{
//	auto drawModel = [this](std::shared_ptr<KdModelData>& model, const Math::Color& color)
//		{
//			if (!model) return;
//
//			if (color.w < 1.0f)
//			{
//				// ★ 半透明オブジェクト用の描画設定
//				// ZWriteDisable を有効にして「奥にいるプレイヤーを消さない」ようにする
//				KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
//				KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);
//
//				KdShaderManager::Instance().m_StandardShader.DrawModel(*model, m_mWorld, color);
//
//				KdShaderManager::Instance().UndoDepthStencilState();
//				KdShaderManager::Instance().UndoBlendState();
//			}
//			else
//			{
//				// 通常の不透明描画
//				KdShaderManager::Instance().m_StandardShader.DrawModel(*model, m_mWorld, color);
//			}
//		};
//
//	// 描画実行
//	drawModel(m_front, m_frontColor);
//	drawModel(m_stage1, m_stage1Color);
//	drawModel(m_stage2, m_stage2Color);
//	drawModel(m_stage3, m_stage3Color);
//}