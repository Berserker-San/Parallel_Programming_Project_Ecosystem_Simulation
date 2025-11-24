#include <iostream>
#include <vector>
#include <string>
#include <omp.h>

using namespace std;

enum
{
    EMPTY = 0,
    ROCK = 1,
    RABBIT = 2,
    FOX = 3
};

int genProcRabbits, genProcFoxes, genFoodFoxes;
int nGen, R, C, N;
int gridSize;

// mundo actual
vector<int> tipo, repro, hambre;
// después de conejos
vector<int> tipoR, reproR, hambreR;
// después de zorros
vector<int> tipoNext, reproNext, hambreNext;

// auxiliares conejos
vector<char> moveR, canReproR, aliveR;
vector<int> destR, age0R, nextAgeR;
// auxiliares zorros
vector<char> moveF, canReproF, aliveF;
vector<int> destF, age0F, hungry0, hunger1, nextAgeF;
// conflictos
vector<int> bestAge, bestHunger, winner;

const int dx[4] = {-1, 0, 1, 0};
const int dy[4] = {0, 1, 0, -1};

int idxPos(int x, int y) { return x * C + y; }

void leerEntrada()
{
    if (!(cin >> genProcRabbits >> genProcFoxes >> genFoodFoxes >> nGen >> R >> C >> N))
    {
        exit(0);
    }
    gridSize = R * C;

    tipo.assign(gridSize, EMPTY);
    repro.assign(gridSize, 0);
    hambre.assign(gridSize, 0);

    for (int i = 0; i < N; ++i)
    {
        string obj;
        int x, y;
        cin >> obj >> x >> y;
        int id = idxPos(x, y);
        if (obj == "ROCK")
            tipo[id] = ROCK;
        if (obj == "RABBIT")
            tipo[id] = RABBIT;
        if (obj == "FOX")
            tipo[id] = FOX;
    }

    tipoR.resize(gridSize);
    reproR.resize(gridSize);
    hambreR.resize(gridSize);

    tipoNext.resize(gridSize);
    reproNext.resize(gridSize);
    hambreNext.resize(gridSize);

    moveR.assign(gridSize, 0);
    canReproR.assign(gridSize, 0);
    aliveR.assign(gridSize, 0);
    destR.assign(gridSize, -1);
    age0R.assign(gridSize, 0);
    nextAgeR.assign(gridSize, 0);

    moveF.assign(gridSize, 0);
    canReproF.assign(gridSize, 0);
    aliveF.assign(gridSize, 0);
    destF.assign(gridSize, -1);
    age0F.assign(gridSize, 0);
    hungry0.assign(gridSize, 0);
    hunger1.assign(gridSize, 0);
    nextAgeF.assign(gridSize, 0);

    bestAge.assign(gridSize, -1);
    bestHunger.assign(gridSize, 0);
    winner.assign(gridSize, -1);
}

