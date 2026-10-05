#include "inc_KdStandardShader.hlsli"
#include "../inc_KdCommon.hlsli"

// モデル描画用テクスチャ
Texture2D g_baseTex : register(t0); // ベースカラーテクスチャ
Texture2D g_metalRoughTex : register(t1); // メタリック/ラフネステクスチャ
Texture2D g_emissiveTex : register(t2); // 発光テクスチャ
Texture2D g_normalTex : register(t3); // 法線マップ

// 特殊処理用テクスチャ
Texture2D g_dirShadowMap : register(t10); // 平行光シャドウマップ
Texture2D g_dissolveTex : register(t11); // ディゾルブマップ
Texture2D g_environmentTex : register(t12); // 反射景マップ

// サンプラ
SamplerState g_ss : register(s0); // 通常のテクスチャ描画用
SamplerComparisonState g_ssCmp : register(s1); // 補間用比較機能付き

float BlinnPhong(float3 lightDir, float3 vCam, float3 normal, float specPower)
{
	float3 H = normalize(-lightDir + vCam);
	float NdotH = saturate(dot(normal, H)); // カメラの角度差(0～1)
	float spec = pow(NdotH, specPower);

	// 正規化Blinn-Phong
	return spec * ((specPower + 2) / (2 * 3.1415926535));
}

