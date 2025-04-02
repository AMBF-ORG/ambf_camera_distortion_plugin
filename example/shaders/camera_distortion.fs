//per eye texture to warp for lens distortion
uniform sampler2D WarpTexture;

//Position of lens center in normalized pixel-coordinate
uniform vec2 LensCenter;

// width and height
uniform vec2 ImageSize;

// center in unnormalized pixel coordinates, i.e. c_x, c_y
uniform vec2 Center;

// f_x, f_y
uniform vec2 FocalLength;

// Distortion Type
uniform int DistortionType;       // 0 = Pinhole, 1 = Fisheye, 2 = PanoTool

//Distoriton coefficients 
uniform vec4 RadialDistortion;   // k1, k2, k3, k4 (if fisheye, only k1-k4 matter)
uniform vec2 TangentialDistortion; // p1, p2

//chromatic distortion post scaling
uniform vec3 ChromaticAberr;

void main()
{   
    // Normalized texture coordinate [0,1]
    vec2 output_loc = gl_TexCoord[0].xy;
    
    // Convert everything in pixel coordinate
    // Compute fragment location in lens-centered coordinates in pixel coordinates
    vec2 r = (output_loc - LensCenter) * ImageSize / FocalLength;
    
    //|r|
    float r_mag = length(r);

    vec2 r_displaced;

    // Pinhole distortion
    if (DistortionType == 0){
        float r2 = r_mag * r_mag;
        float r4 = r2 * r2;
        float r6 = r4 * r2;

        float radial_factor = 1.0 + RadialDistortion.x * r2 +
                                    RadialDistortion.y * r4 +
                                    RadialDistortion.z * r6;

        // Tangential distortion
        vec2 tangential;
        tangential.x = 2.0 * TangentialDistortion.x * r.x * r.y + TangentialDistortion.y * (r2 + 2.0 * r.x * r.x);
        tangential.y = TangentialDistortion.x * (r2 + 2.0 * r.y * r.y) + 2.0 * TangentialDistortion.y * r.x * r.y;

        // Apply distortion
        r_displaced = r * radial_factor + tangential;
    }
    
    // Fisheye
    else if (DistortionType == 1){
        float theta = atan(r_mag);
        float theta2 = theta * theta;
        float theta4 = theta2 * theta2;
        float theta6 = theta4 * theta2;
        float theta8 = theta4 * theta4;

        float theta_d = theta * (1.0 + RadialDistortion.x * theta2 +
                                       RadialDistortion.y * theta4 +
                                       RadialDistortion.z * theta6 +
                                       RadialDistortion.w * theta8);

        if (r_mag > 0.0) {
            r_displaced = (r / r_mag) * tan(theta_d);
        } else {
            r_displaced = r;
        }
    }
    
    //PANOTOOl
    else if (DistortionType == 2){
        r_displaced = r * (RadialDistortion.w + RadialDistortion.z * r_mag +
        RadialDistortion.y * r_mag * r_mag +
        RadialDistortion.x * r_mag * r_mag * r_mag);
    }

    // Convert back to normalized coordinate
    // wait to recenter after chromatic aberration
    r_displaced = r_displaced * FocalLength / ImageSize;

    //back to viewport co-ord
    vec2 tc_r = (LensCenter + ChromaticAberr.r * r_displaced);
    vec2 tc_g = (LensCenter + ChromaticAberr.g * r_displaced);
    vec2 tc_b = (LensCenter + ChromaticAberr.b * r_displaced);

    float red = texture2D(WarpTexture, tc_r).r;
    float green = texture2D(WarpTexture, tc_g).g;
    float blue = texture2D(WarpTexture, tc_b).b;

    //Black edges off the texture
    if (DistortionType == 1){
        gl_FragColor = ((tc_g.x < 0.0) || (tc_g.x > 1.0) || (tc_g.y < 0.0) || (tc_g.y > 1.0) 
        // || r_mag > 0.5
        ) ? vec4(0.0, 0.0, 0.0, 1.0) : vec4(red, green, blue, 1.0);        
    }
    
    // No black edges off 
    else{
        gl_FragColor = vec4(red, green, blue, 1.0);
    }
};
