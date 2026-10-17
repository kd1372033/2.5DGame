#include "Computer.h"
#include "../../Item/Jewely/Jewelry.h"
#include "../../Item/Item/Item.h"
#include "../../Character/Player/Player.h" // Playerヘッダーを追加
#include "../../../../Scene/SceneManager.h"

void Computer::Init()
{
	m_drawType = eDrawTypeLit;

	m_pCollider = std::make_unique<KdCollider>();

	// --- 各モデルのロード ---
	m_pcNormalModel = std::make_shared<KdModelData>();
	m_pcNormalModel->Load("Asset/Models/ComputerDesk/Comdesk.gltf");

	m_pcBrokenModel = std::make_shared<KdModelData>();
	m_pcBrokenModel->Load("Asset/Models/ComputerDesk/Comdeskbreak.gltf");

	// ★ 1. Choice.png を貼り付けた KdSquarePolygon の初期化
	m_spChoicePoly = std::make_shared<KdSquarePolygon>();
	m_spChoicePoly->SetMaterial("Asset/Textures/Choice.png"); // ※実際のファイルパスに合わせて調整してください
	m_spChoicePoly->SetScale({ 0.35f, 0.35f }); // 幅・高さ

	m_isBroken = false;
	m_isNearPlayer = false;
}

void Computer::Update()
{
	Math::Vector3 pos = GetPos();

	Math::Matrix scalemat = Math::Matrix::CreateScale(0.5f);
	Math::Matrix rotatemat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_rotY));
	Math::Matrix transmat = Math::Matrix::CreateTranslation(pos);
	m_mWorld = scalemat * rotatemat * transmat;

	// --------------------------------------------------
	// ★ 2. プレイヤーとの距離判定
	// --------------------------------------------------
	m_isNearPlayer = false;

	if (!m_isBroken)
	{
		if (auto player = m_wpPlayer.lock())
		{
			// ComputerとPlayerの距離を計算
			float distance = Math::Vector3::Distance(pos, player->GetPos());

			// 設定した距離（例: 1.8m）以内にプレイヤーが入ったら表示フラグをオン
			if (distance <= m_interactDistance)
			{
				m_isNearPlayer = true;
			}
		}
	}

	// コライダーの更新
	if (m_pCollider)
	{
		if (!m_isBroken)
		{
			// 1. 通常PC本体の当たり判定
			if (m_pcNormalModel)
			{
				m_pCollider->RegisterCollisionShape("PC_Normal", m_pcNormalModel, KdCollider::TypeBump | KdCollider::TypeGround);
			}

			// 2. モデル無しの「円形（球体）不可侵エリア」をコライダーに登録
			DirectX::BoundingSphere barrierSphere;
			barrierSphere.Center = pos;            // PCの中心
			barrierSphere.Center.y += 0.27f;       // プレイヤーの腰の高さに合わせる
			barrierSphere.Radius = m_barrierRadius; // バリアの円形半径

			// TypeBump（押し戻し壁）として登録
			m_pCollider->RegisterCollisionShape("BarrierSphere", barrierSphere, KdCollider::TypeBump);
		}
		else
		{
			// 壊れた後はバリアが消えて壊れたPC本体のみ障害物となる
			if (m_pcBrokenModel)
			{
				m_pCollider->RegisterCollisionShape("PC_Broken", m_pcBrokenModel, KdCollider::TypeBump | KdCollider::TypeGround);
			}
		}
	}
}

void Computer::PostUpdate()
{
	if (m_isBroken) return;

	// --- 投げられたアイテムとのヒット判定 ---
	KdCollider::SphereInfo pcSphere;
	pcSphere.m_sphere.Center = GetPos();
	pcSphere.m_sphere.Center.y += 0.3f;
	pcSphere.m_sphere.Radius = 0.5f; // PCの当たり判定半径
	pcSphere.m_type = KdCollider::TypeSight;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto item = std::dynamic_pointer_cast<Item>(obj);
		if (!item) continue;

		// 投げられたアイテムのみ判定対象にする
		if (item->IsThrown() || item->HasBeenThrown())
		{
			float dist = (item->GetPos() - pcSphere.m_sphere.Center).Length();

			if (dist <= pcSphere.m_sphere.Radius)
			{
				m_isBroken = true; // PC破壊

				// 関連付けられている宝石を取得可能状態に設定
				auto jewelry = m_wpJewelry.lock();
				if (jewelry)
				{
					jewelry->SetCanGet(true);
				}

				break;
			}
		}
	}
}

void Computer::DrawLit()
{
	// 1. PCモデルの描画（通常 / 破壊後）
	if (!m_isBroken)
	{
		if (m_pcNormalModel)
		{
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_pcNormalModel, m_mWorld);
		}

		// --------------------------------------------------
		// ★ プレイヤー接近時：Choice.png（3Dポリゴン）を描画
		// --------------------------------------------------
		if (m_isNearPlayer && m_spChoicePoly)
		{
			if (auto player = m_wpPlayer.lock())
			{
				// プレイヤーの位置から「右へ +0.4m, 上へ +0.8m, 手前へ +0.1m」
				Math::Vector3 uiPos = player->GetPos() + Math::Vector3(0.2f, 0.6f, 0.1f);

				// ビルボード計算なし：単純な移動行列のみ作成
				Math::Matrix transMat = Math::Matrix::CreateTranslation(uiPos);

				// アルファブレンド有効化
				KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);

				// Player とまったく同じ方法で描画
				KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_spChoicePoly, transMat);

				// ブレンドステート復元
				KdShaderManager::Instance().UndoBlendState();
			}
		}
	}
	else
	{
		if (m_pcBrokenModel)
		{
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_pcBrokenModel, m_mWorld);
		}
	}
}