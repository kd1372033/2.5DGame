#include "TitleScene.h"
#include "../SceneManager.h"

void TitleScene::Init()
{
	// 1. 前のシーン（ゲームやリザルト）で鳴っていたSEなどの残響をすべて止める
	KdAudioManager::Instance().StopAllSound();

	// 2. タイトルBGMを再生（すでに鳴っていても強制的に頭からリスタートされます）
	KdAudioManager::Instance().PlayBGM("Asset/Sounds/BGM.wav", 0.2f);
}

void TitleScene::Event()
{
	if (GetAsyncKeyState(VK_RETURN) & 0x8000)
	{
		while (GetAsyncKeyState(VK_RETURN) & 0x8000) {} // 押しっぱなし防止

		// 💡 ここでは Stop 処理を何も書かない！
		// マネージャーの「m_currentBGM」が生存を保証してくれるため、
		// シーンがゲームに切り替わってもBGMは途切れずに鳴り続けます。

		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Game);
	}
}