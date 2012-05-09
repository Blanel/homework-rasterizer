#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"

using namespace std;
using glm::vec4;
using glm::vec3;
using glm::vec2;
using glm::mat3;

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

struct Pixel
{
    int x;
    int y;
    float zinv;
    vec3 pos3d;
};

struct Vertex
{
    vec3 position;
};

// Screen
const int SCREEN_HEIGHT = 500;
const int SCREEN_WIDTH = 500;
SDL_Surface* screen;

// Ticker
int t;

// World
vector<Triangle> triangles;
float depthBuffer[SCREEN_HEIGHT+1][SCREEN_WIDTH+1];
vec3 currentNormal;

// Camera
int f = 250;
vec3 camPosition(0,0,-2);
vec3 color;

// Light
vec3 lightPos( 0, -0.5, -0.7 );
vec3 lightPower = 16.f * vec3( 1, 1, 1 );
vec3 indirectLightPowerPerArea = 0.5f*vec3( 1, 1, 1 );

// Transformation
mat3 rot;
float thetaX = 0;
float thetaY = 0;
float thetaZ = 0;
// ----------------------------------------------------------------------------
// FUNCTIONS


void Update();
void Draw();
void VertexShader( const Vertex& v, Pixel& p );
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result );
void ComputePolygonRows(
                        const vector<Pixel>& vertexPixels,
                        vector<Pixel>& leftPixels,
                        vector<Pixel>& rightPixels );
void DrawRows(
              const vector<Pixel>& leftPixels,
              const vector<Pixel>& rightPixels );
void DrawPolygon( const vector<Vertex>& vertices );
void Rotate();
void PixelShader( const Pixel& p );
vec3 Light( const Pixel& i );

