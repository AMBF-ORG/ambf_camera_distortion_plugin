//per eye texture to warp for lens distortion
uniform sampler2D WarpTexture;

// width and height during camera calibration
uniform vec2 ImageSize;

// width and height of simulator window
uniform vec2 WindowSize;

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

// Whether to overlay blackout for circular viewing region
uniform bool Blackout;

void main()
{   
    // Normalized texture coordinate [0,1]
    vec2 output_loc = gl_TexCoord[0].xy;

    // flip the y axis because OpenGL textures have y axis pointing up but
    // Center expect y axis to point down
    output_loc.y = 1.0 - output_loc.y;

    // the segment of the window with an aspect ratio matching the image
    vec2 SubWindowSize = vec2(WindowSize.y * (ImageSize.x / ImageSize.y), WindowSize.y);
    // offset so the subwindow is centered in the window
    vec2 SubWindowOffset = vec2((WindowSize.x - SubWindowSize.x) / 2.0, 0.0);

    // Convert everything to unnormalized camera-calibration pixel coordinates
    // that is, the coordinates relative to the camera before focal length scaling and center translation
    vec2 r = ((output_loc * WindowSize - SubWindowOffset) * ImageSize / SubWindowSize - Center) / FocalLength;
    
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
    vec2 r_displaced_normed = r_displaced * FocalLength * SubWindowSize / ImageSize / WindowSize;

    vec2 LensCenter = Center * SubWindowSize / ImageSize / WindowSize + SubWindowOffset / WindowSize;

    // back to viewport co-ord
    vec2 tc_r = (LensCenter + ChromaticAberr.r * r_displaced_normed);
    vec2 tc_g = (LensCenter + ChromaticAberr.g * r_displaced_normed);
    vec2 tc_b = (LensCenter + ChromaticAberr.b * r_displaced_normed);

    // flip y axis back
    tc_r.y = 1.0 - tc_r.y;
    tc_g.y = 1.0 - tc_g.y;
    tc_b.y = 1.0 - tc_b.y;

    float red = texture2D(WarpTexture, tc_r).r;
    float green = texture2D(WarpTexture, tc_g).g;
    float blue = texture2D(WarpTexture, tc_b).b;

    // Black edges off the texture
    gl_FragColor = (
            (tc_r.x < 0.0) || (tc_r.x > 1.0) || (tc_r.y < 0.0) || (tc_r.y > 1.0) 
            || (tc_g.x < 0.0) || (tc_g.x > 1.0) || (tc_g.y < 0.0) || (tc_g.y > 1.0) 
            || (tc_b.x < 0.0) || (tc_b.x > 1.0) || (tc_b.y < 0.0) || (tc_b.y > 1.0) 
        || (Blackout && r_mag > min(ImageSize[0] / FocalLength[0], ImageSize[1] / FocalLength[1]) / 2.0)
        ) ? vec4(0.0, 0.0, 0.0, 1.0) : vec4(red, green, blue, 1.0);        
};
