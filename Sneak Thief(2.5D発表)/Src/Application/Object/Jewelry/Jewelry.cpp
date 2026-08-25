#include "Jewelry.h"

#include "../Player/Player.h"

void Jewelry::Init()
{
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();

	// 実体化
	m_polygon = std::make_shared<KdSquarePolygon>();

	// テクスチャの設定
	m_polygon->SetMaterial("Asset/Textures/Jewelry.png");

	m_pos = { 9.5,-1,-1.75 };
	m_alpha = 1.0f;

}

void Jewelry::Update()
{
	static float m_rotationY = 0;
	m_rotationY += 0.02f;

	Math::Matrix scalemat = Math::Matrix::CreateScale(0.75f);
	Math::Matrix rotatemat = Math::Matrix::CreateRotationY(m_rotationY);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * rotatemat * transmat;
}

void Jewelry::PostUpdate()
{

	// 1. ウィークポインタが有効か、そしてshared_ptrに変換できるか（lock）をチェック
	if (auto player = m_wpplayer.lock())
	{
		// ★このif文の中だけ、'player' は安全な shared_ptr として使えます！

		// 自分（Jewelry）の中心座標
		Math::Vector3 myCenter = m_pos;
		myCenter.y -= 0.27f;

		// 相手（Player）の中心座標
		Math::Vector3 pCenter = player->GetPos();
		pCenter.y += 0.27f;

		// 先ほどの綺麗な距離判定ロジック
		Math::Vector3 v = pCenter - myCenter;
		float distance = v.Length();
		float hitDistance = 0.4f; // お互いの半径

		if (distance < hitDistance)
		{
			// ★修正：触れた瞬間（フェードアウトが始まる最初）に、プレイヤーに宝石を渡して音を鳴らす！
			player->SetHasJewelry(true);

			m_alpha -= 0.03f;
			m_pos.y += 0.01f;

			if (m_alpha <= 0.0f)
			{
				m_alpha = 0.0f;
				// (player->SetHasJewelry(true); はここにありましたが、上に引っ越しました)
				OnHit(); // 完全に透明になったので、ここでステージから削除！
			}
		}
	}
	// もしPlayerがすでにステージから消滅していた場合、lock()は不成立(null)になるので
	// 自動的にスルーされ、エラー（不正アクセス）で落ちるのを防げます。
}

void Jewelry::DrawLit()
{
	Math::Color color = { 1.0f,1.0f,1.0f,m_alpha };
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld, color);
}

void Jewelry::GenerateDepthMapFromLight()
{
	// アルファ値が 0 になったら影の描画自体を完全にスキップする
	if (m_alpha <= 0.0f) return;

	Math::Color color = { 1.0f,1.0f,1.0f,m_alpha };

	// ★重要：影用シェーダーのブレンドモードをAlpha（半透明）に対応させる
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);

	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld, color);

	// 後始末（通常の状態に戻す）
	KdShaderManager::Instance().UndoBlendState();
}

void Jewelry::DrawBright()
{
	Math::Color color = { 1.0f,1.0f,1.0f,m_alpha };
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld, color);
}

void Jewelry::OnHit()
{
	if (m_isFadeOut) return;
	m_isExpired = true;
}
