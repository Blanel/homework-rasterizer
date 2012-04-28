#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"

using namespace std;
using glm::vec3;
using glm::ivec2;
using glm::vec2;
using glm::mat3;

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
SDL_Surface* screen;
int t;
vector<Triangle> triangles;
int f = 250;
vec3 camPosition(0,0,-2);
vec3 color;
vector<ivec2> leftPixels;
vector<ivec2> rightPixels;

// ----------------------------------------------------------------------------
// FUNCTIONS

void Update();
void Draw();
void VertexShader( const vec3& v, ivec2& p );
void Interpolate( ivec2 a, ivec2 b, vector<ivec2>& result );
void ComputePolygonRows(
                        const vector<ivec2>& vertexPixels,
                        vector<ivec2>& leftPixels,
                        vector<ivec2>& rightPixels );
void DrawRows(
              const vector<ivec2>& leftPixels,
              const vector<ivec2>& rightPixels );
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
void VertexShader( const vec3& v, ivec2& p )
{
    vec3 vLocal = v-camPosition;
    p.x = f*(vLocal.x/(vLocal.z))+SCREEN_WIDTH/2;
    p.y = f*(vLocal.y/(vLocal.z))+SCREEN_HEIGHT/2;
}
void Interpolate( ivec2 a, ivec2 b, vector<ivec2>& result )
{
    int N = result.size();
    vec2 step = vec2(b-a) / float(max(N-1,1));
    vec2 current( a );
    for( int i=0; i<N; i++ )
    {
        result[i] = current;
        current += step;
    }
}
void ComputePolygonRows(
                        const vector<ivec2>& vertexPixels,
                        vector<ivec2>& leftPixels,
                        vector<ivec2>& rightPixels )
{
    // 1. Find max and min y-value of the polygon
    // and compute the number of rows it occupies.
    
    int V = vertexPixels.size();
    int max = 0;
    int min = 1000;
    int ROWS = 0;
    
    for (int i = 0; i < V; i++)
    {
        max = vertexPixels[i].y > max ? vertexPixels[i].y : max;
        min = vertexPixels[i].y < min ? vertexPixels[i].y : min;
        ROWS = max-min+1;
    }
    
    
    // 2. Resize leftPixels and rightPixels
    // so that they have an element for each row.
    
    leftPixels.resize(ROWS);
    rightPixels.resize(ROWS);
    
    
    // 3. Initialize the x-coordinates in leftPixels
    // to some really large value and the x-coordinates
    // in rightPixels to some really small value.
    
    for( int i=0; i < ROWS; i++ )
    {
        leftPixels[i].x = +numeric_limits<int>::max();
        rightPixels[i].x = -numeric_limits<int>::max();
    }
    
    // 4. Loop through all edges of the polygon and use
    // linear interpolation to find the x-coordinate for
    // each row it occupies. Update the corresponding
    // values in rightPixels and leftPixels.
    
    // Loop over all vertices and draw the edge from it to the next vertex:
    
    vector<ivec2> result(0);
    for( int i=0; i<V; i++ )
    {
        int j = (i+1)%V;                    // The next vertex
        vector<ivec2> resultVertex(ROWS);
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
void DrawRows(
              const vector<ivec2>& leftPixels,
              const vector<ivec2>& rightPixels )
{
    for (int i = 0; i < leftPixels.size(); i++) {
        vector<ivec2> rowPixels(rightPixels[i].x-leftPixels[i].x+1);
        Interpolate(leftPixels[i], rightPixels[i], rowPixels);
        for( int j = 0; j < rowPixels.size(); j++) {
            PutPixelSDL( screen, rowPixels[j].x, rowPixels[j].y, color );
        }
    }
}
void DrawPolygon( const vector<vec3>& vertices )
{
    int V = vertices.size();
    vector<ivec2> vertexPixels( V );
    for( int j=0; j<V; j++ )
        VertexShader( vertices[j], vertexPixels[j] );
    
    ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
    DrawRows( leftPixels, rightPixels );
        
    leftPixels.clear();
    rightPixels.clear();
}

