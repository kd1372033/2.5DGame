#include "GameUI.h"

void GameUI::Init()
{
	m_arrow.Load("Asset/Textures/Arrow.png");
	m_space.Load("Asset/Textures/Space.png");
	m_jewelryIcon.Load("Asset/Textures/Jewelry.png");

	// 画面内の配置（例：矢印キーを左側、スペースキーを右側に並べる）
	m_arrowbasepos = { 450.0f, -288.0f };
	m_spacebasepos = { 0.0f, -320.0f }; 
	m_jewelryPos = { -600.0f, -320.0f };

	// 初期アルファ値（最初は半透明）
	m_alphaLeft = 0.3f;
	m_alphaRight = 0.3f;
	m_alphaUp = 0.3f;
	m_alphaDown = 0.3f;
	m_alphaSpace = 0.3f; // ★追加
}

void GameUI::Update()
{
	// =============================================================
	// ★修正：押しているときだけ 1.0f（表示）、離したら 0.0f（非表示）
	// =============================================================
	// ※パチッと切り替えたいので、fadeSpeed による補間は無くしています。

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
}

void GameUI::PostUpdate()
{
	// 各UIの基準位置行列を作成
	arrowbasemat = Math::Matrix::CreateTranslation(m_arrowbasepos.x, m_arrowbasepos.y, 0.0f);
	spacebasemat = Math::Matrix::CreateTranslation(m_spacebasepos.x, m_spacebasepos.y, 0.0f);
	m_jewelryMat = Math::Matrix::CreateTranslation(m_jewelryPos.x, m_jewelryPos.y, 0.0f); // ★追加
}

void GameUI::DrawSprite()
{
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZDisable);

	// 常時薄っすら表示される土台カラー（アルファ値 0.3f 固定）
	Math::Color baseColor(1.0f, 1.0f, 1.0f, 0.3f);



	// =============================================================
	// 矢印キー（凸型）の描画
	// =============================================================
	KdShaderManager::Instance().m_spriteShader.SetMatrix(arrowbasemat);

	// --- 左キー ---
	{
		Math::Rectangle rc = { 0, 64, 64, 64 };
		// ① まず常時表示の土台を描画 (0.3f)
		KdShaderManager::Instance().m_spriteShader.DrawTex(&m_arrow, -64, -32, 64, 64, &rc, &baseColor);

		// ② 押されているとき(1.0f)のみ、その上に重ねて描画（0.0fのときは描画しても透明で映らない）
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

	// =============================================================
	// スペースキーの描画
	// =============================================================
	KdShaderManager::Instance().m_spriteShader.SetMatrix(spacebasemat);
	{
		// ① 常時表示の土台を描画 (0.3f)
		KdShaderManager::Instance().m_spriteShader.DrawTex(&m_space, 0, 0, 256, 64, nullptr, &baseColor);

		// ② 押されているとき(1.0f)のみ重ねて描画
		if (m_alphaSpace > 0.0f) {
			Math::Color keyColor(1.0f, 1.0f, 1.0f, m_alphaSpace);
			KdShaderManager::Instance().m_spriteShader.DrawTex(&m_space, 0, 0, 256, 64, nullptr, &keyColor);
		}
	}

	if (m_isDrawJewelry)
	{
		KdShaderManager::Instance().m_spriteShader.SetMatrix(m_jewelryMat);

		// ※ 64, 64 の部分は実際のアイコン画像のサイズに合わせて調整してください
		KdShaderManager::Instance().m_spriteShader.DrawTex(&m_jewelryIcon, 0, 0, 64, 64);
	}

	// 後始末
	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::Identity);
	KdShaderManager::Instance().UndoDepthStencilState();
	KdShaderManager::Instance().UndoBlendState();
}