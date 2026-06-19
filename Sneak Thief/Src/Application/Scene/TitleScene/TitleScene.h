#pragma once

#include"../BaseScene/BaseScene.h"

class TitleScene : public BaseScene
{
public :

	TitleScene() { Init(); }
	~TitleScene() { /*KdAudioManager::Instance().SoundReset();*/ }

private :

	void Event() override;
	void Init()  override;

	std::shared_ptr<KdSoundInstance> m_bgm = nullptr;
};
