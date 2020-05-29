#version 430 core
// shader source : https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/metallic-roughness.frag

out vec4 fcolor;

layout(binding = 0) uniform sampler2D base;
layout(binding = 1) uniform sampler2D metallicroughness;
layout(binding = 2) uniform sampler2D normalmap;
uniform vec3 basedcolor;
uniform vec3 campos;

in VERTEX {
	vec3 worldpos;
	vec3 normal;
	vec2 texcoord;
} fragment;

struct MaterialInfo
{
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	vec3 reflectance0;            // full reflectance color (normal incidence angle)

	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	vec3 diffuseColor;            // color contribution from diffuse lighting

	vec3 reflectance90;           // reflectance color at grazing angle
	vec3 specularColor;           // color contribution from specular lighting
};

struct Light
{
	vec3 direction;
	vec3 color;
	float intensity;
};

struct AngularInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector

	float VdotH;                  // cos angle between view direction and half vector

	vec3 padding;
};

const float M_PI = 3.141592653589793;
const float GAMMA = 2.2;
const float INV_GAMMA = 1.0/2.2;

vec4 SRGBtoLINEAR(vec4 srgbIn)
{
	return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w);
}

vec3 diffuse(MaterialInfo materialInfo)
{
	return materialInfo.diffuseColor / M_PI;
}

AngularInfo getAngularInfo(vec3 pointToLight, vec3 normal, vec3 view)
{
	// Standard one-letter names
	vec3 n = normalize(normal);           // Outward direction of surface point
	vec3 v = normalize(view);             // Direction from surface point to view
	vec3 l = normalize(pointToLight);     // Direction from surface point to light
	vec3 h = normalize(l + v);            // Direction of the vector between l and v

	float NdotL = clamp(dot(n, l), 0.0, 1.0);
	float NdotV = clamp(dot(n, v), 0.0, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);
	float VdotH = clamp(dot(v, h), 0.0, 1.0);

	return AngularInfo( NdotL, NdotV, NdotH, LdotH, VdotH, vec3(0, 0, 0));
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(MaterialInfo materialInfo, AngularInfo angularInfo)
{
	return materialInfo.reflectance0 + (materialInfo.reflectance90 - materialInfo.reflectance0) * pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

float visibilityOcclusion(MaterialInfo materialInfo, AngularInfo angularInfo)
{
	float NdotL = angularInfo.NdotL;
	float NdotV = angularInfo.NdotV;
	float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;

	float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
	float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

	float GGX = GGXV + GGXL;
	if (GGX > 0.0) { return 0.5 / GGX; }
	return 0.0;
}

float microfacetDistribution(MaterialInfo materialInfo, AngularInfo angularInfo)
{
	float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
	float f = (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
	return alphaRoughnessSq / (M_PI * f * f);
}

vec3 getPointShade(vec3 pointToLight, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
	AngularInfo angularInfo = getAngularInfo(pointToLight, normal, view);

	//if (angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0) {
		// Calculate the shading terms for the microfacet specular shading model
		vec3 F = specularReflection(materialInfo, angularInfo);
		float Vis = visibilityOcclusion(materialInfo, angularInfo);
		float D = microfacetDistribution(materialInfo, angularInfo);

		// Calculation of analytical lighting contribution
		vec3 diffuseContrib = (1.0 - F) * diffuse(materialInfo);
		vec3 specContrib = F * Vis * D;

		// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
		return angularInfo.NdotL * (specContrib);
		/*
	}

	return vec3(0.0, 0.0, 0.0);
	*/
}

vec3 applyDirectionalLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
	vec3 pointToLight = -light.direction;
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return light.intensity * light.color * shade;
}

vec3 LINEARtoSRGB(vec3 color)
{
	return pow(color, vec3(INV_GAMMA));
}

void main(void)
{
	float perceptualRoughness = 0.0;
	float metallic = 0.0;
	vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);
	vec3 diffuseColor = vec3(0.0);
	vec3 specularColor= vec3(0.0);
	vec3 f0 = vec3(0.04);

	vec4 mrSample = texture2D(metallicroughness, fragment.texcoord);
	mrSample.rgb = pow(mrSample.rgb, vec3(INV_GAMMA));
	perceptualRoughness = mrSample.g * 1.0;
	metallic = mrSample.b * 1.0;

	baseColor = texture2D(base, fragment.texcoord);
	if (baseColor.a < 0.5) { discard; }
	baseColor.rgb += basedcolor;
	baseColor.rgb = pow(baseColor.rgb, vec3(GAMMA));

	diffuseColor = baseColor.rgb * (vec3(1.0) - f0) * (1.0 - metallic);

	specularColor = mix(f0, baseColor.rgb, metallic);

	perceptualRoughness = clamp(perceptualRoughness, 0.0, 1.0);
	metallic = clamp(metallic, 0.0, 1.0);

	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness [2].
	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	vec3 specularEnvironmentR0 = specularColor.rgb;
	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));

	MaterialInfo materialInfo = MaterialInfo(
	perceptualRoughness,
	specularEnvironmentR0,
	alphaRoughness,
	diffuseColor,
	specularEnvironmentR90,
	specularColor
	);

	// LIGHTING
	vec3 color = mix(diffuseColor, vec3(0.0, 0.0, 0.0), 0.5);
	vec3 normal = fragment.normal;
	vec3 view = normalize(campos - fragment.worldpos);

	Light light = Light(
		vec3(-0.7399, -0.6428, -0.1983),
		vec3(1.0, 0.9, 0.8),
		3.4
	);

	color += applyDirectionalLight(light, materialInfo, normal, view);

	fcolor = vec4(LINEARtoSRGB(color), 1.0);
}
