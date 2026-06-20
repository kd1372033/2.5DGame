#pragma once

#include"../BaseScene/BaseScene.h"

class Player;
class GameUI;

class GameScene : public BaseScene
{
public :

	GameScene() { Init(); }
	~GameScene() { /*KdAudioManager::Instance().SoundReset();*/ }

private:

	void Event() override;
	void Init()  override;

	std::shared_ptr<Player> m_player;
	std::shared_ptr<GameUI> m_UI;

	bool m_hasSpawnedEnemies = false;
};