//================================
// ピクセルシェーダ
//================================
float4 main(VSOutput In) : SV_Target0
{
	// ディゾルブによる描画スキップ
	float discardValue = g_dissolveTex.Sample(g_ss, In.UV).r;
	if (discardValue < g_dissolveValue)
	{
		discard;
	}
	
	//------------------------------------------
	// 材質色
	//------------------------------------------
	float4 baseColor = g_baseTex.Sample(g_ss, In.UV) * g_BaseColor * In.Color;
	
	// Alphaテスト
	if (baseColor.a < 0.05f)
	{
		discard;
	}
	
	// カメラへの方向
	float3 vCam = g_CamPos - In.wPos;
	float camDist = length(vCam); // カメラ - ピクセル距離
	vCam = normalize(vCam);

	// 法線マップから法線ベクトル取得
	float3 wN = g_normalTex.Sample(g_ss, In.UV).rgb;

	// UV座標（0～1）から 射影座標（-1～1）へ変換
	wN = wN * 2.0 - 1.0;
	
	{
		// 3種の法線から法線行列を作成
		row_major float3x3 mTBN =
		{
			normalize(In.wT),
			normalize(In.wB),
			normalize(In.wN),
		};
	
		// 法線ベクトルをこのピクセル空間へ変換
		wN = mul(wN, mTBN);
	}

	// 法線正規化
	wN = normalize(wN);

	float4 mr = g_metalRoughTex.Sample(g_ss, In.UV);
	// 金属性
	float metallic = mr.b * g_Metallic;
	// 粗さ
	float roughness = mr.g * g_Roughness;
	// ラフネスを逆転させ「滑らか」さにする
	float smoothness = 1.0 - roughness;
	float specPower = pow(2, 11 * smoothness); // 1～2048
	
	//------------------------------------------
	// ライティング
	//------------------------------------------
	// 最終的な色
	float3 outColor = 0;
	
		// 材質の拡散色 非金属ほど材質の色になり、金属ほど拡散色は無くなる
	const float3 baseDiffuse = lerp(baseColor.rgb, float3(0, 0, 0), metallic);
		// 材質の反射色 非金属ほど光の色をそのまま反射し、金属ほど材質の色が乗る
	const float3 baseSpecular = lerp(0.04, baseColor.rgb, metallic);

	//-------------------------------
	// シャドウマッピング(影判定)
	//-------------------------------
	float shadow = 1;

	// ピクセルの3D座標から、DepthMapFromLight空間へ変換
	float4 liPos = mul(float4(In.wPos, 1), g_DL_mLightVP);
	liPos.xyz /= liPos.w;

	// 深度マップの範囲内？
	if (abs(liPos.x) <= 1 && abs(liPos.y) <= 1 && liPos.z <= 1)
	{
		// 射影座標 -> UV座標へ変換
		float2 uv = liPos.xy * float2(1, -1) * 0.5 + 0.5;
		// ライトカメラからの距離
		float z = liPos.z - 0.004; // シャドウアクネ対策
		
		// 画像のサイズからテクセルサイズを求める
		float w, h;
		g_dirShadowMap.GetDimensions(w, h);
		float tw = 1.0 / w;
		float th = 1.0 / h;
	
		// uvの周辺3x3も判定し、平均値を求める
		shadow = 0;
		for (int y = -1; y <= 1; y++)
		{
			for (int x = -1; x <= 1; x++)
			{
				shadow += g_dirShadowMap.SampleCmpLevelZero(g_ssCmp, uv + float2(x * tw, y * th), z);
			}
		}
		shadow *= 0.11;
	}
		
	//-------------------------
	// 平行光
	//-------------------------
	// Diffuse(拡散光)
	{
		// 光の方向と法線の方向との角度さが光の強さになる
		float lightDiffuse = dot(-g_DL_Dir, wN);
		lightDiffuse = saturate(lightDiffuse); // マイナス値は0にする 0(暗)～1(明)になる

		// 正規化Lambert
		lightDiffuse /= 3.1415926535;

		// 光の色 * 材質の拡散色 * 透明率
		outColor += (g_DL_Color * lightDiffuse) * baseDiffuse * baseColor.a * shadow;
	}

	// Specular(反射色)
	{
		// 反射した光の強さを求める
		// Blinn-Phong NDF
		float spec = BlinnPhong(g_DL_Dir, vCam, wN, specPower);

		// 光の色 * 反射光の強さ * 材質の反射色 * 透明率 * 適当な調整値
		outColor += (g_DL_Color * spec) * baseSpecular * baseColor.a * 0.5 * shadow;
	}

	// 全体の明度：環境光に1が設定されている場合は影響なし
	// 環境光の不透明度を下げる事により、明度ライトの周り以外は描画されなくなる
	float totalBrightness = g_AmbientLight.a;

	//-------------------------
	// 点光
	//-------------------------
	for (int i = 0; i < g_PointLightNum.x; i++)
	{
		// ピクセルから点光への方向
		float3 dir = g_PointLights[i].Pos - In.wPos;
		
		// 距離を算出
		float dist = length(dir);
		
		// 正規化
		dir /= dist;
		
		// 点光の判定以内
		if (dist < g_PointLights[i].Radius)
		{
			// 半径をもとに、距離の比率を求める
			float atte = 1.0 - saturate(dist / g_PointLights[i].Radius);
			
			// 明度の追加
			totalBrightness += (1 - pow(1 - atte, 2)) * g_PointLights[i].IsBright;
			
			// 逆２乗の法則
			atte *= atte;
			
			// Diffuse(拡散光)
			{
				// 光の方向と法線の方向との角度さが光の強さになる
				float lightDiffuse = dot(dir, wN);
				lightDiffuse = saturate(lightDiffuse); // マイナス値は0にする 0(暗)～1(明)になる

				lightDiffuse *= atte; // 減衰

				// 正規化Lambert
				lightDiffuse /= 3.1415926535;

				// 光の色 * 材質の拡散色 * 透明率
				outColor += (g_PointLights[i].Color * lightDiffuse) * baseDiffuse * baseColor.a;
			}

			// Specular(反射色)
			{
				// 反射した光の強さを求める
				// Blinn-Phong NDF
				float spec = BlinnPhong(-dir, vCam, wN, specPower);

				spec *= atte; // 減衰
				
				// 光の色 * 反射光の強さ * 材質の反射色 * 透明率 * 適当な調整値
				outColor += (g_PointLights[i].Color * spec) * baseSpecular * baseColor.a * 0.5;
			}
		}
	}

	//-------------------------
	// 2.5D コーンライト (スポットライト)
	//-------------------------
	if (g_ConeLightEnable)
	{
		// --------------------------------------------------
		// 1. 床面への照射（扇形・三角形）
		// --------------------------------------------------
		float3 lightVecXZ = In.wPos - g_ConeLightPos;
		lightVecXZ.y = 0.06f; // キャラ被り防止の設定を保持
		float distXZ = length(lightVecXZ);

		if (distXZ < g_ConeLightRange && distXZ > 0.8f)
		{
			float3 lightDirXZ = lightVecXZ / distXZ;

			float3 coneDirXZ = g_ConeLightDir;
			coneDirXZ.y = 0.0f;
			coneDirXZ = normalize(coneDirXZ);

			float cosAngle = dot(lightDirXZ, coneDirXZ);
			float halfAngle = g_ConeLightAngle * 0.5f;
			float coneCos = cos(radians(halfAngle));

			if (cosAngle > coneCos)
			{
				// 床面（wN.y >= 0.7f）のみ描画
				if (wN.y >= 0.7f)
				{
					float currentAngle = acos(saturate(cosAngle));
					float edgeFactor = 1.0f - smoothstep(radians(halfAngle) * 0.90f, radians(halfAngle), currentAngle);

					float distFade = saturate(1.0f - (distXZ / g_ConeLightRange) * 0.1f);
					float atte = distFade * edgeFactor;

					float maxIntensity = max(g_ConeLightColor.r, max(g_ConeLightColor.g, g_ConeLightColor.b));
					float3 normColor = (maxIntensity > 0.0f) ? (g_ConeLightColor / maxIntensity) : 0;
					float3 searchLightColor = normColor * 3.0f;

					outColor += searchLightColor * atte * baseDiffuse * baseColor.a;
				}
			}
		}

		// --------------------------------------------------
		// 2. 壁面へのヒット演出（3D空間ベースの半円切り出し）
		// ★ g_ConeLightIsHit == true（壁命中時）のみ描画
		// --------------------------------------------------
		if (g_ConeLightIsHit && wN.y < 0.7f)
		{
			// 3D空間上でのライト位置からの方向と距離
			float3 lightToPos3D = In.wPos - g_ConeLightPos;
			float dist3D = length(lightToPos3D);

			if (dist3D < g_ConeLightRange)
			{
				float3 lightDir3D = lightToPos3D / dist3D;

				// 3Dコーン照射角度内の判定
				float cosAngle3D = dot(lightDir3D, g_ConeLightDir);
				float halfAngle = g_ConeLightAngle * 0.5f;
				float coneCos = cos(radians(halfAngle));

				if (cosAngle3D > coneCos)
				{
					// 着弾点（g_ConeLightHitPos）からの3D距離を計算
					float3 hitToPos = In.wPos - g_ConeLightHitPos;
					float hitDist = length(hitToPos);

					// ライトに向いている壁面（表側）かどうかのチェック
					float wallNdotL = dot(wN, -lightDir3D);

					// ★ 着弾点からの半径 0.8f（ドーム状＝壁でカットされて綺麗な半円になる）かつ表側の壁のみ描画
					if (hitDist <= 0.8f && wallNdotL > 0.0f)
					{
						float currentAngle = acos(saturate(cosAngle3D));
						float edgeFactor = 1.0f - smoothstep(radians(halfAngle) * 0.90f, radians(halfAngle), currentAngle);
						float distFade = saturate(1.0f - (dist3D / g_ConeLightRange) * 0.1f);
						float atte = distFade * edgeFactor;

						float maxIntensity = max(g_ConeLightColor.r, max(g_ConeLightColor.g, g_ConeLightColor.b));
						float3 normColor = (maxIntensity > 0.0f) ? (g_ConeLightColor / maxIntensity) : 0;
						float3 searchLightColor = normColor * 3.0f;

						outColor += searchLightColor * atte * baseDiffuse * baseColor.a;
					}
				}
			}
		}
	}
	
	
	outColor += g_AmbientLight.rgb * baseColor.rgb * baseColor.a;
	
	// 自己発光色の適応
	if (g_OnlyEmissie)
	{
		outColor = g_emissiveTex.Sample(g_ss, In.UV).rgb * g_Emissive * In.Color.rgb;
	}
	else
	{
		outColor += g_emissiveTex.Sample(g_ss, In.UV).rgb * g_Emissive * In.Color.rgb;
	}
	
	//------------------------------------------
	// 高さフォグ
	//------------------------------------------
	if (g_HeightFogEnable && g_FogEnable)
	{
		if (In.wPos.y < g_HeightFogTopValue)
		{
			float distRate = length(In.wPos - g_CamPos);
			distRate = saturate(distRate / g_HeightFogDistance);
			distRate = pow(distRate, 2.0);
			
			float heightRange = g_HeightFogTopValue - g_HeightFogBottomValue;
			float heightRate = 1 - saturate((In.wPos.y - g_HeightFogBottomValue) / heightRange);
			
			float fogRate = heightRate * distRate;
			outColor = lerp(outColor, g_HeightFogColor, fogRate);
		}
	}
	
	//------------------------------------------
	// 距離フォグ
	//------------------------------------------
	if (g_DistanceFogEnable && g_FogEnable)
	{
		// フォグ 1(近い)～0(遠い)
		float f = saturate(1.0 / exp(camDist * g_DistanceFogDensity));
		
		// 適用
		outColor = lerp(g_DistanceFogColor, outColor, f);
	}
	
	// ディゾルブ輪郭発光
	if (g_dissolveValue > 0)
	{
		// 閾値とマスク値の差分で、縁を検出
		if (abs(discardValue - g_dissolveValue) < g_dissolveEdgeRange)
		{
			// 輪郭に発光色追加
			outColor += g_dissolveEmissive;
		}
	}
	
	totalBrightness = saturate(totalBrightness);
	outColor *= totalBrightness;
	
	//------------------------------------------
	// 出力
	//------------------------------------------
	// 10/5 追加
	if (ColorEnable)
	{
		// 色
		// float4 (x y z w)
		// float4 (float3, float);	←これ採用
		// float4 (float2, float2);
		// outColor：最終的に塗られる色
		return float4(outColor + float3(0, 0, 0), baseColor.a - 0.5f);
	}
	else
	{
		return float4(outColor, baseColor.a);
	}
	//	return float4(outColor, baseColor.a);
}
