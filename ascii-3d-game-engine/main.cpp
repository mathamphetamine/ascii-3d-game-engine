#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <Windows.h>

using namespace std;

// Console Screen Size
const int nScreenWidth = 120;   // Console Screen Width (columns)
const int nScreenHeight = 40;   // Console Screen Height (rows)

// World Dimensions
const int nMapWidth = 16;
const int nMapHeight = 16;

// Player's Start Position and Rotation
float fPlayerX = 14.7f;
float fPlayerY = 5.09f;
float fPlayerA = 0.0f; // Player Start Rotation

// Field of View and render distance
const float fFOV = 3.14159f / 4.0f; // Field of View
const float fDepth = 16.0f;         // Maximum render distance
const float fSpeed = 5.0f;          // Player Speed

int main() {
    // Create Screen Buffer
    wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

    // Define the Map: '#' = wall block, '.' = space
    wstring map;
    map += L"#########.......";
    map += L"#...............";
    map += L"#.......########";
    map += L"#..............#";
    map += L"#......##......#";
    map += L"#......##......#";
    map += L"#..............#";
    map += L"###............#";
    map += L"##.............#";
    map += L"#......####..###";
    map += L"#......#.......#";
    map += L"#......#.......#";
    map += L"#..............#";
    map += L"#......#########";
    map += L"#..............#";
    map += L"################";

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    while (true) {
        // Time differential per frame for consistent movement
        tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();

        // Handle player rotation (left: 'A', right: 'D')
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
            fPlayerA -= (fSpeed * 0.75f) * fElapsedTime;

        if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
            fPlayerA += (fSpeed * 0.75f) * fElapsedTime;

        // Handle player movement (forward: 'W', backward: 'S')
        if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
            fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
            fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
            if (map[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#') {
                fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
            }
        }

        if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
            fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
            if (map[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#') {
                fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
            }
        }

        // Render the scene
        for (int x = 0; x < nScreenWidth; x++) {
            // Calculate the ray angle for each column
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

            // Incrementally cast the ray from the player's position
            float fStepSize = 0.1f;
            float fDistanceToWall = 0.0f;

            bool bHitWall = false;
            bool bBoundary = false;

            float fEyeX = sinf(fRayAngle);
            float fEyeY = cosf(fRayAngle);

            while (!bHitWall && fDistanceToWall < fDepth) {
                fDistanceToWall += fStepSize;
                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                // Check if ray is out of bounds
                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight) {
                    bHitWall = true;
                    fDistanceToWall = fDepth;
                } else {
                    // Check if the ray has hit a wall block
                    if (map[nTestX * nMapWidth + nTestY] == '#') {
                        bHitWall = true;

                        // Check for boundary (intersection between blocks)
                        vector<pair<float, float>> p;
                        for (int tx = 0; tx < 2; tx++)
                            for (int ty = 0; ty < 2; ty++) {
                                float vy = (float)nTestY + ty - fPlayerY;
                                float vx = (float)nTestX + tx - fPlayerX;
                                float d = sqrt(vx * vx + vy * vy);
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
                                p.push_back(make_pair(d, dot));
                            }
                        sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) { return left.first < right.first; });

                        float fBound = 0.01;
                        if (acos(p.at(0).second) < fBound) bBoundary = true;
                        if (acos(p.at(1).second) < fBound) bBoundary = true;
                        if (acos(p.at(2).second) < fBound) bBoundary = true;
                    }
                }
            }

            // Calculate the distance to the ceiling and the floor
            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
            int nFloor = nScreenHeight - nCeiling;

            // Choose the shade character based on the distance
            short nShade = ' ';
            if (fDistanceToWall <= fDepth / 4.0f) nShade = 0x2588;
            else if (fDistanceToWall < fDepth / 3.0f) nShade = 0x2593;
            else if (fDistanceToWall < fDepth / 2.0f) nShade = 0x2592;
            else if (fDistanceToWall < fDepth) nShade = 0x2591;
            else nShade = ' ';

            if (bBoundary) nShade = ' ';

            // Draw the column on the screen
            for (int y = 0; y < nScreenHeight; y++) {
                if (y <= nCeiling) screen[y * nScreenWidth + x] = ' ';
                else if (y > nCeiling && y <= nFloor) screen[y * nScreenWidth + x] = nShade;
                else {
                    // Shade the floor based on distance
                    float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                    if (b < 0.25) nShade = '#';
                    else if (b < 0.5) nShade = 'x';
                    else if (b < 0.75) nShade = '.';
                    else if (b < 0.9) nShade = '-';
                    else nShade = ' ';
                    screen[y * nScreenWidth + x] = nShade;
                }
            }
        }

        // Display frame rate and player coordinates
        swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

        // Display the map
        for (int nx = 0; nx < nMapWidth; nx++)
            for (int ny = 0; ny < nMapHeight; ny++)
                screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
        screen[((int)fPlayerX + 1) * nScreenWidth + (int)fPlayerY] = 'P';

		// Display Frame
		screen[nScreenWidth * nScreenHeight - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}

	return 0;
}
