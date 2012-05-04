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
const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
SDL_Surface* screen;

// Ticker
int t;

// World
vector<Triangle> triangles;

// Camera
int f = 250;
vec3 camPosition(0,0,-2);

// Misc
vec3 color;
vector<Pixel> leftPixels();
vector<Pixel> rightPixels;

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
		;
    
	if( keystate[SDLK_DOWN] )
		;
    
	if( keystate[SDLK_RIGHT] )
		;
    
	if( keystate[SDLK_LEFT] )
		;
    
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
}
void Draw()
{
	SDL_FillRect( screen, 0, 0 );
    
	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);
	
	for( int i=0; i<triangles.size(); ++i )
	{
		vector<vec3> vertices(3);
        
		vertices[0] = triangles[i].v0;
		vertices[1] = triangles[i].v1;
		vertices[2] = triangles[i].v2;
        color = triangles[i].color;
        
		// Add drawing
        
        DrawPolygon( vertices );
        // vertices.clear();
    }
	
	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);
    
	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}
void DrawPolygon( const vector<vec3>& vertices )
{
    int V = vertices.size();
    vector<Pixel> vertexPixels( V );
    for( int i=0; i<V; ++i )
        VertexShader( vertices[i], vertexPixels[i] );
    
    ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
    DrawRows( leftPixels, rightPixels );
    
    leftPixels.clear();
    rightPixels.clear();
}
void VertexShader( const vec3& v, Pixel& p )
{
    vec3 vLocal = v-camPosition;
    p.x = f*(vLocal.x/(vLocal.z))+SCREEN_WIDTH/2;
    p.y = f*(vLocal.y/(vLocal.z))+SCREEN_HEIGHT/2;
    p.zinv = 1/vLocal.z;
}

void ComputePolygonRows(
                        const vector<Pixel>& vertexPixels,
                        vector<Pixel>& leftPixels,
                        vector<Pixel>& rightPixels )
{
    // 1. Find max and min y-value of the polygon
    // and compute the number of rows it occupies.
    
    int V = vertexPixels.size();
    int max = -1;
    int min = 501;
    int ROWS;
    
    for (int i = 0; i < V; ++i)
    {
        max = vertexPixels[i].y > max ? vertexPixels[i].y : max;
        min = vertexPixels[i].y < min ? vertexPixels[i].y : min;
        ROWS = max-min+1;
    }
    
    
    // 2. Resize leftPixels and rightPixels
    // so that they have an element for each row.
    
    leftPixels.clear();
    rightPixels.clear();
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
    
    // Loop over all vertices and compute the edge from it to the next vertex:
    
    vector<Pixel> result(V*ROWS);
    for( int i=0; i<V; ++i )
    {
        int j = (i+1)%V;                    // The next vertex
        vector<Pixel> resultVertex(ROWS);
        Interpolate(vertexPixels[i], vertexPixels[j], resultVertex);
        result.insert(result.end(), resultVertex.begin(), resultVertex.end());
    }
    
    int row;
    for (int i = 0; i < result.size(); i++) {
        row = result[i].y-min;
        leftPixels[row].y = result[i].y;
        leftPixels[row].x = result[i].x < leftPixels[row].x ? result[i].x : leftPixels[row].x;
        rightPixels[row].y = result[i].y;
        rightPixels[row].x = result[i].x > rightPixels[row].x ? result[i].x : rightPixels[row].x;
    }
}
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result )
{
    int N = result.size();
    vec3 diff = vec3(b.x-a.x,b.y-a.y,b.zinv-a.zinv) / float(max(N-1,1));
    Pixel step;
    step.x = diff.x;
    step.y = diff.y;
    step.zinv = diff.z;
    Pixel current( a );
    for( int i=0; i<N; i++ )
    {
        result[i] = current;
        current.x += step.x;
        current.y += step.y;
        current.zinv += step.zinv;
        
        cout << "Start: ("
        << result[i].x << ","
        << result[i].y << ")." << endl;
        //<< rightPixels[row].x << ","
        //<< rightPixels[row].y << "). " 
    }
}

void DrawRows(
              const vector<Pixel>& leftPixels,
              const vector<Pixel>& rightPixels )
{
    for (int i = 0; i < leftPixels.size(); i++) {
        vector<Pixel> rowPixels(rightPixels[i].x-leftPixels[i].x+1);
        Interpolate(leftPixels[i], rightPixels[i], rowPixels);
        for( int j = 0; j < rowPixels.size(); j++) {
            PutPixelSDL( screen, rowPixels[j].x, rowPixels[j].y, color );
        }
    }
}


