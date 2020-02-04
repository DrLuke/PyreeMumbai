#version 450 core
layout (location = 0) in vec3 posIn;
layout (location = 1) in vec2 uvIn;
layout (location = 2) in vec3 normIn;

layout (location = 0) out vec3 posOut;
layout (location = 1) out vec2 uvOut;
layout (location = 2) out vec3 normOut;

uniform mat4 MVP;

uniform float time;

uniform vec2 c0pos;
uniform vec2 c1pos;
uniform vec2 c2pos;
uniform vec3 dist_amount;
uniform vec3 dist_amount_weight;

vec2 distort(vec2 pos)
{
    vec3 w = dist_amount_weight;

    vec2 sdir0 = c2pos - c1pos;
    vec2 sdir1 = c0pos - c2pos;
    vec2 sdir2 = c1pos - c0pos;

    //vec2 dist_center0 = c1pos + sdir0 * w.x;
    //vec2 dist_center1 = c2pos + sdir1 * w.y;
    //vec2 dist_center2 = c0pos + sdir2 * w.z;

    vec2 dist_center0 = 0.5*(c1pos + c2pos);
    vec2 dist_center1 = 0.5*(c2pos + c0pos);
    vec2 dist_center2 = 0.5*(c0pos + c1pos);
    vec2 dir0 = c0pos - dist_center0;
    vec2 dir1 = c1pos - dist_center1;
    vec2 dir2 = c2pos - dist_center2;

    vec3 edge_lengths = vec3(
        length(c2pos - c1pos),
        length(c0pos - c2pos),
        length(c1pos - c0pos)
    );

    vec3 corner_dist = vec3(
    dot(pos - c1pos, normalize(sdir0)),
    dot(pos - c2pos, normalize(sdir1)),
    dot(pos - c0pos, normalize(sdir2))
    ) / edge_lengths;

    //w.x = sin(time)*0.2 + 0.5;
    vec3 orth_distance =
    (corner_dist / w) * (1.-step(w, corner_dist)) +
    (1.-(corner_dist - w) / (1.-w)) * step(w, corner_dist);

    vec3 dist = cos((1.-orth_distance)*3.14159*0.5);
    //dist = orth_distance;

    vec3 weight = normIn * dist * dist_amount;

    //weight = corner_dist*.1;
    //weight.x = 0;
    //weight.y = 0;
    //weight.z = 0;//corner_dist.z*0.1 - 0.1;


    return dir0 * weight.x + dir1 * weight.y + dir2 * weight.z;
}

void main()
{
    vec3 pos = (MVP * vec4(posIn, 1)).xyz;

    vec3 cornerweight = 1 - normIn;

    // Corner positions
    pos.xy = c0pos * cornerweight.x + c1pos * cornerweight.y + c2pos * cornerweight.z;

    // Edge distortion
    pos.xy += distort(pos.xy);

    gl_Position = vec4(pos, 1);

    posOut = posIn;
    uvOut = uvIn;
    normOut = normIn;
}