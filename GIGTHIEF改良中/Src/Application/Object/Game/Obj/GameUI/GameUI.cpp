#include "GameUI.h"

void GameUI::Init()
{
	m_arrow.Load("Asset/Textures/Arrow.png");
	m_space.Load("Asset/Textures/Space.png");
	m_enter.Load("Asset/Textures/EnterIcon.png"); // ★追加：Enterキー画像の読み込み
	m_jewelryIcon.Load("Asset/Textures/Jewelry.png");

	// 画面内の配置
	m_arrowbasepos = { 270.0f, -288.0f };
	m_enterbasepos = { 410.0f, -288.0f }; // ★追加：矢印キー（↑）の直上に配置
	m_spacebasepos = { 0.0f, -320.0f };
	m_jewelryPos = { -600.0f, 320.0f };

	// 初期アルファ値（最初は半透明）
	m_alphaLeft = 0.3f;
	m_alphaRight = 0.3f;
	m_alphaUp = 0.3f;
	m_alphaDown = 0.3f;
	m_alphaSpace = 0.3f;
	m_alphaEnter = 0.3f; // ★追加

	m_isVisibleKeyGuide = true;
}

void GameUI::Update()
{
	// --- 左キー ---
	m_alphaLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000) ? 1.0f : 0.0f;

	// --- 右キー ---
	m_alphaRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000) ? 1.0f : 0.0f;

	// --- 上キー ---
	m_alphaUp = (GetAsyncKeyState(VK_UP) & 0x8000) ? 1.0f : 0.0f;

	// --- 下キー ---
	m_alphaDown = (GetAsyncKeyState(VK_DOWN) & 0x8000) ? 1.0f : 0.0f;

	// --- スペースキー ---
	m_alphaSpace = (GetAsyncKeyState(VK_SPACE) & 0x8000) ? 1.0f : 0.0f;

	// --- エンターキー ---
	m_alphaEnter = (GetAsyncKeyState(VK_RETURN) & 0x8000) ? 1.0f : 0.0f; // ★追加
}

void GameUI::PostUpdate()
{
	// 各UIの基準位置行列を作成
	arrowbasemat = Math::Matrix::CreateTranslation(m_arrowbasepos.x, m_arrowbasepos.y, 0.0f);
	enterbasemat = Math::Matrix::CreateTranslation(m_enterbasepos.x, m_enterbasepos.y, 0.0f); // ★追加
	spacebasemat = Math::Matrix::CreateTranslation(m_spacebasepos.x, m_spacebasepos.y, 0.0f);
	m_jewelryMat = Math::Matrix::CreateTranslation(m_jewelryPos.x, m_jewelryPos.y, 0.0f);
}

void GameUI::DrawSprite()
{
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZDisable);

	// 常時薄っすら表示される土台カラー（アルファ値 0.3f 固定）
	Math::Color baseColor(1.0f, 1.0f, 1.0f, 0.3f);

	if (m_isVisibleKeyGuide)
	{
		// -------------------------------------------------------------
		// 1. 矢印キー（凸型）の描画
		// -------------------------------------------------------------
		KdShaderManager::Instance().m_spriteShader.SetMatrix(arrowbasemat);

		// --- 左キー ---
		{
			Math::Rectangle rc = { 0, 64, 64, 64 };
			KdShaderManager::Instance().m_spriteShader.DrawTex(&m_arrow, -64, -32, 64, 64, &rc, &baseColor);
			if (m_alphaLeft > 0.0f) {
				Math::Color keyColor(1.0f, 1.0f, 1.0f, m_alphaLeft);
				KdShaderManager::Instance().m_spriteShader.DrawTex(&m_arrow, -64, -32, 64, 64, &rc, &keyColor);
			}
		}

		// --- 下キー ---
		{
			Math::Rectangle rc = { 64, 64, 64, 64 };
			KdShaderManager::Instance().m_spriteShader.DrawTex(&m_arrow, 0, -32, 64, 64, &rc, &baseColor);
			if (m_alphaDown > 0.0f) {
				Math::Color keyColor(1.0f, 1.0f, 1.0f, m_alphaDown);
				KdShaderManager::Instance().m_spriteShader.DrawTex(&m_arrow, 0, -32, 64, 64, &rc, &keyColor);
			}
		}

		// --- 右キー ---
		{
			Math::Rectangle rc = { 128, 64, 64, 64 };
			KdShaderManager::Instance().m_spriteShader.DrawTex(&m_arrow, 64, -32, 64, 64, &rc, &baseColor);
			if (m_alphaRight > 0.0f) {
				Math::Color keyColor(1.0f, 1.0f, 1.0f, m_alphaRight);
				KdShaderManager::Instance().m_spriteShader.DrawTex(&m_arrow, 64, -32, 64, 64, &rc, &keyColor);
			}
		}

		// --- 上キー ---
		{
			Math::Rectangle rc = { 64, 0, 64, 64 };
			KdShaderManager::Instance().m_spriteShader.DrawTex(&m_arrow, 0, 32, 64, 64, &rc, &baseColor);
			if (m_alphaUp > 0.0f) {
				Math::Color keyColor(1.0f, 1.0f, 1.0f, m_alphaUp);
				KdShaderManager::Instance().m_spriteShader.DrawTex(&m_arrow, 0, 32, 64, 64, &rc, &keyColor);
			}
		}

		// -------------------------------------------------------------
		// 2. エンターキーの描画（★追加）
		// -------------------------------------------------------------
		KdShaderManager::Instance().m_spriteShader.SetMatrix(enterbasemat);
		{
			// 横幅192, 高さ64程度の横長キー画像（または正方形サイズに応じて調整）
			KdShaderManager::Instance().m_spriteShader.DrawTex(&m_enter, 0, 0, 96, 128, nullptr, &baseColor);
			if (m_alphaEnter > 0.0f) {
				Math::Color keyColor(1.0f, 1.0f, 1.0f, m_alphaEnter);
				KdShaderManager::Instance().m_spriteShader.DrawTex(&m_enter, 0, 0, 96, 128, nullptr, &keyColor);
			}
		}

		// -------------------------------------------------------------
		// 3. スペースキーの描画
		// -------------------------------------------------------------
		KdShaderManager::Instance().m_spriteShader.SetMatrix(spacebasemat);
		{
			KdShaderManager::Instance().m_spriteShader.DrawTex(&m_space, 0, 0, 256, 64, nullptr, &baseColor);
			if (m_alphaSpace > 0.0f) {
				Math::Color keyColor(1.0f, 1.0f, 1.0f, m_alphaSpace);
				KdShaderManager::Instance().m_spriteShader.DrawTex(&m_space, 0, 0, 256, 64, nullptr, &keyColor);
			}
		}
	}

	// 宝石アイコン
	if (m_isDrawJewelry)
	{
		KdShaderManager::Instance().m_spriteShader.SetMatrix(m_jewelryMat);
		KdShaderManager::Instance().m_spriteShader.DrawTex(&m_jewelryIcon, 0, 0, 64, 64);
	}

	// 後始末
	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::Identity);
	KdShaderManager::Instance().UndoDepthStencilState();
	KdShaderManager::Instance().UndoBlendState();
}