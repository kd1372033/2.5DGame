#include "ResultScene.h"
#include "../SceneManager.h"

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

}
