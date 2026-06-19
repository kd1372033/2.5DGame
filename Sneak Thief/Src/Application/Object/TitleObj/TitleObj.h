#pragma once

class TitleObj : public KdGameObject
{
public:
	TitleObj(){}
	~TitleObj(){}

	void Init()			override;
	void Update()		override;
	void PostUpdate()	override;
	void DrawSprite()		override;


private:

};
