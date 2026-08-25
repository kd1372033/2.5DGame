#include "TitleObj.h"
#include "../../Scene/SceneManager.h"

void TitleObj::Init()
{
	m_titleback.Load("Asset/Textures/titleBack.png");
	m_titlelogo.Load("Asset/Textures/Title.png");
	m_enter.Load("Asset/Textures/Enter.png");
	m_player.Load("Asset/Textures/Player.png");
	operate.Load("Asset/Textures/Operate.png");

	// ==========================================
	// 2. 各要素の初期位置（座標）の設定
	// ==========================================
	m_backpos = { 0.0f,    0.0f }; // 背景は画面中央
	m_logopos = { 0.0f,  175.0f }; // ロゴは上部
	m_enterpos = { 0.0f, -275.0f }; // 「Enter」は下部
	m_playerpos = { -610.0f,    -200.0f }; // プレイヤー
	m_operatepos = { 0.0f,    0.0f }; // 操作方法は中央

	m_alpha = 0.0f;
	m_delta = 0.01f;

	// ⭕ 状態管理：-1 は「まだ操作説明を開いていない（タイトル）」状態にします
	m_opeCnt = -1;

	m_anime = 0.0f;
	m_moveDirX = 1.0f;
	m_dirID = 0;
}

void TitleObj::Update()
{
	// ==========================================
	// 1. Enter文字の点滅処理（既存のまま）
	// ==========================================
	m_alpha += m_delta;
	if (m_alpha >= 1.0f) { m_alpha = 1.0f; m_delta = -0.01f; }
	else if (m_alpha <= 0.2f) { m_alpha = 0.2f; m_delta = 0.01f; }

	// ==========================================
	// 2. 自機の自動歩行・往復ロジック（★修正：最初のEnterを押す前だけ動かす）
	// ==========================================
	if (m_opeCnt == -1)
	{
		// 座標を移動方向に進める
		m_playerpos.x += m_moveDirX * m_speed;

		// アニメーションのコマを進める（8コマ分ループ）
		m_anime += 0.1f;
		if (m_anime >= 8.0f) m_anime = 0.0f;

		// 移動方向に応じたアニメーションの向きを設定（0:右歩き、1:左歩き）
		m_dirID = (m_moveDirX > 0.0f) ? 0 : 1;

		// 画面端（右端 600、左端 -600）に達したら反転する
		if (m_playerpos.x >= 610.0f)
		{
			m_playerpos.x = 610.0f;
			m_moveDirX = -1.0f; // 左へ反転
		}
		else if (m_playerpos.x <= -610.0f)
		{
			m_playerpos.x = -610.0f;
			m_moveDirX = 1.0f;  // 右へ反転
		}
	}

	// ==========================================
	// 3. 入力検知によるページ進行（既存のまま）
	// ==========================================
	if (m_opeCnt == -1)
	{
		if (GetAsyncKeyState(VK_RETURN) & 0x0001)
		{
			m_opeCnt = 0;
			auto se = KdAudioManager::Instance().Play("Asset/Sounds/Pera.wav", false);
			if (se) se->SetVolume(0.05f);
		}
	}
	else if (m_opeCnt == 0)
	{
		if (GetAsyncKeyState(VK_RIGHT) & 0x0001)
		{
			m_opeCnt = 1;
			auto se = KdAudioManager::Instance().Play("Asset/Sounds/Pera.wav", false);
			if (se) se->SetVolume(0.05f);
		}
	}
	else if (m_opeCnt == 1)
	{
		if (GetAsyncKeyState(VK_RETURN) & 0x0001)
		{
			auto se = KdAudioManager::Instance().Play("Asset/Sounds/Pera.wav", false);
			if (se) se->SetVolume(0.05f);
			SceneManager::Instance().SetNextScene(SceneManager::SceneType::Game);
		}
	}
}

void TitleObj::PostUpdate()
{
	// マトリクス重複対策としてIdentityにしておく
	backmat = Math::Matrix::Identity;
	logomat = Math::Matrix::Identity;
	entermat = Math::Matrix::Identity;
	playermat = Math::Matrix::Identity;
	operatemat = Math::Matrix::Identity;
}

void TitleObj::DrawSprite()
{
	// 描画ステートの適応
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZDisable);

	// 1. 背景 (1280x720)
	KdShaderManager::Instance().m_spriteShader.SetMatrix(backmat);
	KdShaderManager::Instance().m_spriteShader.DrawTex(&m_titleback, (int)m_backpos.x, (int)m_backpos.y, 1280, 720);

	// ==========================================
	// 2. プレイヤーの歩行アニメーション描画
	// ==========================================
	// スプライトシートから切り出す「1コマの元のサイズ」は 30x48 のまま
	int frameX = ((int)m_anime % 8) * 30;
	int frameY = m_dirID * 48;

	Math::Rectangle playerSrc = { frameX, frameY, 30, 48 };

	KdShaderManager::Instance().m_spriteShader.SetMatrix(playermat);

	// ⭕【修正】第5引数（幅）と第6引数（高さ）を3倍の数値（30*3 = 90, 48*3 = 144）に変更します！
	KdShaderManager::Instance().m_spriteShader.DrawTex(&m_player, (int)m_playerpos.x, (int)m_playerpos.y, 90, 144, &playerSrc);

	// 3. タイトルロゴ (789x134)
	KdShaderManager::Instance().m_spriteShader.SetMatrix(logomat);
	KdShaderManager::Instance().m_spriteShader.DrawTex(&m_titlelogo, (int)m_logopos.x, (int)m_logopos.y, 789, 134);

	// 4. 「Press Enter」文字
	if (m_opeCnt == -1)
	{
		Math::Color color(1.0f, 1.0f, 1.0f, m_alpha);
		KdShaderManager::Instance().m_spriteShader.SetMatrix(entermat);
		KdShaderManager::Instance().m_spriteShader.DrawTex(&m_enter, (int)m_enterpos.x, (int)m_enterpos.y, 247, 89, nullptr, &color);
	}

	// 5. 操作説明の描画
	if (m_opeCnt >= 0)
	{
		Math::Rectangle rc = { 640 * m_opeCnt, 0, 640, 720 };
		KdShaderManager::Instance().m_spriteShader.SetMatrix(operatemat);
		KdShaderManager::Instance().m_spriteShader.DrawTex(&operate, (int)m_operatepos.x, (int)m_operatepos.y, 640, 720, &rc);
	}

	// 描画ステートを戻す
	KdShaderManager::Instance().UndoDepthStencilState();
	KdShaderManager::Instance().UndoBlendState();
}