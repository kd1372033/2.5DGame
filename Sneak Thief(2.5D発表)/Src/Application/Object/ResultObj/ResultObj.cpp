#include "ResultObj.h"
#include "../../Scene/SceneManager.h"

void ResultObj::Init()
{
	m_result.Load("Asset/Textures/Result.png");
	m_clear = SceneManager::Instance().GetClearFlag();

	m_clearEnter.Load("Asset/Textures/ClearEnter.png");
	m_caughtEnter.Load("Asset/Textures/GameoverEnter.png");

	m_resultpos = { 0.0f,0.0f };
	m_enterpos = { 0.0f, -275.0f };

	m_alpha = 0.0f;
	m_delta = 0.01f;

	m_isplay = false;

}

void ResultObj::Update()
{
	m_alpha += m_delta;
	if (m_alpha >= 1.0f) { m_alpha = 1.0f; m_delta = -0.01f; }
	else if (m_alpha <= 0.2f) { m_alpha = 0.2f; m_delta = 0.01f; }

	if (!m_isplay)
	{
		if (m_clear)
		{
			auto se = KdAudioManager::Instance().Play("Asset/Sounds/GameClear.wav", false);
			if (se) se->SetVolume(0.05f);
		}
		else
		{
			auto se = KdAudioManager::Instance().Play("Asset/Sounds/GameOver.wav", false);
			if (se) se->SetVolume(0.05f);
		}
		m_isplay = true;
	}

	if (GetAsyncKeyState(VK_RETURN) & 0x8000)
	{
		while (GetAsyncKeyState(VK_RETURN) & 0x8000) {
			// 何もしない
		}

		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Title
		);
	}

	// クリア画面切り替え
	if (GetAsyncKeyState('Z') & 0x8000)
	{
		if (m_clear == false) // 状態が変わったときだけ
		{
			m_clear = true;
			m_isplay = false; // SEをもう一度鳴らすためにリセット
		}
	}
	if (GetAsyncKeyState('X') & 0x8000)
	{
		if (m_clear == true) // 状態が変わったときだけ
		{
			m_clear = false;
			m_isplay = false; // SEをもう一度鳴らすためにリセット
		}
	}
}

void ResultObj::PostUpdate()
{
	resultmat = Math::Matrix::Identity;
	entermat = Math::Matrix::Identity;
}

void ResultObj::DrawSprite()
{
	Math::Rectangle rc = { 1280 * m_clear,0,1280,720 };
	KdShaderManager::Instance().m_spriteShader.DrawTex(&m_result, (int)m_resultpos.x, (int)m_resultpos.y, 1280, 720, &rc);

	Math::Color color(1.0f, 1.0f, 1.0f, m_alpha);
	if (m_clear)
	{
		KdShaderManager::Instance().m_spriteShader.SetMatrix(entermat);
		KdShaderManager::Instance().m_spriteShader.DrawTex(&m_clearEnter, (int)m_enterpos.x, (int)m_enterpos.y, 247, 89, nullptr, &color);
	}
	else
	{
		KdShaderManager::Instance().m_spriteShader.SetMatrix(entermat);
		KdShaderManager::Instance().m_spriteShader.DrawTex(&m_caughtEnter, (int)m_enterpos.x, (int)m_enterpos.y, 247, 89, nullptr, &color);
	}
	
}
