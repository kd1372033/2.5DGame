#pragma once

#include"../BaseScene/BaseScene.h"

class Player;

class GameScene : public BaseScene
{
public :

	GameScene() { Init(); }
	~GameScene() { /*KdAudioManager::Instance().SoundReset();*/ }

private:

	void Event() override;
	void Init()  override;

	std::shared_ptr<Player> m_player;

	bool m_hasSpawnedEnemies = false;
};
