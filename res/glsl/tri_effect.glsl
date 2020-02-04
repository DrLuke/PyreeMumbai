#version 450 core
layout (location = 0) in vec3 posIn;
layout (location = 1) in vec2 uvIn;
layout (location = 2) in vec3 normIn;

layout(binding=0) uniform sampler2D prev;
layout(binding=1) uniform sampler2D img;

layout (location = 0) out vec4 colorOut;

uniform float time;
uniform vec2 res;

const vec2 uv_tri_center = (vec2(0, 0) + vec2(1, 0) + vec2(0.5, 1)) / 3.;

// UTILITIES
vec2 cexp(vec2 z)
{
    return exp(z.x) * vec2(cos(z.y), sin(z.y));
}

vec2 clog(vec2 z)
{
    return vec2(log(length(z)), atan(z.y, z.x));
}

vec2 cpow(vec2 z, float p)
{
    return cexp(clog(z) * p);
}

mat3 rot3(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c          );
}

float discretize(float a, float s)
{
    return round(a*s)/s;
}
vec2 discretize(vec2 a, float s)
{
    return round(a*s)/s;
}
vec3 discretize(vec3 a, float s)
{
    return round(a*s)/s;
}
vec4 discretize(vec4 a, float s)
{
    return round(a*s)/s;
}

mat2 rot2(float a) {
	return mat2(cos(a), -sin(a), sin(a), cos(a));
}

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 hsvrot(vec3 rgb, float amount)
{
    vec3 hsv = rgb2hsv(rgb);
    hsv.r += amount;
    return hsv2rgb(hsv);
}


vec2 center_rotate(vec2 uv, float angle)
{
    return (uv-uv_tri_center)*rot2(angle) + uv_tri_center;
}

vec2 center_scale(vec2 uv, float scale)
{
    return (uv-uv_tri_center)/scale + uv_tri_center;
}

vec4 debug()
{
    float tridist = max(max(normIn.x, normIn.y), normIn.z);

    vec3 b = cos(normIn*50);
    float lines1 = clamp(smoothstep(0.98, 1, max(max(b.x, b.y), b.z)), 0, 1);
    vec3 a = cos(normIn*200);
    float lines2 = clamp(smoothstep(0.95, 1, max(max(a.x, a.y), a.z)), 0, 1);

    vec3 l1c = vec3(0.3) * lines1;
    vec3 l2c = vec3(0.15) * lines2;
    vec3 l3c = vec3(0, 1, 1) * (1.-lines1) * (1.-lines2) * (step(0.98, tridist));

    return vec4(- l1c - l2c + vec3(0.8, 0.8, 0.8) - l3c, 1);
}

void main() {
    vec2 uv = uvIn;
    vec2 uv11 = uvIn * 2 - 1;
    vec2 uvc = uvIn - 0.5;

    float cornerdist = min(min(normIn.x, normIn.y), normIn.z) * 3./2.;
    float edgedist = 1.-(max(max(normIn.x, normIn.y), normIn.z) - 2./3.) * 3;

    vec4 prev_samp = texture(prev, uv);

    // ------------------------------------

    vec4 samp = texture(prev, center_rotate(center_scale(uv, 0.99), 0.01));
    samp.rgb = hsvrot(samp.rgb, prev_samp.g*0.1 + sin(time)*cornerdist*0.1);
    colorOut = 0.99* samp +
    vec4(1-step(0.05, edgedist), 0, 0, 0)*0.95;

    colorOut = clamp(colorOut, 0, 1);

    colorOut = debug();
    //colorOut = texture(img, uv);
    //colorOut = vec4(0);
}
