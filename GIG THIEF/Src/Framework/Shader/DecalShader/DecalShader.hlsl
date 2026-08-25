// =============================================================
// DecalShader.hlsl : プロジェクター型デカールシェーダー
// =============================================================

// カメラ定数バッファ (b7)
cbuffer cbCamera : register(b7)
{
	row_major matrix g_mView; // ビュー行列
	row_major matrix g_mProj; // 射影行列
	row_major matrix g_mProjInv; // 射影行列の逆行列
	float3 g_CamPos; // カメラ座標
	int g_blank;
};

// デカール用定数バッファ (b1)
cbuffer cbDecal : register(b1)
{
	row_major matrix g_mDecalWorldInv; // 投影箱のワールド逆行列
	float4 g_DecalColor; // 色・透過度
};

// テクスチャとサンプラー
Texture2D g_DepthTexture : register(t0); // 画面の深度バッファ
Texture2D g_DecalTexture : register(t1); // 照射するデカール画像
SamplerState g_Sampler : register(s0);

struct VS_INPUT
{
	float3 Pos : POSITION;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 ScreenPos : TEXCOORD0;
};

// -------------------------------------------------------------
// 頂点シェーダー
// -------------------------------------------------------------
VS_OUTPUT VS(VS_INPUT In)
{
	VS_OUTPUT Out;
    
    // 1. ローカル座標 -> ビュー座標 -> 画面座標
	float4 vPos = float4(In.Pos, 1.0f);
	float4 wPos = mul(vPos, g_mDecalWorldInv); // 必要に応じて調整
	float4 vViewPos = mul(vPos, g_mView);
	Out.Pos = mul(vViewPos, g_mProj);
    
	Out.ScreenPos = Out.Pos;
	return Out;
}

// -------------------------------------------------------------
// ピクセルシェーダー
// -------------------------------------------------------------
float4 PS(VS_OUTPUT In) : SV_TARGET
{
    // 1. スクリーンUV座標の算出 ( [-1, 1] -> [0, 1] )
	float2 screenUV = In.ScreenPos.xy / In.ScreenPos.w;
	screenUV.x = screenUV.x * 0.5f + 0.5f;
	screenUV.y = -screenUV.y * 0.5f + 0.5f;

    // 2. 深度バッファからZ値を読み取る
	float depth = g_DepthTexture.Sample(g_Sampler, screenUV).r;

    // 3. 画面座標 + 深度 から ビュー空間の3D座標を復元
	float4 clipPos = float4((screenUV.x * 2.0f - 1.0f), (1.0f - screenUV.y * 2.0f), depth, 1.0f);
	float4 viewPos = mul(clipPos, g_mProjInv);
	viewPos /= viewPos.w;

    // 4. ビュー座標からデカールのローカル空間（-0.5 ～ 0.5の立方体）へ変換
	float4 decalLocalPos = mul(viewPos, g_mDecalWorldInv);

    // 5. デカール箱の外側にあるピクセルを破棄
	float3 boxDiff = 0.5f - abs(decalLocalPos.xyz);
	clip(boxDiff.x);
	clip(boxDiff.y);
	clip(boxDiff.z);

    // 6. ローカル座標 [-0.5, 0.5] を テクスチャUV [0.0, 1.0] に変換 (XZ面)
	float2 decalUV;
	decalUV.x = decalLocalPos.x + 0.5f;
	decalUV.y = -decalLocalPos.z + 0.5f;

    // 7. テクスチャをサンプリング
	float4 texColor = g_DecalTexture.Sample(g_Sampler, decalUV);
	return texColor * g_DecalColor;
}
