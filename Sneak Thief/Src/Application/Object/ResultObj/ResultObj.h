#pragma once

class ResultObj : public KdGameObject
{
public:
	ResultObj() {}
	~ResultObj() {}

	void Init()			override;
	void Update()		override;
	void PostUpdate()	override;
	void DrawSprite()		override;


private:

};
