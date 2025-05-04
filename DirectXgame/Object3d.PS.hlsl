#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
};
ConstantBuffer<Material> gmaterial : register(b0);


struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// テクスチャリソース
Texture2D<float32_t4> gTexture : register(t0);

// サンプラー
SamplerState gSampler : register(s0);


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    //output.color = float32_t4(1.0f, 1.0f, 1.0f, 1.0f);
    output.color = gmaterial.color * textureColor;
    return output;
}