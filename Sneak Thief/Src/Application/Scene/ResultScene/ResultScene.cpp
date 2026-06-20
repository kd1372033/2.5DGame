#include "ResultScene.h"
#include "../SceneManager.h"
#include "../../Object/ResultObj/ResultObj.h"

void ResultScene::Event()
{
	if (GetAsyncKeyState('T') & 0x8000)
	{
		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Title
		);
	}
	if (GetAsyncKeyState('G') & 0x8000)
	{
		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Game
		);
	}
}

void ResultScene::Init()
{
	std::shared_ptr<ResultObj> result;
	result = std::make_shared<ResultObj>();
	m_objList.push_back(result);
}
