#pragma once
//
//class ResultObj : public KdGameObject
//{
//public:
//	ResultObj() {}
//	~ResultObj() {}
//
//	void Init()			override;
//	void Update()		override;
//	void PostUpdate()	override;
//	void DrawSprite()	override;
//
//private:
//
//	
//	
//	
//
//	
//	
//
//	
//
//	
//	
//};

#pragma once

class ResultObj : public KdGameObject
{
public:
	ResultObj() { Init(); }
	~ResultObj() {}

	void Init()			override;
	void Update()		override;
	void PostUpdate()	override;
	void DrawSprite()		override;


private:
	KdTexture m_result;
	KdTexture m_caughtEnter;
	KdTexture m_clearEnter;

	Math::Matrix resultmat;
	Math::Matrix entermat;

	Math::Vector2 m_resultpos;
	Math::Vector2 m_enterpos;

	bool m_clear;
	bool m_isplay = false;

	float m_alpha;
	float m_delta;
};