#version 300 es
precision mediump float;
precision mediump int;

uniform vec2 resolution;
uniform sampler2D tex0;
uniform vec2 scale;
uniform vec2 viewport;
out vec4 fragColor;

vec2 uv_iq( vec2 uv, vec2 texture_size ) {
    vec2 pixel = uv * texture_size;

    vec2 seam = floor(pixel + 0.5);
    vec2 dudv = fwidth(pixel);
    pixel = seam + clamp( (pixel - seam) / dudv, -0.5, 0.5);

    return pixel / texture_size;
}

void main() {

    vec2 uv = (gl_FragCoord.xy - viewport.xy) / resolution.xy * scale;
    vec2 pixelPerfectUV = uv_iq(uv, resolution.xy);
    fragColor = texture( tex0, pixelPerfectUV );

}

/*
void main() {
  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
*/