#include <stdio.h>
#include <math.h>
//#include <cmath.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#define DEBUG_LOG 0

struct Settings
{
	bool defaultAllow = true;
	bool drawontop = true;
	std::vector<std::string> allows;
	std::vector<std::string> disallows;
	std::vector<std::string> musts;
	std::vector<std::string> avoids;
	int duration = 60;
	bool drawTriggerOutlines = true;
	bool drawEntCubes = false;
};
// Vector 3
struct Vector3
{
	float x = 0;
	float y = 0;
	float z = 0;

	// This just exists for compat with the std::vector code
	float& operator[](int i) {
		return reinterpret_cast<float*>(this)[i];
	}
	const float& operator[](int i) const {
		return reinterpret_cast<const float*>(this)[i];
	}
};

Vector3 operator+ (const Vector3& l, const Vector3& r)
{
	return { l.x + r.x, l.y + r.y, l.z + r.z };
}
Vector3 operator* (const Vector3& l, float r)
{
	return { l.x * r, l.y * r, l.z * r };
}
Vector3& operator+= (Vector3& l, const Vector3& r)
{
	l.x += r.x;
	l.y += r.y;
	l.z += r.z;
	return l;
}
Vector3& operator*= (Vector3& l, float r)
{
	l.x *= r;
	l.y *= r;
	l.z *= r;
	return l;
}
bool operator== (const Vector3& l, const Vector3& r)
{
	return (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
}

// Vector 4
struct Vector4
{
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;

	// This just exists for compat with the std::vector code
	float& operator[](int i) {
		return reinterpret_cast<float*>(this)[i];
	}
	const float& operator[](int i) const {
		return reinterpret_cast<const float*>(this)[i];
	}
};

Vector4 operator+ (const Vector4& l, const Vector4& r)
{
	return { l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w };
}
Vector4 operator* (const Vector4& l, float r)
{
	return { l.x * r, l.y * r, l.z * r, l.w * r };
}
Vector4& operator+= (Vector4& l, const Vector4& r)
{
	l.x += r.x;
	l.y += r.y;
	l.z += r.z;
	l.w += r.w;
	return l;
}
Vector4& operator*= (Vector4& l, float r)
{
	l.x *= r;
	l.y *= r;
	l.z *= r;
	l.w *= r;
	return l;
}
Vector4& operator/= (Vector4& l, float r)
{
	l.x /= r;
	l.y /= r;
	l.z /= r;
	l.w /= r;
	return l;
}

// Matrix 3x3
struct Matrix3x3
{
	Vector3 a;
	Vector3 b;
	Vector3 c;
};

struct Plane
{
	Vector3 normal;
	float dist = 0;
	bool skip = false;
	bool bbox = false;
};

struct Edge
{
	// Stem is the starting position, tail is the ending position
	// I just... want the equal spacing...
	Vector3 stem;
	Vector3 tail;
};

struct Brush
{
	std::vector<Plane> planes;
	std::vector<Edge> edges;
};

struct Entity
{
	std::string editorclass;
	std::string classname;
	std::string targetname;
	std::string scriptflag;
	std::string spawnclass;

	Vector3 origin;
	Vector3 mins;
	Vector3 maxs;

	bool isTrigger = false;
	std::vector<Brush> brushes;
};

Vector3 crossProduct(const Vector3& l, const Vector3& r)
{
	return { l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x };
}

float dotProduct(const Vector3& l, const Vector3& r)
{
	return l.x * r.x + l.y * r.y + l.z * r.z;
}


bool mat3x3_solve(const Matrix3x3& A, const Vector3& b, Vector3* pOut)
{
	// Transpose from column major to row major and augment in b
	float omat[3][4] = {
		{A.a.x, A.b.x, A.c.x, b.x},
		{A.a.y, A.b.y, A.c.y, b.y},
		{A.a.z, A.b.z, A.c.z, b.z},
	};

	// Keep the matrix as pointers so we can swap rows quickly without damaging our augment in omat
	float* mat[3] = {
		&omat[0][0],
		&omat[1][0],
		&omat[2][0],
	};

	// Get it into row echelon form
	for (int n = 0; n < 2; n++)
	{
		// Find pivots
		float largest = 0;
		float la = 0;
		int pivrow = -1;
		for (int m = n; m < 3; m++)
		{
			float v = mat[m][n];
			float va = fabsf(v);

			if (va > la)
			{
				pivrow = m;
				largest = v;
				la = va;
			}
		}

		// No pivot? No solution!
		if (pivrow == -1)
			return false;

		// Swap pivot to highest
		float* pivot = mat[pivrow];
		mat[pivrow] = mat[n];
		mat[n] = pivot;

		Vector4* pivotv = reinterpret_cast<Vector4*>(pivot);

		// Apply our pivot row to the rows below 
		for (int m = n + 1; m < 3; m++)
		{
			// Get the multiplier
			float* row = mat[m];
			float v = -row[n] / pivot[n];

			Vector4* rowv = reinterpret_cast<Vector4*>(row);
			*rowv += *pivotv * v;
		}
	}

	// Get it into reduced row echelon form
	for (int n = 2; n; n--)
	{
		for (int m = n - 1; m >= 0; m--)
		{
			float* pivot = mat[n];
			Vector4* pivotv = reinterpret_cast<Vector4*>(pivot);

			// Get the multiplier
			float* row = mat[m];
			float v = -row[n] / pivot[n];

			// Push that pivot up
			Vector4* rowv = reinterpret_cast<Vector4*>(row);
			*rowv += *pivotv * v;
		}
	}

	// Clean up our diagonal
	for (int n = 0; n < 3; n++)
	{
		float* rowf = mat[n];
		float v = rowf[n];

		// Check for zeros along the diagonal
		if (fabsf(v) <= FLT_EPSILON)
			return false;

		Vector4* row = reinterpret_cast<Vector4*>(rowf);
		*row /= v;
	}

	// Hoist the augment back off mat
	Vector3 v = { mat[0][3], mat[1][3], mat[2][3] };
	*pOut = v;
	return true;
}

bool PlaneIntersect(const Plane& plane1, const Plane& plane2, const Plane& plane3, Vector3* pOut)
{
	Vector3 n1 = plane1.normal;
	Vector3 n2 = plane2.normal;
	Vector3 n3 = plane3.normal;

	// Push all norms into the rows, transposed
	Matrix3x3 n = {
		{n1.x, n2.x, n3.x},
		{n1.y, n2.y, n3.y},
		{n1.z, n2.z, n3.z},
	};

	// The dist is the augment
	Vector3 d = { plane1.dist, plane2.dist, plane3.dist };

	// Solve x for n*x=d
	Vector3 i;
	if (mat3x3_solve(n, d, &i))
	{
		*pOut = i;
		return true;
	}

	return false;
}

Vector3 ParseVector(const std::string& str)
{
	// Parse the vector 
	Vector3 v;
	sscanf_s(str.c_str(), "%f %f %f", &v.x, &v.y, &v.z);
	return v;
}

Plane ParsePlane(const std::string& str, Vector3 origin)
{
	// Parse the plane
	Plane plane;
	sscanf_s(str.c_str(), "%f %f %f %f", &plane.normal.x, &plane.normal.y, &plane.normal.z, &plane.dist);

#if DEBUG_LOG
	//debug
	Vector3 vecPlanePos = plane.normal * plane.dist;
	std::cout << "script_client DebugDrawLine(Vector(" << origin.x << ", " << origin.y << ", " << origin.z << "), Vector(" << origin.x + vecPlanePos.x << ", " << origin.y + vecPlanePos.y << ", " << origin.z + vecPlanePos.z << "), 255, 255, 255, false, 60); \n";
#endif

	return plane;
}

constexpr float k_flEpsilon = 0.001f;

void ParseFile(std::ifstream& ReadFile, std::vector<Entity>& entities)
{
	std::string textLine;
	Entity newEntity;
	int iSkipBB = 0;
	int iLastBrush = 0;
	while (getline(ReadFile, textLine))
	{
		// Skip any blank lines
		if (textLine.size() == 0)
			continue;

		if (textLine[0] == '{')
		{
			// Start of entity
			// Clear out the entity
			newEntity = {};
			continue;
		}

		if (textLine[0] == '}')
		{
			// End of entity
			// Commit the entity
			entities.push_back(newEntity);
			continue;
		}

		// Everything here has a key and a value. Both the key and the value are surrounded by double quotes
		// Find everything between the pairs of double quotes
		// The + 1 is to move us over the double quote
		size_t keyStart = textLine.find('"');
		size_t keyEnd = textLine.find('"', keyStart + 1);
		size_t valueStart = textLine.find('"', keyEnd + 1);
		size_t valueEnd = textLine.find('"', valueStart + 1);

		// Move the string start over the double quote
		keyStart++;
		valueStart++;

		std::string key = textLine.substr(keyStart, keyEnd - keyStart);
		std::string value = textLine.substr(valueStart, valueEnd - valueStart);

		if (key == "editorclass")
		{
			newEntity.editorclass = value;
		}
		else if (key == "origin")
		{
			newEntity.origin = ParseVector(value);
		}
		else if (key == "targetname")
		{
			newEntity.targetname = value;
		}
		else if (key == "script_flag")
		{
			newEntity.scriptflag = value;
		}
		else if (key == "spawnclass")
		{
			newEntity.spawnclass = value;
		}
		else if (key == "classname")
		{
			newEntity.classname = value;
			iSkipBB = 0;
			iLastBrush = 0;
		}
		else if (key.find("*trigger_brush_") != std::string::npos)
		{
			newEntity.isTrigger = true;
			//std::cout << "string " << key << "\n";
			const std::string token1 = "*trigger_brush_";
			size_t brushDigitsPos = key.find("_", token1.length());//some triggers have 10+ brushes
			size_t brushNumDigits = brushDigitsPos - token1.length();
			int iBrush = stoi(key.substr(token1.length(), brushNumDigits));
			//std::cout << "brush " << brush << ", ";

			if (iLastBrush != iBrush)
			{
				iSkipBB = 0;
				iLastBrush = iBrush;
			}
			const std::string token2 = "_plane_";
			int iPlane = stoi(key.substr(token1.length() + brushNumDigits + token2.length()));
			//std::cout << "iPlane " << iPlane << ", ";

			// Parse the plane
			Plane plane = ParsePlane(value, newEntity.origin);

			if (iSkipBB <= 5)//0-5 are bounding box of the brush
			{
				//std::cout << "Found BB plane " << iSkipBB << "\n";
				iSkipBB++;
				plane.bbox = true;
			}

			// Normally, I'd just use pushback, but these have IDs soooo idk

			// Make room for the brush if we haven't yet
			if (newEntity.brushes.size() <= iBrush)
				newEntity.brushes.resize(iBrush + 1);

			// Grab the brush
			Brush& brush = newEntity.brushes[iBrush];

			// Make room for the plane if we haven't yet
			if (brush.planes.size() <= iPlane)
				brush.planes.resize(iPlane + 1);

			//don't add plane if clone exists
			int nPlanes = brush.planes.size();
			for (int iPlane2 = 0; iPlane2 < nPlanes; iPlane2++)
			{
				Plane& plane2 = brush.planes[iPlane2];
				float f = dotProduct(plane.normal, plane2.normal);
				//std::cout << "Plane " << iPlane << " and " << iPlane2 << " dot product is " << f << "\n";

				if (f == 1)
				{
					//std::cout << "Clone Plane: " << iPlane << " and " << iPlane2 << "\n";
					plane.skip = true;
				}
			}

			// Set the plane
			brush.planes[iPlane] = plane;
#if DEBUG_LOG
			std::cout << "Add plane " << iPlane << "\n";
#endif
		}
		else if (key == "*trigger_bounds_mins")
		{
			newEntity.mins = ParseVector(value);
		}
		else if (key == "*trigger_bounds_maxs")
		{
			newEntity.maxs = ParseVector(value);
		}
	}
}


bool IsNear( float a, float b, float eps = k_flEpsilon )
{
	float c = a - b;
	return -eps < c && c < eps;
}


bool ShouldSkipPlane( const Plane& plane1, const Plane& plane2 )
{
	// Are 1 and 2 parallel or opposing? If so, there will never be a intersection
	float f = dotProduct( plane1.normal, plane2.normal );
	if ( IsNear( fabs( f ), 1.0 ) )
	{
#if DEBUG_LOG
		std::cout << "Intersection between planes " << iPlane1 << ", " << iPlane2 << " skipped because parallel\n";
#endif
		return true;
	}
	return false;
}

bool TestPointInBrush( const Brush& brush, const Vector3& p )
{
	// Test if the point is past any planes on the brush
	for ( const Plane &k : brush.planes )
	{
		if ( k.skip )
			continue;

		// Get the dist of the point for the plane normal
		if ( dotProduct( p, k.normal ) - k.dist > k_flEpsilon )
			return false;
	}

	// Passed all plane tests!
	return true;
}


// Each edge pair corresponds to two planes
constexpr uint32_t k_iInvalidVertex = 0xFFFFFFFF;
struct EdgePair
{
	uint32_t iVertex1;
	uint32_t iVertex2;
};

// Util class for building brush edge lists
class BrushBuilder
{
public:
	BrushBuilder();
	~BrushBuilder();


	void Build( Brush& brush );

private:

	void BeginBrush( int nPlanes );

	// Returns the edge shared by two planes, x and y
	EdgePair& GetEdge( int x, int y );

	// Adds a vertex onto a given edge shared by two planes, x and y 
	// Returns true when the given edge now has two pairs
	bool PushPartialEdge( int x, int y, uint32_t iVert );

	// Stores a vertex and returns its index or returns the index of the vertex if it's already stored
	uint32_t StoreVertex( const Vector3& newVert );

	int m_nPlaneCount;

	// This is a triangular array of all potential edge pairings between planes
	int m_nEdgeCount;
	int m_nEdgeCapacity;
	EdgePair* m_pEdgePairs;

	// This is a set of all vertices with no duplicates
	std::vector<Vector3> m_vecVerts;
};


BrushBuilder::BrushBuilder()
{
	m_nPlaneCount = 0;
	m_nEdgeCount = 0;
	m_nEdgeCapacity = 0;
	m_pEdgePairs = nullptr;

	// Setup with basic starter data
	BeginBrush( 6 );
}

BrushBuilder::~BrushBuilder()
{
	delete[] m_pEdgePairs;
}

void BrushBuilder::BeginBrush( int nPlanes )
{
	// Each plane can share an edge with another plane.
	// If each plane were to interact with every other plane (including itself), that'd be n^2 edges
	// We can optimize this down, as an edge between plane1 and plane2 is the same as an edge between plane2 and plane1
	// We also don't care about interactions between plane1 and plane1, as the a plane cannot intersect with itself
	// This is just the formula to find the nth triangular number, but we're subtracting 1 off nPlanes
	int nPotentialEdges = ( nPlanes - 1 ) * nPlanes / 2;

	// If we need more space, then resize our pairs
	if ( nPotentialEdges > m_nEdgeCapacity )
	{
		delete[] m_pEdgePairs;
		m_pEdgePairs = new EdgePair[nPotentialEdges];
		m_nEdgeCapacity = nPotentialEdges;
	}

	// Store the new values
	m_nPlaneCount = nPlanes;
	m_nEdgeCount = nPotentialEdges;

	// Clear out the old data
	memset( m_pEdgePairs, 0xFF, nPotentialEdges * sizeof( EdgePair ) );
	m_vecVerts.clear();
}

EdgePair& BrushBuilder::GetEdge( int x, int y )
{
	// Ensure the lowest is always plane y
	if ( x < y )
		std::swap( x, y );
	
	// Get the index within our triangular array
	// I thought indexing into this would be easier, but oh well!

	// Sum of an arithmetic series to get the row Y
	int a1  = m_nPlaneCount - 1;
	int an  = m_nPlaneCount - y;
	int n   = y;
	int sum = y * ( a1 + an ) / 2;

	// Get the index for column x
	int idx = sum + (x - y - 1);

	// return the edge!
	return m_pEdgePairs[idx];
}

bool BrushBuilder::PushPartialEdge( int x, int y, uint32_t iVertex )
{
	EdgePair& edge = GetEdge( x, y );


	// If it's already on the edge, ignore the request
	if ( edge.iVertex1 == iVertex || edge.iVertex2 == iVertex )
	{
		// We need to still return true if the edge is full!
		// One of the two verts are already valid, we always fill iVertex1 first, so iVertex2 is either valid or invalid
		return edge.iVertex2 == k_iInvalidVertex ? false : true;
	}

	// Fill iVertex1 first
	if ( edge.iVertex1 == k_iInvalidVertex )
	{
		edge.iVertex1 = iVertex;
		return false;
	}
	
	// Fill iVertex2 last
	if ( edge.iVertex2 == k_iInvalidVertex )
	{
		edge.iVertex2 = iVertex;
		return true;
	}

	// Uh oh, pushing a third vert?
	const Vector3& v1 = m_vecVerts[edge.iVertex1];
	const Vector3& v2 = m_vecVerts[edge.iVertex2];
	const Vector3& v3 = m_vecVerts[iVertex];
	/*
	std::cout << "Hey!! The edge (" << x << "," << y << ") already has two verts!\n";
	std::cout << "\t Vertex 1 : " << v1.x << ", " << v1.y << ", " << v1.z << "\n";
	std::cout << "\t Vertex 2 : " << v2.x << ", " << v2.y << ", " << v2.z << "\n";
	std::cout << "\t Vertex 3 : " << v3.x << ", " << v3.y << ", " << v3.z << "\n";
	*/
	return true;
}


uint32_t BrushBuilder::StoreVertex( const Vector3& newVert )
{
	// Ideally, we'd just be using std::set or std::unordered_set, but both are being a pain to use..

	// Find a vert with the value close enough to our newVert
	int n = m_vecVerts.size();
	for ( int i = 0; i < n; i++ )
	{
		const Vector3& curVert = m_vecVerts[i];
		if ( IsNear( curVert.x, newVert.x ) && IsNear( curVert.y, newVert.y ) && IsNear( curVert.z, newVert.z ) )
			return i;
	}

	// No duplicate found! We'll need to store it then
	int idx = m_vecVerts.size();
	m_vecVerts.push_back( newVert );
	return idx;
}


void BrushBuilder::Build( Brush& brush )
{
	int nPlanes = brush.planes.size();

	// We need at least 4 plans for a brush
	if (nPlanes < 4)
	{
		std::cout << "Less than 4 planes!\n";
		return;
	}

	// Good brush! We can begin!
	BeginBrush( nPlanes );

	// Note: If issues come up with points not combining up properly, then it
	//       might be worth while to recenter everything for floating point accuracy


	// Get all plane intersections
	for (int iPlane1 = 0; iPlane1 < nPlanes - 2; iPlane1++)
	{
		Plane& plane1 = brush.planes[iPlane1];
		if (plane1.skip)
			continue;
		
		for (int iPlane2 = iPlane1 + 1; iPlane2 < nPlanes - 1; iPlane2++)
		{
			Plane& plane2 = brush.planes[iPlane2];
			if (plane2.skip)
				continue;

			// Is it parallel or opposing?
			if ( ShouldSkipPlane( plane1, plane2 ) )
				continue;

			// Loop for the other remaining untested planes
			for (int iPlane3 = iPlane2 + 1; iPlane3 < nPlanes; iPlane3++)
			{
				Plane& plane3 = brush.planes[iPlane3];
				if (plane3.skip)
					continue;

				// Is it parallel or opposing?
				if ( ShouldSkipPlane( plane3, plane1 ) )
					continue;
				if ( ShouldSkipPlane( plane3, plane2 ) )
					continue;

				// Do our 3 planes intersect? If not, next plane
				Vector3 p;
				if (!PlaneIntersect(plane1, plane2, plane3, &p))
				{
#if DEBUG_LOG
					std::cout << "Intersection between planes " << iPlane1 << ", " << iPlane2 << ", " << iPlane3 << " skipped because they don't intersect\n";
#endif
					continue;
				}

				// Cull points outside of the solid
				if ( !TestPointInBrush( brush, p ) )
					continue;

#if DEBUG_LOG
				std::cout << "Intersection between planes " << iPlane1 << ", " << iPlane2 << ", " << iPlane3 << " found\n";
#endif

				// Good intersection! Store the point and push the three partial edges
				uint32_t iVert = StoreVertex( p );

				PushPartialEdge( iPlane3, iPlane1, iVert );
				PushPartialEdge( iPlane3, iPlane2, iVert );

				if ( PushPartialEdge( iPlane2, iPlane1, iVert ) )
				{
					// Our pair is full! No need to compute more intersections for iPlane1 and iPlane2
#if DEBUG_LOG
					std::cout << "Intersection between planes " << iPlane1 << ", " << iPlane2 << " completed edge\n";
#endif
					break;
				}
			}
		}
	}


	// Emit edges
	int iEdge = 0;
	for ( int iPlane1 = 0; iPlane1 < nPlanes - 1; iPlane1++ )
	{
		for ( int iPlane2 = iPlane1 + 1; iPlane2 < nPlanes; iPlane2++ )
		{
			EdgePair& edge = m_pEdgePairs[iEdge++];

			// Skip invalid edges
			if ( edge.iVertex1 == k_iInvalidVertex || edge.iVertex2 == k_iInvalidVertex )
				continue;

			// Emit!
			brush.edges.push_back( { m_vecVerts[edge.iVertex1], m_vecVerts[edge.iVertex2] } );
		}
	}
}
int ReadSettings(std::ifstream& ReadFile, Settings& settings)
{
	std::string textLine;
	bool inComment = false;
	while (getline(ReadFile, textLine))
	{
		//std::cout << textLine << "\n";
		if (textLine.size() == 0)
			continue;

		//comments
		if (textLine[0] == '/' && textLine[1] == '/')
			continue;
		else if (textLine[0] == '/' && textLine[1] == '*')//decent but not as good as c++
			inComment = true;
		else if (textLine[0] == '*' && textLine[1] == '/')
			inComment = false;
		if (inComment)
			continue;

		size_t keyStart = textLine.find('"');
		size_t keyEnd = textLine.find('"', keyStart + 1);
		size_t valueStart = textLine.find('"', keyEnd + 1);
		size_t valueEnd = textLine.find('"', valueStart + 1);

		keyStart++;
		valueStart++;

		std::string key = textLine.substr(keyStart, keyEnd - keyStart);
		std::string value = textLine.substr(valueStart, valueEnd - valueStart);
		//std::cout << "key " << key << " value " << value << "\n";
		if (key == "default")
		{
			if (!strcmp(value.c_str(), "allow"))
				settings.defaultAllow = true;
			else if (!strcmp(value.c_str(), "disallow"))
				settings.defaultAllow = false;
			else
			{
				std::cout << "Unknown setting for " << key << ". Should be either 'allow' or 'disallow'.\n";
				return 0;
			}
		}
		else if (key == "drawontop")
		{
			if (!strcmp(value.c_str(), "yes"))
				settings.drawontop = true;
			else if (!strcmp(value.c_str(), "no"))
				settings.drawontop = false;
			else
			{
				std::cout << "Unknown setting for " << key << ". Should be either 'yes' or 'no'.\n";
				return 0;
			}
		}
		else if (key == "drawtriggeroutlines")
		{
			if (!strcmp(value.c_str(), "yes"))
				settings.drawTriggerOutlines = true;
			else if (!strcmp(value.c_str(), "no"))
				settings.drawTriggerOutlines = false;
			else
			{
				std::cout << "Unknown setting for " << key << ". Should be either 'yes' or 'no'.\n";
				return 0;
			}
		}
		else if (key == "drawentcubes")
		{
			if (!strcmp(value.c_str(), "yes"))
				settings.drawEntCubes = true;
			else if (!strcmp(value.c_str(), "no"))
				settings.drawEntCubes = false;
			else
			{
				std::cout << "Unknown setting for " << key << ". Should be either 'yes' or 'no'.\n";
				return 0;
			}
		}
		else if (key == "duration")
		{
			settings.duration = stoi(value);
		}
		else if (key == "allow" && !settings.defaultAllow)
		{
			settings.allows.push_back(value);
		}
		else if (key == "disallow" && settings.defaultAllow)
		{
			settings.disallows.push_back(value);
		}
		else if (key == "must")
		{
			settings.musts.push_back(value);
		}
		else if (key == "avoid")
		{
			settings.avoids.push_back(value);
		}
	}
	return 1;
}

bool StringMatch(std::string strEnt, std::string strSetting)
{
	if (strEnt == strSetting)
		return true;
	const char* caEnt = strEnt.c_str();
	const char* caSetting = strSetting.c_str();
	while (*caEnt && *caSetting)
	{
		unsigned char cEnt = *caEnt;
		unsigned char cSetting = *caSetting;
		if (cEnt != cSetting)
			break;
		caEnt++;
		caSetting++;
	}
	if (*caSetting == 0 && *caEnt == 0)
		return true;
	if (*caSetting == '*' && !strEnt.empty())
		return true;
	return false;
}

bool CriteriaMet(std::string& line, Entity& ent)
{
	size_t keyStart = line.find('"');
	size_t keyEnd = line.find(' ', keyStart + 1);
	size_t valueEnd = line.find('"', keyEnd + 1);

	keyStart++;
	std::string key = line.substr(keyStart, keyEnd - keyStart);
	keyEnd++;
	std::string value = line.substr(keyEnd, valueEnd - keyEnd);

	if (key == "classname")
		return StringMatch(ent.classname, value);
	else if (key == "editorclass")
		return StringMatch(ent.editorclass, value);
	else if (key == "script_flag")
		return StringMatch(ent.scriptflag, value);
	else if (key == "spawnclass")
		return StringMatch(ent.spawnclass, value);
	else if (key == "targetname")
		return StringMatch(ent.targetname, value);
	else if (key == "_istrigger")
		return ent.isTrigger;
	else
		std::cout << "Did not recognize property named " << key << ".\n";
	return false;
}

int BaseColorOffCoord(float coord)
{
	return (((abs((int)coord % 64) * 4) + 128) / 1.5);
}

int main(int argc, char* argv[])
{
	bool debug = argc == 1;
	std::vector<Entity> entities;
	Settings settings;

	//settings
	std::cout << "Got file(s). Please drag settings file onto window and press ENTER, or just press ENTER to go without one.\n";
	std::string settingspath;
	std::getline(std::cin, settingspath);
	std::ifstream ReadSettingsFile(settingspath);
	bool n = ReadSettings(ReadSettingsFile, settings);
	ReadSettingsFile.close();

	for (int i = 1; debug || i < argc; i++)
	{
		debug = false;
		entities.clear();
		//read entity data
		std::string path = argc == 1 ? "filename.txt" : argv[i];
		std::ifstream ReadFile(path);
		std::string base_filename = path.substr(path.find_last_of("/\\") + 1);
		std::string::size_type const p(base_filename.find_last_of('.'));
		std::string file_without_extension = base_filename.substr(0, p);
		ParseFile(ReadFile, entities);
		ReadFile.close();

		BrushBuilder bb;

		//get line from two intersecting planes
		//every plane in a brush must be checked against all others in the brush
		for (Entity& ent : entities)
			for (Brush& brush : ent.brushes)
				bb.Build(brush);

		std::ofstream writingFile;
		writingFile.open(file_without_extension + ".cfg");
		std::cout << "Starting writing to " << file_without_extension << ".cfg\n";
		writingFile << "sv_cheats 1;enable_debug_overlays 1;\n";
		//write drawlines
		for (Entity& ent : entities)
		{
			//filtering
			bool allowed = false;
			bool disallowed = false;

			//allow for blacklist
			if (!settings.defaultAllow)
			{
				for (std::string& line : settings.allows)
				{
					allowed = CriteriaMet(line, ent);
					if (allowed)
						break;//found something that allows us, even one thing
				}
			}

			//re-dis-allow for blacklist
			//or
			//disallow for whitelist
			for (std::string& line : settings.disallows)
			{
				disallowed = CriteriaMet(line, ent);
				if (disallowed)
					break;
			}

			//re-allow for whitelist
			if (settings.defaultAllow)
			{
				for (std::string& line : settings.allows)
				{
					allowed = CriteriaMet(line, ent);
					if (allowed)
						break;
				}
			}
			//std::cout << ent.classname << " defaultAllow " << (settings.defaultAllow ? "true" : "false") << ", disallowed " << (disallowed ? "true" : "false") << ", allowed " << (allowed ? "true" : "false") << "\n";
			if (settings.defaultAllow)
			{
				if (disallowed && !allowed)
					continue;//got disallowed, no re-allow
			}
			else
			{
				if (!allowed)
					continue;//never was allowed
			}

			bool goodMusts = true;
			for (std::string& line : settings.musts)
			{
				goodMusts = CriteriaMet(line, ent);
				if (!goodMusts)
					break;
			}

			if (!goodMusts)
				continue;

			bool badAvoids = false;
			for (std::string& line : settings.avoids)
			{
				badAvoids = CriteriaMet(line, ent);
				if (badAvoids)
					break;
			}

			if (badAvoids)
				continue;
			int color[3];
			color[0] = BaseColorOffCoord(ent.origin.x);
			color[1] = BaseColorOffCoord(ent.origin.y);
			color[2] = BaseColorOffCoord(ent.origin.z);
			if (!ent.spawnclass.empty()) writingFile << "//Spawn Class: " << ent.spawnclass << "\n";
			if (!ent.editorclass.empty()) writingFile << "//Editor Class: " << ent.editorclass << "\n";
			if (!ent.classname.empty()) writingFile << "//Class Name: " << ent.classname << "\n";
			if (!ent.targetname.empty()) writingFile << "//Target Name: " << ent.targetname << "\n";
			if (!ent.scriptflag.empty()) writingFile << "//Script Flag: " << ent.scriptflag << "\n";
			if (ent.isTrigger && settings.drawTriggerOutlines)
			{
				for (Brush& brush : ent.brushes)
				{
					//std::cout << "\n";
					for (Edge& edge : brush.edges)
					{
						Vector3 stem = ent.origin + edge.stem;
						Vector3 tail = ent.origin + edge.tail;
#if 1

						writingFile << "script_client DebugDrawLine("
							<< "Vector(" << stem.x << ", " << stem.y << ", " << stem.z << "), "
							<< "Vector(" << tail.x << ", " << tail.y << ", " << tail.z << "), "
							<< color[0] << ", "
							<< color[1] << ", "
							<< color[2] << ", "
							<< (!settings.drawontop ? "true" : "false") << ", "
							<< settings.duration << ");\n";
#else
						// Desmos 3D lol
						std::cout << "["
							<< "(" << stem.x << ", " << stem.y << ", " << stem.z << "), "
							<< "(" << tail.x << ", " << tail.y << ", " << tail.z << ")"
							<< "]\n";
#endif
					}
				}
			}
			if (settings.drawEntCubes)
			{
				writingFile << "script_client DebugDrawCube("
					<< "Vector(" << ent.origin.x << ", " << ent.origin.y << ", " << ent.origin.z << "), "
					<< "16, "
					<< color[0] << ", "
					<< color[1] << ", "
					<< color[2] << ", "
					<< (!settings.drawontop ? "true" : "false") << ", "
					<< settings.duration << ");\n";
			}
		}
		std::cout << "Finished writing to " << file_without_extension << ".cfg\n";
		writingFile.close();
	}
	std::cout << "Done. Press ENTER or the X button to close.\n";
	std::cin.get();
}