void faseConejos(int gen)
{
#pragma omp parallel for
    for (int i = 0; i < gridSize; ++i)
    {
        moveR[i] = 0;
        canReproR[i] = 0;
        aliveR[i] = 0;
        destR[i] = -1;
        age0R[i] = 0;
        tipoR[i] = EMPTY;
        reproR[i] = 0;
        hambreR[i] = 0;
    }

#pragma omp parallel for
    for (int id = 0; id < gridSize; ++id)
    {
        if (tipo[id] != RABBIT)
            continue;
        int x = id / C, y = id % C;

        age0R[id] = repro[id];
        canReproR[id] = (age0R[id] >= genProcRabbits);

        int cand[4], pc = 0;
        for (int d = 0; d < 4; ++d)
        {
            int nx = x + dx[d], ny = y + dy[d];
            if (nx < 0 || nx >= R || ny < 0 || ny >= C)
                continue;
            int nid = idxPos(nx, ny);
            if (tipo[nid] == EMPTY)
                cand[pc++] = nid;
        }
        if (pc > 0)
        {
            int ch = (gen + x + y) % pc;
            destR[id] = cand[ch];
            moveR[id] = 1;
        }
    }

#pragma omp parallel for
    for (int i = 0; i < gridSize; ++i)
    {
        if (tipo[i] == ROCK || tipo[i] == FOX)
        {
            tipoR[i] = tipo[i];
            reproR[i] = repro[i];
            hambreR[i] = hambre[i];
        }
        bestAge[i] = -1;
        winner[i] = -1;
    }

    for (int id = 0; id < gridSize; ++id)
    {
        if (tipo[id] != RABBIT || !moveR[id])
            continue;
        int d = destR[id];
        int a = age0R[id];
        if (bestAge[d] == -1 || a > bestAge[d])
        {
            int prev = winner[d];
            if (prev >= 0)
                aliveR[prev] = 0;
            bestAge[d] = a;
            winner[d] = id;
            aliveR[id] = 1;
            tipoR[d] = RABBIT;
            reproR[d] = age0R[id];
            hambreR[d] = 0;
        }
    }

    for (int id = 0; id < gridSize; ++id)
    {
        if (tipo[id] != RABBIT)
            continue;
        if (!moveR[id])
        {
            aliveR[id] = 1;
            if (tipoR[id] == EMPTY)
            {
                tipoR[id] = RABBIT;
                reproR[id] = age0R[id];
                hambreR[id] = 0;
            }
        }
    }

    for (int id = 0; id < gridSize; ++id)
    {
        if (tipo[id] != RABBIT)
            continue;
        if (!moveR[id] || !aliveR[id] || !canReproR[id])
            continue;
        if (tipoR[id] == EMPTY)
        {
            tipoR[id] = RABBIT;
            reproR[id] = 0;
            hambreR[id] = 0;
        }
    }

    nextAgeR.assign(gridSize, 0);

    for (int id = 0; id < gridSize; ++id)
    {
        if (tipo[id] != RABBIT || !aliveR[id])
            continue;
        int pos = moveR[id] ? destR[id] : id;
        nextAgeR[pos] = canReproR[id] ? 0 : age0R[id] + 1;
    }
    for (int id = 0; id < gridSize; ++id)
    {
        if (tipo[id] != RABBIT || !moveR[id] || !aliveR[id] || !canReproR[id])
            continue;
        nextAgeR[id] = 0;
    }

    for (int i = 0; i < gridSize; ++i)
    {
        if (tipoR[i] == RABBIT)
            reproR[i] = nextAgeR[i];
    }
}

