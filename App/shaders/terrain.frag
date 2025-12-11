#version 440 core

uniform sampler2D uHeight;
uniform float uHeightScale;
uniform float uTexSize; // résolution de la heightmap
uniform vec3 uHitPos; // Position du hit du raycast
uniform float uHitValid; // 1.0 si hit valide, 0.0 sinon
uniform vec3 uCameraForward; // direction avant de la caméra

// Brush preview
uniform sampler2DArray uBrushArray;
uniform int uBrushIndex;
uniform float uBrushSize;

// Erosion mask overlay
uniform sampler2D uErosionMask; // GL_R8 mask in [0,1]
uniform float uMaskEnabled; // 1.0 if mask should be visualized

in float vHeight;
in vec2 vUV;
in vec3 vWorldPos;

out vec4 fragColor;

// Calcule une normale approchée depuis la texture height (centrée)
vec3 computeNormal(vec2 uv) {
    float texel = 1.0 / uTexSize;
    float hL = texture(uHeight, uv + vec2(-texel, 0.0)).r * uHeightScale;
    float hR = texture(uHeight, uv + vec2( texel, 0.0)).r * uHeightScale;
    float hD = texture(uHeight, uv + vec2(0.0, -texel)).r * uHeightScale;
    float hU = texture(uHeight, uv + vec2(0.0,  texel)).r * uHeightScale;
    // Gradient
    float dx = hR - hL;
    float dz = hU - hD;
    vec3 n = normalize(vec3(-dx, 2.0, -dz));
    return n;
}

vec3 gradientColor(float hNorm) {
    // Couleurs clés
    vec3 low  = vec3(0.08, 0.25, 0.10);   // sombre
    vec3 mid  = vec3(0.15, 0.50, 0.20);   // vert moyen
    vec3 high = vec3(0.80, 0.80, 0.75);   // sommet clair (neige)
    if (hNorm < 0.5) {
        float t = hNorm / 0.5;
        return mix(low, mid, t);
    } else {
        float t = (hNorm - 0.5) / 0.5;
        return mix(mid, high, t);
    }
}

void main() {
    float safeScale = max(uHeightScale, 0.0001);
    float hNorm = clamp(vHeight / safeScale, 0.0, 1.0); // normalisation (0..1)
    vec3 baseCol = gradientColor(hNorm);

    // Normal + lumière directionnelle simple
    vec3 N = computeNormal(vUV);
    vec3 L = normalize(vec3(0.4, 1.0, 0.3));
    float diff = max(dot(N, L), 0.0);
    float ambient = 0.35;
    vec3 lit = baseCol * (ambient + diff * 0.65);

    // Aperçu du brush : on utilise la texture du brush
    if (uHitValid > 0.5 && uBrushSize > 0.0 && uBrushIndex >= 0) {
        // Coordonnées locales [0,1] dans l'espace du brush (plan XZ)
        vec2 delta = vWorldPos.xz - uHitPos.xz;

        // On calcule l'angle de rotation basé sur la direction de la caméra (yaw)
        vec2 cameraDir2D = normalize(uCameraForward.xz);
        float angle = atan(cameraDir2D.x, -cameraDir2D.y);

        // Matrice de rotation 2D
        float c = cos(angle);
        float s = sin(angle);
        mat2 rotation = mat2(c, -s, s, c);

        vec2 rotatedDelta = rotation * delta;
        vec2 local = rotatedDelta / (uBrushSize * 2.0) + 0.5;

        // si on est dans le carré
        if (local.x >= 0.0 && local.x <= 1.0 &&
            local.y >= 0.0 && local.y <= 1.0) {

            float texVal = texture(uBrushArray, vec3(local, float(uBrushIndex))).r;
            lit = mix(lit, vec3(0.627, 0.796, 0.835), texVal);
        }
    }

    // Overlay erosion mask in blue for user feedback
    if (uMaskEnabled > 0.5) {
        float mask = texture(uErosionMask, vUV).r;
        if (mask > 0.01) {
            vec3 blue = vec3(0.1, 0.1, 0.9);
            // Blend proportionally to mask value but keep it subtle
            float a = clamp(mask, 0.2, 0.8);
            lit = mix(lit, blue, a * 0.5);
        }
    }

    fragColor = vec4(lit, 1.0);
}

