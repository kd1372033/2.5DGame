#pragma once

class GameUI : public KdGameObject
{
public:
	GameUI() {}
	~GameUI() {}

	void Init()			override;
	void Update()		override;
	void PostUpdate()	override;
	void DrawSprite()		override;


private:

};
