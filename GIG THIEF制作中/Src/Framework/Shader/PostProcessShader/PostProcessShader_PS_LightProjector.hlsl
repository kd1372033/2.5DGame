// ポスプロ共通インクルード（既存ファイルを使用）
#include "inc_KdPostProcessShader.hlsli"

// C++側の cbLightProjector と対応する定数バッファ
cbuffer cbLightProjector : register(b0)
{
    row_major matrix g_mInvViewProj; // カメラの逆行列
    row_major matrix g_mLightViewProj; // ライト視点の行列
    
    // ★追加: ライトのワールド方向ベクトル（C++側から渡すか、行列から抽出）
    // ※もしC++から渡せない場合は、下記のように行列から計算も可能です
    float3 g_vLightDir;
};

Texture2D g_inputTexture : register(t0); // 元の画面画像
Texture2D g_depthTexture : register(t1); // Depthバッファ（奥行き）
Texture2D g_lightTexture : register(t2); // 投影したい光の画像（スポットライトのテクスチャ）
SamplerState g_samLinear : register(s0);

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float4 main(VS_OUTPUT In) : SV_TARGET
{
    // 1. 元の画面の色を取得
    float4 baseColor = g_inputTexture.Sample(g_samLinear, In.UV);

    // 2. Depthバッファから深度値を取得
    float depth = g_depthTexture.Sample(g_samLinear, In.UV).r;

    // 背景領域（空など深度値が1.0）ならそのまま返す
    if (depth >= 1.0f)
    {
        return baseColor;
    }

    // 3. 画面UVとDepthからNDC座標（-1～1）を作成
    float4 ndcPos;
    ndcPos.x = In.UV.x * 2.0f - 1.0f;
    ndcPos.y = (1.0f - In.UV.y) * 2.0f - 1.0f; // Y軸反転
    ndcPos.z = depth;
    ndcPos.w = 1.0f;

    // 4. カメラの逆行列を使ってワールド座標を復元
    float4 worldPos = mul(ndcPos, g_mInvViewProj);
    worldPos.xyz /= worldPos.w;

    // -------------------------------------------------------------
    // 法線の算出（床・壁の両方を許容する）
    // -------------------------------------------------------------
    float3 dX = ddx(worldPos.xyz);
    float3 dY = ddy(worldPos.xyz);
    float3 worldNormal = normalize(cross(dY, dX));

    // 5. ワールド座標をライト空間へ変換してUV（0～1）を作る
    float4 lightSpacePos = mul(float4(worldPos.xyz, 1.0f), g_mLightViewProj);
    
    // 光の背面（Z <= 0）への貫通描画を防止
    if (lightSpacePos.w <= 0.0f)
    {
        return baseColor;
    }

    float3 lightNdc = lightSpacePos.xyz / lightSpacePos.w;

    float2 lightUV;
    lightUV.x = lightNdc.x * 0.5f + 0.5f;
    lightUV.y = -lightNdc.y * 0.5f + 0.5f;

    // 6. 照射範囲外（アスペクト・奥行き範囲外）ならスキップ
    if (lightUV.x < 0.0f || lightUV.x > 1.0f ||
        lightUV.y < 0.0f || lightUV.y > 1.0f ||
        lightNdc.z < 0.0f || lightNdc.z > 1.0f)
    {
        return baseColor;
    }

    // -------------------------------------------------------------
    // ★角度減衰（N dot L）の計算
    // -------------------------------------------------------------
    // ライトの向き（照射方向）と面の法線との内積をとる
    // 光を受ける面（向かい合っている面）ほど 1.0 に近づき、真横や裏面は 0.0 以下になる
    float NdotL = saturate(dot(worldNormal, -g_vLightDir));

    // 角度が浅すぎる（引き伸ばしが発生する）部分を滑らかにフェードアウト
    float angleFade = smoothstep(0.0f, 0.2f, NdotL);

    // 7. 光のテクスチャをサンプル
    float4 lightColor = g_lightTexture.Sample(g_samLinear, lightUV);

    // 角度減衰（angleFade）を乗算して合成
    // ※テクスチャ自体にAlphaが含まれる場合は lightColor.a も乗算してください
    return baseColor + (lightColor * angleFade);
}