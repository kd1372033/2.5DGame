#pragma once

class TitleObj : public KdGameObject
{
public:
	TitleObj() { Init(); }
	~TitleObj() {}

	void Init()			override;
	void Update()		override;
	void PostUpdate()	override;
	void DrawSprite()		override;


private:


	KdTexture m_titlelogo;	// タイトルロゴ
	KdTexture m_titleback;	// 背景
	KdTexture m_enter;		// エンター
	KdTexture m_player;		// プレイヤー
	KdTexture operate;		// 操作方法

	Math::Matrix logomat;		// タイトルロゴ
	Math::Matrix backmat;		// 背景
	Math::Matrix entermat;		// エンター
	Math::Matrix playermat;		// プレイヤー
	Math::Matrix operatemat;	// 操作方法

	Math::Vector2 m_logopos;
	Math::Vector2 m_backpos;
	Math::Vector2 m_enterpos;
	Math::Vector2 m_playerpos;
	Math::Vector2 m_operatepos;

	float m_alpha;
	float m_delta;

	bool m_opeFlg;
	int m_opeCnt;

	float m_anime = 0.0f;  // アニメーションのコマ進捗
	float m_moveDirX = 1.0f;  // 移動方向（1.0f:右、-1.0f:左）
	float m_speed = 1.0f;  // 歩くスピード（ピクセル単位）
	int   m_dirID = 0;     // 向きID（0:右歩き、1:左歩き）
};