void faseZorros(int gen)
{
#pragma omp parallel for
    for (int i = 0; i < gridSize; ++i)
    {
        moveF[i] = 0;
        canReproF[i] = 0;
        aliveF[i] = 0;
        destF[i] = -1;
        age0F[i] = 0;
        hungry0[i] = 0;
        hunger1[i] = 0;
        tipoNext[i] = EMPTY;
        reproNext[i] = 0;
        hambreNext[i] = 0;
    }

#pragma omp parallel for
    for (int i = 0; i < gridSize; ++i)
    {
        if (tipoR[i] == ROCK || tipoR[i] == RABBIT)
        {
            tipoNext[i] = tipoR[i];
            reproNext[i] = reproR[i];
            hambreNext[i] = hambreR[i];
        }
    }

    for (int id = 0; id < gridSize; ++id)
    {
        if (tipoR[id] != FOX)
            continue;
        int x = id / C, y = id % C;

        age0F[id] = reproR[id];
        hungry0[id] = hambreR[id];
        canReproF[id] = (age0F[id] >= genProcFoxes);

        int candRab[4], pr = 0;
        for (int d = 0; d < 4; ++d)
        {
            int nx = x + dx[d], ny = y + dy[d];
            if (nx < 0 || nx >= R || ny < 0 || ny >= C)
                continue;
            int nid = idxPos(nx, ny);
            if (tipoR[nid] == RABBIT)
                candRab[pr++] = nid;
        }

        if (pr > 0)
        {
            int ch = (gen + x + y) % pr;
            destF[id] = candRab[ch];
            moveF[id] = 1;
            aliveF[id] = 1;
            hunger1[id] = 0;
        }
        else
        {
            int nh = hungry0[id] + 1;
            if (nh >= genFoodFoxes)
            {
                aliveF[id] = 0;
                hunger1[id] = nh;
            }
            else
            {
                int candEmp[4], pe = 0;
                for (int d = 0; d < 4; ++d)
                {
                    int nx = x + dx[d], ny = y + dy[d];
                    if (nx < 0 || nx >= R || ny < 0 || ny >= C)
                        continue;
                    int nid = idxPos(nx, ny);
                    if (tipoR[nid] == EMPTY)
                        candEmp[pe++] = nid;
                }
                hunger1[id] = nh;
                if (pe > 0)
                {
                    int ch = (gen + x + y) % pe;
                    destF[id] = candEmp[ch];
                    moveF[id] = 1;
                    aliveF[id] = 1;
                }
                else
                {
                    aliveF[id] = 1;
                }
            }
        }
    }

#pragma omp parallel for
    for (int i = 0; i < gridSize; ++i)
    {
        bestAge[i] = -1;
        bestHunger[i] = 0;
        winner[i] = -1;
    }

    for (int id = 0; id < gridSize; ++id)
    {
        if (tipoR[id] != FOX || !aliveF[id] || !moveF[id])
            continue;
        int d = destF[id];
        int a = age0F[id];
        int h = hunger1[id];
        if (bestAge[d] == -1 ||
            a > bestAge[d] ||
            (a == bestAge[d] && h < bestHunger[d]))
        {
            bestAge[d] = a;
            bestHunger[d] = h;
            winner[d] = id;
        }
    }

    for (int d = 0; d < gridSize; ++d)
    {
        int id = winner[d];
        if (id == -1)
            continue;
        tipoNext[d] = FOX;
        reproNext[d] = age0F[id];
        hambreNext[d] = hunger1[id];
    }

    for (int id = 0; id < gridSize; ++id)
    {
        if (tipoR[id] != FOX || !aliveF[id] || moveF[id])
            continue;
        if (tipoNext[id] == EMPTY)
        {
            tipoNext[id] = FOX;
            reproNext[id] = age0F[id];
            hambreNext[id] = hunger1[id];
        }
    }

    nextAgeF.assign(gridSize, 0);

    for (int id = 0; id < gridSize; ++id)
    {
        if (tipoR[id] != FOX || !aliveF[id])
            continue;
        int pos;
        if (moveF[id])
        {
            int d = destF[id];
            if (winner[d] != id)
                continue;
            pos = d;
        }
        else
        {
            pos = id;
        }
        if (tipoNext[pos] != FOX)
            continue;
        bool reproduced = moveF[id] && canReproF[id] && winner[destF[id]] == id;
        nextAgeF[pos] = reproduced ? 0 : age0F[id] + 1;
    }

    for (int id = 0; id < gridSize; ++id)
    {
        if (tipoR[id] != FOX || !aliveF[id] || !moveF[id])
            continue;
        if (!canReproF[id])
            continue;
        if (winner[destF[id]] != id)
            continue;
        if (tipoNext[id] == EMPTY)
        {
            tipoNext[id] = FOX;
            reproNext[id] = 0;
            hambreNext[id] = 0;
        }
        nextAgeF[id] = 0;
    }

    for (int i = 0; i < gridSize; ++i)
    {
        if (tipoNext[i] == FOX)
            reproNext[i] = nextAgeF[i];
    }
}

void imprimirResultado()
{
    int finalN = 0;
    for (int i = 0; i < gridSize; ++i)
        if (tipo[i] != EMPTY)
            ++finalN;

    cout << genProcRabbits << ' '
         << genProcFoxes << ' '
         << genFoodFoxes << ' '
         << 0 << ' '
         << R << ' '
         << C << ' '
         << finalN << "\n";

    for (int x = 0; x < R; ++x)
    {
        for (int y = 0; y < C; ++y)
        {
            int id = idxPos(x, y);
            if (tipo[id] == ROCK)
                cout << "ROCK " << x << ' ' << y << "\n";
            if (tipo[id] == RABBIT)
                cout << "RABBIT " << x << ' ' << y << "\n";
            if (tipo[id] == FOX)
                cout << "FOX " << x << ' ' << y << "\n";
        }
    }
}

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    leerEntrada();

    for (int g = 0; g < nGen; ++g)
    {
        faseConejos(g);
        faseZorros(g);
        tipo.swap(tipoNext);
        repro.swap(reproNext);
        hambre.swap(hambreNext);
    }

    imprimirResultado();
    return 0;
}
