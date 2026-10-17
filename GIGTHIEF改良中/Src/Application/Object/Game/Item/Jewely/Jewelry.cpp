#include "Jewelry.h"
#include "../../Character/Player/Player.h"
#include "../../Character/Enemy/Enemy.h"
#include "../../../../Scene/SceneManager.h"

void Jewelry::Init()
{
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();

	m_model = std::make_shared<KdModelData>();
	m_model->Load("Asset/Models/Jewelry/Jewelry.gltf");

	// ★ 奥の部屋のど真ん中（高さのみ床/台座から少し浮かせる 0.3f〜0.5f）
	m_pos = { 0.0f, 0.4f, 4.0f };

	m_alpha = 1.0f;
	m_canGet = false;
	m_isAlertTriggered = false;
	m_isFadeOut = false;
}

void Jewelry::Update()
{
	static float m_rotationY = 0;
	m_rotationY += 0.02f;

	Math::Matrix scalemat = Math::Matrix::CreateScale(0.3f);
	Math::Matrix rotatemat = Math::Matrix::CreateRotationY(m_rotationY);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * rotatemat * transmat;

	// --------------------------------------------------
	// ★ カラースフィア（赤色バリア）制御
	// --------------------------------------------------
	// m_canGet が true（Computerが壊れて開放状態）の場合はスフィアを消去する
	if (!m_canGet && m_alpha > 0.0f)
	{
		KdShaderManager::EffectSphere sphere;
		sphere.Pos = m_pos;
		sphere.Radius = m_barrierRadius;
		sphere.Color = { 1.0f, 0.0f, 0.0f };

		std::vector<KdShaderManager::EffectSphere> sphereList = { sphere };
		KdShaderManager::Instance().WriteCBColor(sphereList);
	}
	else
	{
		// バリア解除時は空のリストを渡してシェーダー側のスフィア描画を完全に消す
		std::vector<KdShaderManager::EffectSphere> sphereList = {};
		KdShaderManager::Instance().WriteCBColor(sphereList);
	}

	// --------------------------------------------------
	// ★ プレイヤー侵入検知 ＆ 敵生成・捕獲トリガー
	// --------------------------------------------------
	// ※ m_canGet が false（バリア展開中）の時のみ侵入検知を行う
	if (!m_canGet && !m_isAlertTriggered)
	{
		if (auto player = m_wpplayer.lock())
		{
			Math::Vector3 pPos = player->GetPos();

			// プレイヤーとスフィア中心（宝石）との距離測定
			float distXZ = Math::Vector3::Distance(
				Math::Vector3(m_pos.x, 0.0f, m_pos.z),
				Math::Vector3(pPos.x, 0.0f, pPos.z)
			);

			// 赤色スフィアの範囲に入った場合
			if (distXZ <= m_barrierRadius)
			{
				m_isAlertTriggered = true; // 重複警戒防止

				// プレイヤーの少し左側に敵を生成
				Math::Vector3 enemySpawnPos = pPos;
				enemySpawnPos.x -= 1.3f;

				auto newEnemy = std::make_shared<Enemy>();
				newEnemy->Init();
				newEnemy->SetPos(enemySpawnPos);
				newEnemy->SetPlayer(player);

				SceneManager::Instance().AddObject(newEnemy);
			}
		}
	}
}

void Jewelry::PostUpdate()
{
	if (!m_canGet) return;

	if (auto player = m_wpplayer.lock())
	{
		Math::Vector3 myCenter = m_pos;
		myCenter.y -= 0.27f;

		Math::Vector3 pCenter = player->GetPos();
		pCenter.y += 0.27f;

		Math::Vector3 v = pCenter - myCenter;
		float distance = v.Length();
		float hitDistance = 0.4f;

		if (distance < hitDistance)
		{
			player->SetHasJewelry(true);

			m_alpha -= 0.03f;
			m_pos.y += 0.01f;

			if (m_alpha <= 0.0f)
			{
				m_alpha = 0.0f;
				OnHit();
			}
		}
	}
}

//void Jewelry::DrawLit()
//{
//	// ガード処理
//	if (!m_model || m_alpha <= 0.0f) return;
//
//	// ★ 半透明処理が正しく行われるようにブレンドステートを Alpha に変更
//	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
//
//	Math::Color color = { 1.0f, 1.0f, 1.0f, m_alpha };
//	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld, color);
//
//	KdShaderManager::Instance().UndoBlendState();
//}

void Jewelry::DrawLit()
{
	// ★ 1. モデルのヌルチェック
	if (!m_model) return;

	

	// アルファブレンドでモデル描画
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	Math::Color color = { 1.0f, 1.0f, 1.0f, m_alpha };
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld, color);
	KdShaderManager::Instance().UndoBlendState();
}

void Jewelry::DrawBright()
{
	// ★ 宝石自体を強く発光させたい場合のみ描画（発光が不要な場合は return してもOKです）
	if (!m_model || m_alpha <= 0.0f) return;

	// 発光パスでのモデル描画
	Math::Color color = { 1.0f, 1.0f, 1.0f, m_alpha };
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld, color);
}

void Jewelry::GenerateDepthMapFromLight()
{
	if (!m_model || m_alpha <= 0.0f) return;

	Math::Color color = { 1.0f, 1.0f, 1.0f, m_alpha };
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld, color);
	KdShaderManager::Instance().UndoBlendState();
}

void Jewelry::OnHit()
{
	if (m_isFadeOut) return;
	m_isExpired = true;
}