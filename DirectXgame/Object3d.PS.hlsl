#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enalbleLighting;
};
ConstantBuffer<Material> gmaterial : register(b0);


struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

// テクスチャリソース
Texture2D<float32_t4> gTexture : register(t0);

// サンプラー
SamplerState gSampler : register(s0);


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    if (gmaterial.enalbleLighting != 0)
    {
        float NdotL = dot(input.normal, -gDirectionalLight.direction);

        float halfLambertCos = NdotL * 0.5f + 0.5f; // Remap NdotL from [-1, 1] to [0, 1]
        halfLambertCos *= halfLambertCos; // Square the result for a softer falloff

        // Combine material color, texture color, light color, and the half-Lambert term
        output.color = gmaterial.color * textureColor * gDirectionalLight.color * halfLambertCos * gDirectionalLight.intensity;
    }
    else
    {
        output.color = gmaterial.color * textureColor;
    }

    return output;
}