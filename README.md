# ambf_camera_distortion_plugin
AMBF plugin to 

This project is a plugin for Asynchronous Multibody Framework ([AMBF](https://github.com/WPI-AIM/ambf)) developed by Munawar et al. 
This plugin will apply the camera distrotion (radial, tangential, fisheye etc).

Author: Hisashi Ishida(hishida3@jhu.edu)

## 1. Installation Instructions:
Lets call the absolute location of this package as **<plugin_path>**. E.g. if you cloned this repo in your home folder, **<plugin_path>** = `~/ambf_camera_distortion_plugin/` OR `/home/<username>/ambf_camera_distortion_plugin`.

### 1.1 clone and build 
```bash
git clone git@github.com:LCSR-CIIS/ambf_camera_distortion_plugin.git
cd ambf_camera_distortion_plugin
mkdir build && cd build
make -j7
```

## 2. How to use this plugin
You can use the following command to run the example:
```bash
ambf_simulator --launch_file example/launch.yaml -l 0
```

In the ADF file, specify the plugin under the camera you want to apply the distortion: 
```yaml
distorted_camera:
  namespace: cameras/
  name: distorted_camera
  location: {x: 2.0, y: 0.0, z: 0.0}
  look at: {x: -1.0, y: 0.0, z: 0.0}
  up: {x: 0.0, y: 0.0, z: 1.0}
  clipping plane: {near: 2.0135309278350517, far: 1006.7654639175257}
  field view angle: 0.17951634837990105 # so that focal length is approximately 1000px and phantom is 250 mm away
  monitor: 0
  visible: true
  plugins: [
    {
      name: camera_distortion_plugin,
      path: ../../../build,
      filename: libambf_camera_distortion_plugin.so,
      distortion_config: example/config_file/example_pinhole.yaml,
      vertex_shader: example/shaders/camera_distortion.vs,
      fragment_shader: example/shaders/camera_distortion.fs
    }
  ]
```
[Caution] Change the path in `distortion_config` to apply the different camera distortion. Please refer to the next section. 

## 3. Configuration file
Example configuration files (`pinhole`, `fisheye`, `panotool`) are located in `example/config_file`.
For panotool, please refer to this [document](https://github.com/OpenHMD/OpenHMD/wiki/Universal-Distortion-Shader) for further informaion about the model.

```yaml
## Example json file for ambf_camera_distortion_plugin
## Type: pinhole

type: pinhole
intrinsic:
  fx: 500.0
  fy: 500.0
  cx: 320.0
  cy: 240.0
radial_distortion_coeffs: [0.0, 0.0, 0.0, 0.0]  # k1, k2, k3, k4
tangential_distortion_coeffs: [0.0, 0.0]  # p1, p2
chromatic_distortion: [1.0, 1.0, 1.0] # [Optional]
```



## Fragment shader
All the distortions are applied in the [fragment shader](example/shaders/camera_distortion.fs). You can add different distortion formulation in this file.
```fs
//per eye texture to warp for lens distortion
uniform sampler2D WarpTexture;

//Position of lens center in normalized pixel-coordinate
uniform vec2 LensCenter;

// Distortion Type
uniform int DistortionType;       // 0 = Pinhole, 1 = Fisheye, 2 = PanoTool

//Distoriton coefficients 
uniform vec4 RadialDistortion;   // k1, k2, k3, k4 (if fisheye, only k1-k4 matter)
uniform vec2 TangentialDistortion; // p1, p2

//chromatic distortion post scaling
uniform vec3 ChromaticAberr;

void main()
{   
    vec2 output_loc = gl_TexCoord[0].xy;
    
    //Compute fragment location in lens-centered coordinates in pixel coordinata  
    vec2 r = (output_loc  - LensCenter);
    
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
//per eye texture to warp for lens distortion
uniform sampler2D WarpTexture;

//Position of lens center in normalized pixel-coordinate
uniform vec2 LensCenter;

// Distortion Type
uniform int DistortionType;       // 0 = Pinhole, 1 = Fisheye, 2 = PanoTool

//Distoriton coefficients 
uniform vec4 RadialDistortion;   // k1, k2, k3, k4 (if fisheye, only k1-k4 matter)
uniform vec2 TangentialDistortion; // p1, p2

//chromatic distortion post scaling
uniform vec3 ChromaticAberr;

void main()
{   
    vec2 output_loc = gl_TexCoord[0].xy;
    
    //Compute fragment location in lens-centered coordinates in pixel coordinata  
    vec2 r = (output_loc  - LensCenter);
    
    //|r|
    float r_mag = length(r);

    vec2 r_displaced;

    // Pinhole distortion
    if (DistortionType == 0){
        float r2 = r_mag * r_mag;
        float r4 = r2 * r//per eye texture to warp for lens distortion
uniform sampler2D WarpTexture;

//Position of lens center in normalized pixel-coordinate
uniform vec2 LensCenter;

// Distortion Type
uniform int DistortionType;       // 0 = Pinhole, 1 = Fisheye, 2 = PanoTool

//Distoriton coefficients 
uniform vec4 RadialDistortion;   // k1, k2, k3, k4 (if fisheye, only k1-k4 matter)
uniform vec2 TangentialDistortion; // p1, p2

//chromatic distortion post scaling
uniform vec3 ChromaticAberr;

void main()
{   
    vec2 output_loc = gl_TexCoord[0].xy;
    
    //Compute fragment location in lens-centered coordinates in pixel coordinata  
    vec2 r = (output_loc  - LensCenter);
    
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

    //back to viewport co-ord
    vec2 tc_r = (LensCenter + ChromaticAberr.r * r_displaced);
    vec2 tc_g = (LensCenter + ChromaticAberr.g * r_displaced);
    vec2 tc_b = (LensCenter + ChromaticAberr.b * r_displaced);

    float red = texture2D(WarpTexture, tc_r).r;
    float green = texture2D(WarpTexture, tc_g).g;
    float blue = texture2D(WarpTexture, tc_b).b;

    // //Black edges off the texture
    gl_FragColor = ((tc_g.x < 0.0) || (tc_g.x > 1.0) || (tc_g.y < 0.0) || (tc_g.y > 1.0)) ? vec4(0.0, 0.0, 0.0, 1.0) : vec4(red, green, blue, 1.0);
    
    // No black edges off 
    gl_FragColor = vec4(red, green, blue, 1.0);
    // gl_FragColor = vec4(gl_TexCoord[0].xy, 0.0, 1.);
};
or = 1.0 + RadialDistortion.x * r2 +
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

    //back to viewport co-ord
    vec2 tc_r = (LensCenter + ChromaticAberr.r * r_displaced);
    vec2 tc_g = (LensCenter + ChromaticAberr.g * r_displaced);
    vec2 tc_b = (LensCenter + ChromaticAberr.b * r_displaced);

    float red = texture2D(WarpTexture, tc_r).r;
    float green = texture2D(WarpTexture, tc_g).g;
    float blue = texture2D(WarpTexture, tc_b).b;

    // //Black edges off the texture
    gl_FragColor = ((tc_g.x < 0.0) || (tc_g.x > 1.0) || (tc_g.y < 0.0) || (tc_g.y > 1.0)) ? vec4(0.0, 0.0, 0.0, 1.0) : vec4(red, green, blue, 1.0);
    
    // No black edges off 
    gl_FragColor = vec4(red, green, blue, 1.0);
    // gl_FragColor = vec4(gl_TexCoord[0].xy, 0.0, 1.);
};
 * TangentialDistortion.x * r.x * r.y + TangentialDistortion.y * (r2 + 2.0 * r.x * r.x);
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

    //back to viewport co-ord
    vec2 tc_r = (LensCenter + ChromaticAberr.r * r_displaced);
    vec2 tc_g = (LensCenter + ChromaticAberr.g * r_displaced);
    vec2 tc_b = (LensCenter + ChromaticAberr.b * r_displaced);

    float red = texture2D(WarpTexture, tc_r).r;
    float green = texture2D(WarpTexture, tc_g).g;
    float blue = texture2D(WarpTexture, tc_b).b;

    // //Black edges off the texture
    gl_FragColor = ((tc_g.x < 0.0) || (tc_g.x > 1.0) || (tc_g.y < 0.0) || (tc_g.y > 1.0)) ? vec4(0.0, 0.0, 0.0, 1.0) : vec4(red, green, blue, 1.0);
    
    // No black edges off 
    gl_FragColor = vec4(red, green, blue, 1.0);
    // gl_FragColor = vec4(gl_TexCoord[0].xy, 0.0, 1.);
};
```
