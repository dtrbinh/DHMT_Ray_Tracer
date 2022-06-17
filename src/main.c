#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "raytrace.h"

#if defined(__APPLE_CC__)
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

// GLOBAL VARIABLES
unsigned int window_width = 512, window_height = 512;
float pixels[512*512*3];

// Scene information
unsigned int numSpheres = 5;
Sphere spheres[5];

// Viewpoint information
Vector e;
float d = 10;

// Camera basis vectors, must be unit vectors
Vector w;
Vector u;
Vector v;

// Variable to represent image plane
float l = -4, r = 4;
float b = -4, t = 4;

// Global light information
Vector light[3];
float lightI = .7;
int numLights = 1;

// Default background color
RGBf bgColor;

// Control variables for the different features
GLboolean antialias = GL_FALSE;
GLboolean reflection = GL_FALSE;
GLboolean transparency = GL_FALSE;
GLboolean depthOfField = GL_FALSE;


void init() {
    // Set backgroud color
    bgColor = newRGB(0, 0, 0);

    // Set the viewpoint
    e = newVector(5, 0, 0);

    // Calculate basis vectors
    Vector up = newVector(0, 0, 1);
    Vector viewDirection = newVector(-1, 0, 0);
    w = scaleVector(-1/mag(viewDirection), viewDirection);
    Vector upCrossW = cross(up, w);
    u = scaleVector(1/mag(upCrossW), upCrossW);
    v = cross(w, u);

    // Initialize lights in the scene
    light[0] = newVector(0,-1,-1);
    light[0] = scaleVector(1/mag(light[0]),light[0]);

    light[1] = newVector(0,-1,1);
    light[1] = scaleVector(1/mag(light[1]),light[1]);

    light[2] = newVector(-1,1,1);
    light[2] = scaleVector(1/mag(light[2]),light[2]);

    // Create spheres in scene
    spheres[0].r = 1;
    spheres[0].c = newVector(-2, -1, 1);
    spheres[0].color = newRGB(255,255,0);
    spheres[0].id = 3;
    spheres[0].ri = 1;
    spheres[0].reflective = 1;

    spheres[1].r = 1;
    spheres[1].c = newVector(2, -1, -1);
    spheres[1].color = newRGB(0,0,255);
    spheres[1].id = 1;
    spheres[1].ri = 1;
    spheres[1].reflective = 1;

    spheres[2].r = 1;
    spheres[2].c = newVector(0, -1, 1);
    spheres[2].color = newRGB(0,255,0);
    spheres[2].id = 2;
    spheres[2].ri = 1.2;
    spheres[2].reflective = 0;

    spheres[3].r = 1;
    spheres[3].c = newVector(1, 1, -1);
    spheres[3].color = newRGB(180,180,180);
    spheres[3].id = 3;
    spheres[3].ri = 1;
    spheres[3].reflective = 1;

    spheres[4].r = 1;
    spheres[4].c = newVector(-0, 1, 1);
    spheres[4].color = newRGB(255,0,0);
    spheres[4].id = 0;
    spheres[4].ri = 1;
    spheres[4].reflective = 1;
}

