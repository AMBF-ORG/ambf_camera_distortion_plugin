//per eye texture to warp for lens distortion
uniform sampler2D WarpTexture;

uniform int DistortionType;

//Position of lens center in normalized pixel-coordinate
uniform vec2 LensCenter;

//Distoriton coefficients 
uniform vec4 DistoritonParam;

//chromatic distortion post scaling
uniform vec3 ChromaticAberr;

void main()
{   
    vec2 output_loc = gl_TexCoord[0].xy;
    
    //Compute fragment location in lens-centered coordinates in pixel coordinata  
    vec2 r = (output_loc  - LensCenter);
    
    //|r|
    float r_mag = length(r);


    // Pinhole distortion
    if (Distoriton == 0){
         // Radial distortion factor
        float radialDistortion = 1.0 + RadialDistortion.x * r2 +
                                    RadialDistortion.y * r4 +
                                    RadialDistortion.z * r6;

        // Tangential distortion
        vec2 tangentialDistortion;
        tangentialDistortion.x = 2.0 * TangentialDistortion.x * r.x * r.y +
                                TangentialDistortion.y * (r2 + 2.0 * r.x * r.x);
        tangentialDistortion.y = TangentialDistortion.x * (r2 + 2.0 * r.y * r.y) +
                                2.0 * TangentialDistortion.y * r.x * r.y;

        // Apply the distortion
        vec2 r_distorted = r * radialDistortion + tangentialDistortion;
    }
    
    // Fisheye
    else if (Distoriton == 1){
        // Apply barrel distortion model
        float r_mag2 = r_mag * r_mag;
        float r_mag4 = r_mag2 * r_mag2;
        float r_mag6 = r_mag4 * r_mag2;

        float distortion = 1.0 + DistoritonParam.x * r_mag2 + 
                                DistoritonParam.y * r_mag4 + 
                                DistoritonParam.z * r_mag6;
        
        // Apply the radial distortion
        vec2 r_displaced = r * distortion;

        //back to viewport co-ord
        vec2 tc_r = (LensCenter + ChromaticAberr.r * r_displaced);
        vec2 tc_g = (LensCenter + ChromaticAberr.g * r_displaced);
        vec2 tc_b = (LensCenter + ChromaticAberr.b * r_displaced);
    }
    
    //PANOTOOl
    else if (Distoriton == 2){

    }

    
    // This does not work
    float red = texture2D(WarpTexture, tc_r).r;
    float green = texture2D(WarpTexture, tc_g).g;
    float blue = texture2D(WarpTexture, tc_b).b;

    // //Black edges off the texture
    gl_FragColor = ((tc_g.x < 0.0) || (tc_g.x > 1.0) || (tc_g.y < 0.0) || (tc_g.y > 1.0)) ? vec4(0.0, 0.0, 0.0, 1.0) : vec4(red, green, blue, 1.0);
    
    // No black edges off 
    // gl_FragColor = vec4(red, green, blue, 1.0);
    // gl_FragColor = vec4(gl_TexCoord[0].xy, 0.0, 1.);
};
