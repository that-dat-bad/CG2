struct PixelShaderOutput
{
    float32t_4 color : SV_TARGET;
};

PixelShaderOutput main()
{
    PixelShaderOutput output;
    output.color = float32t_4(1.0, 1.0, 1.0, 1.0);
    return output;
}