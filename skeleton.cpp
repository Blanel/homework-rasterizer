#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"

using namespace std;
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

// Camera
int f = 250;
vec3 camPosition(0,0,-2);
vec3 color;

// Transformation
mat3 rot;
float thetaX = 0;
float thetaY = 0;
float thetaZ = 0;

// ----------------------------------------------------------------------------
// FUNCTIONS


void Update();
void Draw();
void VertexShader( const vec3& v, Pixel& p );
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result );
void ComputePolygonRows(
                        const vector<Pixel>& vertexPixels,
                        vector<Pixel>& leftPixels,
                        vector<Pixel>& rightPixels );
void DrawRows(
              const vector<Pixel>& leftPixels,
              const vector<Pixel>& rightPixels );
void DrawPolygon( const vector<vec3>& vertices );
void Rotate();

int main( int argc, char* argv[] )
{
	LoadTestModel( triangles );
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
    
	if( keystate[SDLK_UP] )
        {
            thetaX -= 0.01;
        }
    
	if( keystate[SDLK_DOWN] )
        {
            thetaX += 0.01;
        }
    
	if( keystate[SDLK_RIGHT] )
        {
            thetaY -= 0.01;
        }
    
	if( keystate[SDLK_LEFT] )
        {
            thetaY += 0.01;
        }
    
	if( keystate[SDLK_RSHIFT] )
		;
    
	if( keystate[SDLK_RCTRL] )
		;
    
	if( keystate[SDLK_w] )
		;
    
	if( keystate[SDLK_s] )
		;
    
	if( keystate[SDLK_d] )
		;
    
	if( keystate[SDLK_a] )
		;
    
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
    
    for( int i=0; i<triangles.size(); i++ )
	{
		vector<vec3> vertices(3);
        
		vertices[0] = triangles[i].v0;
		vertices[1] = triangles[i].v1;
		vertices[2] = triangles[i].v2;
        color = triangles[i].color;
        
		// Add drawing
        DrawPolygon( vertices );
        vertices.clear();
    }
	
	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);
    
	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}
void VertexShader( const vec3& v, Pixel& p )
{
    vec3 vLocal = v-camPosition;
    vLocal = rot * vLocal;
    
    p.zinv = 1/vLocal.z;
    p.x = f*(vLocal.x/(vLocal.z))+SCREEN_WIDTH/2;
    p.y = f*(vLocal.y/(vLocal.z))+SCREEN_HEIGHT/2;
}
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result )
{
    int N = result.size();
    vec3 diff = vec3(b.x-a.x,b.y-a.y,b.zinv-a.zinv) / float(max(N-1,1));
    
    vec3 current( a.x, a.y, a.zinv );
    for( int i=0; i<N; ++i )
    {
        result[i].x = current.x;
        result[i].y = current.y;
        result[i].zinv = current.z;
        
        current.x += diff.x;
        current.y += diff.y;
        current.z += diff.z;
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
            }
            rightPixels[row].y = result[i].y;
            if (result[i].x > rightPixels[row].x){
                rightPixels[row].x = result[i].x;
                rightPixels[row].zinv = result[i].zinv;
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
               
                int x = rowPixels[j].x;
                int y = rowPixels[j].y;
                float zinv = rowPixels[j].zinv;
                if (depthBuffer[y][x] < rowPixels[j].zinv ) {
                    
                    depthBuffer[y][x] = zinv;
                    PutPixelSDL( screen, x, y, color );
                
                }
            }
        }
    }
}
void DrawPolygon( const vector<vec3>& vertices )
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

