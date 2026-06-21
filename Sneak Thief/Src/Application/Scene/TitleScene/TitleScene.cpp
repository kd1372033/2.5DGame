#include "TitleScene.h"
#include "../SceneManager.h"
#include "../../Object/TitleObj/TitleObj.h"

void TitleScene::Event()
{
	if (GetAsyncKeyState('T') & 0x8000)
	{
		Init();
	}
	if (GetAsyncKeyState('G') & 0x8000)
	{
		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Game
		);
	}
	if (GetAsyncKeyState('R') & 0x8000)
	{
		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Result
		);
	}
}

void TitleScene::Init()
{
	// 1. 前のシーン（ゲームやリザルト）で鳴っていたSEなどの残響をすべて止める
	KdAudioManager::Instance().StopAllSound();

	// 2. タイトルBGMを再生（すでに鳴っていても強制的に頭からリスタートされます）
	KdAudioManager::Instance().PlayBGM("Asset/Sounds/BGM.wav", 0.1f);

	std::shared_ptr<TitleObj> title;
	title = std::make_shared<TitleObj>();
	m_objList.push_back(title);
}