//=====================================================
//
// 全シェーダ共通
//
//=====================================================

//------------------------------
// 定数バッファ(カメラ)
//------------------------------
cbuffer cbCamera : register(b7)
{
	// カメラ情報
	row_major float4x4 g_mView; // ビュー変換行列
	row_major float4x4 g_mProj; // 射影変換行列
	row_major float4x4 g_mProjInv; // 射影変換行列：逆行列
	float3 g_CamPos; // カメラ座標
};

//------------------------------
// 定数バッファ(フォグ)
//------------------------------
cbuffer cbFog : register(b8)
{
	// 距離フォグ
	int g_DistanceFogEnable; // 距離フォグ有効/無効フラグ
	float3 g_DistanceFogColor; // 距離フォグ色
	float g_DistanceFogDensity; // 距離フォグ減衰率
	
	// 高さフォグ
	float g_HeightFogTopValue; // 高さフォグの開始の高さ
	float g_HeightFogBottomValue; // 高さフォグの終了の高さ
	float g_HeightFogDistance; // 高さフォグの開始の距離
	float3 g_HeightFogColor; // 高さフォグ色
	int g_HeightFogEnable; // 高さフォグ有効/無効フラグ
};

//------------------------------
// 定数バッファ(ライト)
//------------------------------
cbuffer cbLight : register(b9)
{
	// 環境光
	float4 g_AmbientLight;

	// 平行光
	float3 g_DL_Dir; // 光の方向
	float3 g_DL_Color; // 光の色
	row_major float4x4 g_DL_mLightVP; // ビュー行列 x 射影行列

	//--------------
	// 点光
	//--------------
	// 使用数
	int4 g_PointLightNum; // パッキング規則が大変なので、int4のxのみを使用する
	// データ
	struct PointLight
	{
		float3 Color; // 色
		float Radius; // 半径
		float3 Pos; // 座標
		int IsBright; // 明度用ライトかどうか
	} g_PointLights[100];

	// 5. 2.5D コーンライト (16 bytes + 配列)
	// 2.5D コーンライト (単体)
	int g_ConeLightEnable;
	float3 g_ConeLightPos;

	float3 g_ConeLightDir;
	float g_ConeLightAngle;

	float3 g_ConeLightColor;
	float g_ConeLightRange;

	float g_ConeLightIntensity;
	float g_ConeLightEdgeSmoothness;
	int g_ConeLightIsHit;
	float dummy_ConeLight;

	float3 g_ConeLightHitPos;
	float dummy_ConeLight2;

	float3 g_ConeLightHitNormal;
	float dummy_ConeLight3;
};

//------------------------------
// 定数バッファ(カラースフィア/赤色バリアエフェクト)
//------------------------------
struct EffectSphereData
{
	float3 Pos; // カラースフィアの中心座標
	float Radius; // カラースフィアの半径
	float3 Color; // 加算・強調する色
	float Padding; // パッキング用
};

static const int MAX_EFFECT_SPHERES = 10;

cbuffer cbEffect : register(b10)
{
	int g_colorEnable; // 全体有効フラグ
	int g_colorCount; // 有効なスフィアの数 (最大10)
	float2 g_colorPadding;
	EffectSphereData g_colorList[MAX_EFFECT_SPHERES]; // スフィア配列
};