float calcIntersection(Ray ray, Sphere sphere) {
    // Compute discriminate
    // (d . (e - c))^2 - (d.d) * ((e-c).(e-c) - r^2)
    Vector eMinusC = minusVector(ray.origin, sphere.c);
    float d2 = dot(ray.direction, ray.direction);
    float discriminate = dot(ray.direction, eMinusC);
    discriminate *= discriminate;
    discriminate -= (d2 * (dot(eMinusC, eMinusC) - pow(sphere.r, 2.0)));

    if (discriminate >= 0) {
        // Solve quadratic for t
        // t = -d . (e-c) +- discriminate / d.d
        float t = dot(scaleVector(-1, ray.direction), eMinusC);
        float t1 = (t + sqrt(discriminate)) / d2;
        float t2 = (t - sqrt(discriminate)) / d2;

        if (t1 > 0 && t2 > 0) {
            if (t1 < t2) {
                return t1;
            } else {
                return t2;
            }
        } else if (t1 > 0) {
            return t1;
        } else if (t2 > 0) {
            return t2;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

GLboolean inShadow(Ray ray) {
    for (int i=0; i<numSpheres; i++) {
        if (calcIntersection(ray,spheres[i]) > 0) {
            return GL_TRUE;
        }
    }
    return GL_FALSE;
}

float sceneHit(Ray ray, Hit* hit) {
    float t = 0;
    float lastT = 9999;
    float result = -1;

    for (int i=0; i<numSpheres; i++) {
        t = calcIntersection(ray, spheres[i]);
        if (t > 0 && t < lastT) {
            result = t;
            hit->sphere = &spheres[i];
        }
    }

    hit->t = result;
}

Ray computeViewingRay(float i, float j, Vector origin) {
    Ray viewingRay;

    float us = l + (r-l) * (i+0.5) / window_width;
    float vs = b + (t-b) * (j+0.5) / window_height;

    viewingRay.origin = origin;
    viewingRay.direction = minusVector(addVector(scaleVector(us,u),scaleVector(vs,v)), viewingRay.origin);
    viewingRay.direction = scaleVector(1/mag(viewingRay.direction), viewingRay.direction);

    return viewingRay;
}

Ray computeViewingNormalRay(float i, float j) {
    Ray viewingRay;

    float us = l + (r-l) * (i+0.5) / window_width;
    float vs = b + (t-b) * (j+0.5) / window_height;

    viewingRay.origin = e;
    viewingRay.direction = addVector(scaleVector(-1*d, w), addVector(scaleVector(us, u), scaleVector(vs, v)));
    viewingRay.direction = scaleVector(1/mag(viewingRay.direction), viewingRay.direction);

    return viewingRay;
}

RGBf diffuse(Vector n, RGBf surfaceColor, int lightNum) {
    float nl = dot(n, light[lightNum]);
    float max = (nl > 0) ? nl : 0;
    float scale = lightI * max;

    return scaleRGB(surfaceColor, scale);
}

RGBf specular(Ray ray, Vector n, int lightNum) {
    RGBf specColor = newRGB(250, 250, 250);
    unsigned int specPow = 40;

    Vector viewingRay = scaleVector(1/mag(ray.direction), ray.direction);
    Vector h = addVector(viewingRay, light[lightNum]);
    h = scaleVector(1/mag(h),h);

    float nh = dot(n,h);

    float max = (nh > 0) ? nh : 0;
    max = pow(max, specPow);

    float scale = max * lightI;

    return scaleRGB(specColor, scale);
}

RGBf ambient(RGBf color) {
    float intensity = 0.2;
    return scaleRGB(color, intensity);
}

RGBf castRay(Ray ray, int recur) {
    Hit hit;
    sceneHit(ray, &hit);

    if (hit.t > 0.001) {
        hit.p = addVector(ray.origin, scaleVector(hit.t-0.0001, ray.direction));
        hit.n = scaleVector(-1/hit.sphere->r, minusVector(hit.p, hit.sphere->c));
        hit.n = scaleVector(1/mag(hit.n), hit.n);
        return shade(hit, ray, recur);
    }

    return bgColor;
}

Ray calcShadowRay(Vector p, Vector lightSource) {
    Ray shadowRay;
    shadowRay.origin = p;
    shadowRay.direction = scaleVector(-1, lightSource);
    return shadowRay;
}

Vector reflect(Vector direction, Vector normal) {
    Vector reflection = scaleVector(1/mag(direction), direction);
    return minusVector(reflection, scaleVector(2 * dot(reflection, normal), normal));
}

GLboolean refract(Vector d, Vector n, float nt, Vector *t) {
    Vector t1 = scaleVector(1/nt, minusVector(d, scaleVector(dot(d, n), n)));
    float root = 1 - (1 - pow(dot(d, n), 2.0)) / pow(nt, 2.0);
    
    if (root < 0) {
        return GL_FALSE;
    }

    Vector t2 = scaleVector(sqrt(root), n);
    *t = minusVector(t1, t2);
    *t = scaleVector(1/mag(*t),*t);

    return GL_TRUE;
}

RGBf attenuate(float kr, float kg, float kb, RGBf color) {
    RGBf result;
    result.r = color.r * kr;
    result.g = color.g * kg;
    result.b = color.b * kb;
    return result;
}

RGBf shade(Hit hit, Ray ray, int recur) {
    RGBf pixelColor = newRGB(0,0,0);

    pixelColor = ambient(hit.sphere->color);

    for (int i=0; i<numLights; i++) {
        Ray shadowRay = calcShadowRay(hit.p,light[i]);
        if (!inShadow(shadowRay)) {
            pixelColor = addRGB(pixelColor, diffuse(hit.n, hit.sphere->color, i));
            pixelColor = addRGB(pixelColor, specular(ray, hit.n, i));
        }
    }

    if (reflection && hit.sphere->reflective && recur > 0) {
        Ray reflectRay;
        reflectRay.origin = hit.p;
        reflectRay.direction = reflect(ray.direction, hit.n);
        pixelColor = addRGB(pixelColor, scaleRGB(castRay(reflectRay, recur-1), 0.25));
    }

    if (transparency && hit.sphere->ri != 1 && recur > 0) {
        Vector r = reflect(ray.direction, hit.n);
        float kr, kg, kb;
        Ray ray1, ray2;
        Vector t;
        float c;


        if (dot(ray.direction, hit.n) < 0) {
            refract(ray.direction, hit.n, hit.sphere->ri, &t);
            c = dot(scaleVector(-1, ray.direction), hit.n);
        } else {
            if (refract(ray.direction, scaleVector(-1,hit.n), 1/hit.sphere->ri, &t)) {
                c = dot(t, hit.n);
            } else {
                ray1.origin = hit.p;
                ray1.direction = r;
                return addRGB(pixelColor, castRay(ray1, recur-1));
            }
        }

        float r0 = pow(hit.sphere->ri-1, 2.0) / pow(hit.sphere->ri+1, 2.0);
        float r1 = r0 + (1-r0) * pow(1-c, 5.0);

        ray1.origin = hit.p;
        ray1.direction = r;

        ray2.origin = hit.p;
        ray2.direction = t;

        return addRGB(pixelColor,addRGB(scaleRGB(castRay(ray1, recur-1), r1), scaleRGB(castRay(ray2, recur-1), (1-r1))));
    }

    return pixelColor;
}

RGBf antialiasPixel(int i, int j) {
    float samples = 5;
    RGBf pixelColor = newRGB(0,0,0);
    float x,y;
    float r;

    for (int p=0; p<samples; p++) {
        for (int q=0; q<samples; q++) {
            r = (rand() % 100)/100.0f;

            x = (float)i + ((float)p+r) / samples;
            y = (float)j + ((float)q+r) / samples;

            Vector origin = e;

            if (depthOfField) {
                origin.y += ((float)q+r) / samples;
                origin.z += ((float)p+r) / samples;
            }

            // Compute viewing ray
            Ray viewingRay = computeViewingRay(x,y,origin);

            pixelColor = addRGB(pixelColor, castRay(viewingRay,3));
        }
    }
    
    return scaleRGB(pixelColor, 1/pow(samples,2.0));
}

// Display method generates the image
void display(void) {
    // Reset drawing window
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    srand(time(NULL));

    // Ray-tracing loop, for each pixel
    RGBf pixelColor;
    for (int i=0; i<window_height; i++) {
        for (int j=0; j<window_width; j++) {
            
            if (antialias || depthOfField) {
                pixelColor = antialiasPixel(i,j);
            } else {
                pixelColor = castRay(computeViewingRay(i,j,e), 5);
            }

            // Update pixel color to result from ray
            setPixelColor(pixelColor, (RGBf*) &pixels[(j*window_width*3) + (i*3)]);
            //RGBf* px = (RGBf*) &pixels[(j*window_width*3) + (i*3)];     
            //setPixelColor(pixelColor, px);
        }
    }

    // Draw the pixel array
    glDrawPixels(window_width, window_height, GL_RGB, GL_FLOAT, pixels);

    // Reset buffer for next frame
    glutSwapBuffers();
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
}

void idle(void) {
    glutPostRedisplay();
}

void toggle(GLboolean* toggle) {
    *toggle = !(*toggle);
}

void keyboard(unsigned char key, int x, int y) {    
    switch (key) {
        case 'h':
            printf("HELP\n");
            printf("----\n");
            printf("a - toggle antialiasing\n");
            printf("d - toggle depth of field\n");
            printf("r - toggle reflections\n");
            printf("t - toggle transparency\n");
            printf("l - increase number of lights (max: 3)\n");
            printf("k - decrease number of lights (min: 1)\n");
            break;
        case 'a':
            toggle(&antialias);
            break;
        case 'd':
            toggle(&depthOfField);
            break;
        case 'r':
            toggle(&reflection);
            break;
        case 't':
            toggle(&transparency);
            break;
        case 'l':
            numLights += (numLights < 3) ? 1 : 0;
            break;
        case 'k':
            numLights -= (numLights > 1) ? 1 : 0;
            break;
    }
    
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    init();

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);

    glutCreateWindow("Project Ray Tracing DHMT");
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);

    glutMainLoop();

    return EXIT_SUCCESS;
}
