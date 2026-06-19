#include "ResultScene.h"
#include "../SceneManager.h"
#include "../../Object/ResultObj/ResultObj.h"

void ResultScene::Event()
{
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
}

void ResultScene::Init()
{
	std::shared_ptr<ResultObj> result;
	result = std::make_shared<ResultObj>();
	m_objList.push_back(result);
}