int main( int argc, char* argv[] )
{
        LoadTestModel( triangles );
    Rotate();
        screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
        t = SDL_GetTicks();	// Set start value for timer.

        while( NoQuitMessageSDL() )
        {
                Update();
                Draw();
    }

        SDL_SaveBMP( screen, "screenshot.bmp" );
        return 0;
}
void Update()
{
        // Compute frame time:
        int t2 = SDL_GetTicks();
        float dt = float(t2-t);
        t = t2;
        cout << "Render time: " << dt << " ms." << endl;

        Uint8* keystate = SDL_GetKeyState(0);

        if( keystate[SDLK_y] )
        {
            Rotate();
            camPosition += vec3(0,0,0.01)*rot;
            //DEBUG=true;

        }

        if( keystate[SDLK_h] )
        {
            Rotate();
            camPosition -= vec3(0,0,0.01)*rot;
        }

        if( keystate[SDLK_j] )
        {
            Rotate();
            camPosition += vec3(0.01,0,0)*rot;
        }

        if( keystate[SDLK_g] )
        {
            Rotate();
            camPosition -= vec3(0.01,0,0)*rot;
        }
        if( keystate[SDLK_UP] )
        {
            thetaX-=0.01;

        }

        if( keystate[SDLK_DOWN] )
        {
            thetaX+=0.01;
        }

        if( keystate[SDLK_RIGHT] )
        {
            thetaY-=0.01;
        }

        if( keystate[SDLK_LEFT] )
        {
            thetaY+=0.01;
        }

        if( keystate[SDLK_RSHIFT] )
                ;

        if( keystate[SDLK_RCTRL] )
                ;

    // Light movement

    if( keystate[SDLK_w] )
                lightPos.z += 0.1;

        if( keystate[SDLK_s] )
                lightPos.z -= 0.1;

        if( keystate[SDLK_a] )
                lightPos.x -= 0.1;

        if( keystate[SDLK_d] )
                lightPos.x += 0.1;

        if( keystate[SDLK_e] )
                ;

        if( keystate[SDLK_q] )
                ;

    Rotate();

}
void Rotate()
{
    rot[0][0] = cos(thetaY)*cos(thetaZ);
    rot[1][0] = sin(thetaX)*sin(thetaY)*cos(thetaZ)-cos(thetaX)*sin(thetaZ);
    rot[2][0] = sin(thetaX)*sin(thetaZ)+cos(thetaX)*sin(thetaY)*cos(thetaZ);
    rot[0][1] = cos(thetaY)*sin(thetaZ);
    rot[1][1] = cos(thetaX)*cos(thetaZ)+sin(thetaX)*sin(thetaY)*sin(thetaZ);
    rot[2][1] = cos(thetaX)*sin(thetaY)*sin(thetaZ)-sin(thetaX)*cos(thetaZ);
    rot[0][2] = -sin(thetaY);
    rot[1][2] = sin(thetaX)*cos(thetaY);
    rot[2][2] = cos(thetaX)*cos(thetaY);
}
void Draw()
{
        SDL_FillRect( screen, 0, 0 );

        if( SDL_MUSTLOCK(screen) )
                SDL_LockSurface(screen);

        // Clear the depthBuffer
    for( int y=0; y<SCREEN_HEIGHT; ++y )
        for( int x=0; x<SCREEN_WIDTH; ++x )
            depthBuffer[y][x] = 0;

    for( int i=0; i<triangles.size(); ++i )
        {
                vector<Vertex> vertices(3);

        vertices[0].position = triangles[i].v0;
        vertices[1].position = triangles[i].v1;
        vertices[2].position = triangles[i].v2;


        // vertices[j].reflectance = 1;

        color = triangles[i].color;
        currentNormal = triangles[i].normal;

                // Add drawing
        DrawPolygon( vertices );

    }

        if ( SDL_MUSTLOCK(screen) )
                SDL_UnlockSurface(screen);

        SDL_UpdateRect( screen, 0, 0, 0, 0 );
}
void VertexShader( const Vertex& v, Pixel& p )
{
    vec3 vLocal = v.position-camPosition;

    vLocal = rot*vLocal;

    p.zinv = 1/vLocal.z;
    p.x = (f * vLocal.x * p.zinv)+SCREEN_WIDTH/2;
    p.y = (f * vLocal.y * p.zinv)+SCREEN_HEIGHT/2;
    p.pos3d = v.position;


}
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result )
{
    int N = result.size();
    vec3 diff = vec3(b.x-a.x,b.y-a.y,b.zinv-a.zinv) / float(max(N-1,1));
    vec3 diffPos = vec3(b.pos3d*b.zinv - a.pos3d*a.zinv) / float(max(N-1,1));

    vec3 current( a.x, a.y, a.zinv);
    vec3 currentPos(a.pos3d*a.zinv);
    for( int i=0; i<N; ++i )
    {
        result[i].x = current.x;
        result[i].y = current.y;
        result[i].zinv = current.z;
        result[i].pos3d = currentPos/current.z;

        current.x += diff.x;
        current.y += diff.y;
        current.z += diff.z;
        currentPos += diffPos;
    }
}
void ComputePolygonRows(
                        const vector<Pixel>& vertexPixels,
                        vector<Pixel>& leftPixels,
                        vector<Pixel>& rightPixels )
{
    // 1. Find max and min y-value of the polygon
    // and compute the number of rows it occupies.

    int V = vertexPixels.size();
    int max = -numeric_limits<int>::max();
    int min = +numeric_limits<int>::max();

    for (int i = 0; i < V; ++i)
    {
        max = vertexPixels[i].y > max ? vertexPixels[i].y : max;
        min = vertexPixels[i].y < min ? vertexPixels[i].y : min;
    }
    int ROWS = max-min+1;

    // 2. Resize leftPixels and rightPixels
    // so that they have an element for each row.

    leftPixels.resize(ROWS);
    rightPixels.resize(ROWS);

    // 3. Initialize the x-coordinates in leftPixels
    // to some really large value and the x-coordinates
    // in rightPixels to some really small value.

    for( int i=0; i < ROWS; ++i )
    {
        leftPixels[i].x = +numeric_limits<int>::max();
        rightPixels[i].x = -numeric_limits<int>::max();
    }

    // 4. Loop through all edges of the polygon and use
    // linear interpolation to find the x-coordinate for
    // each row it occupies. Update the corresponding
    // values in rightPixels and leftPixels.

    for( int i=0; i<V; ++i )
    {
        int j = (i+1)%V;                    // The next vertex
        int EDGE_ROWS = abs( vertexPixels[i].y - vertexPixels[j].y ) +1;
        vector<Pixel> result(EDGE_ROWS);
        Interpolate(vertexPixels[i], vertexPixels[j], result);
        for (int i = 0; i < result.size(); ++i){
            int row = result[i].y-min;
            leftPixels[row].y = result[i].y;

            if (result[i].x < leftPixels[row].x){
                leftPixels[row].x = result[i].x;
                leftPixels[row].zinv = result[i].zinv;
                leftPixels[row].pos3d = result[i].pos3d;

            }
            rightPixels[row].y = result[i].y;

            if (result[i].x > rightPixels[row].x){
                rightPixels[row].x = result[i].x;
                rightPixels[row].zinv = result[i].zinv;
                rightPixels[row].pos3d = result[i].pos3d;
            }
        }
    }
}
void DrawRows(
              const vector<Pixel>& leftPixels,
              const vector<Pixel>& rightPixels )
{
    for (int i = 0; i < leftPixels.size(); ++i) {
        if (rightPixels[i].x != -numeric_limits<int>::max() && leftPixels[i].x != +numeric_limits<int>::max() ) {
            vector<Pixel> rowPixels(rightPixels[i].x-leftPixels[i].x+1);
            Interpolate(leftPixels[i], rightPixels[i], rowPixels);
            for( int j = 0; j < rowPixels.size(); ++j) {
                PixelShader(rowPixels[j]);
            }
        }
    }
}
void DrawPolygon( const vector<Vertex>& vertices )
{
    int V = vertices.size();
    vector<Pixel> vertexPixels( V );
    for( int i=0; i<V; ++i )
        VertexShader( vertices[i], vertexPixels[i] );

    vector<Pixel> leftPixels;
    vector<Pixel> rightPixels;

    ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
    DrawRows( leftPixels, rightPixels );
}
void PixelShader( const Pixel& p )
{
    int x = p.x;
    int y = p.y;
    if( x < SCREEN_WIDTH && x > 0 && y < SCREEN_HEIGHT && y > 0 && p.zinv > depthBuffer[y][x])
    {
        depthBuffer[y][x] = p.zinv;
        PutPixelSDL( screen, x, y, Light(p)*color);
    }
}
vec3 Light( const Pixel& i ){

    vec3 r = lightPos - i.pos3d ;
    vec3 rHat = glm::normalize(r);

    float rLength = glm::length(r);

    float rRatio = glm::dot(rHat,currentNormal);
    float ratio = rRatio >= 0 ? rRatio : 0;

    float A = (4*3.14*rLength*rLength);
    vec3 B = lightPower/A;
    vec3 D = B*ratio;

    return D+indirectLightPowerPerArea;
}
