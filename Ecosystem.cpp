#include <bits/stdc++.h>
#include <omp.h>

using namespace std;

// Tipos de objeto en la celda
enum { EMPTY = 0, ROCK = 1, RABBIT = 2, FOX = 3 };

struct Cell {
    int type;      // EMPTY, ROCK, RABBIT, FOX
    int repro;     // contador de generaciones para reproducción
    int hunger;    // contador de hambre (solo zorros)
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES;
    int N_GEN, R, C, N;

    // Leemos la línea de configuración
    if (!(cin >> GEN_PROC_RABBITS >> GEN_PROC_FOXES >> GEN_FOOD_FOXES
              >> N_GEN >> R >> C >> N)) {
        return 0;
    }

    int size = R * C;
    vector<Cell> world(size);         // mundo actual
    vector<Cell> world_after_r(size); // mundo después de mover conejos
    vector<Cell> world_next(size);    // mundo después de mover zorros

    // Inicializamos todas las celdas como vacías
    for (int i = 0; i < size; ++i) {
        world[i].type = EMPTY;
        world[i].repro = 0;
        world[i].hunger = 0;
    }

    // Leemos objetos iniciales
    for (int i = 0; i < N; ++i) {
        string obj;
        int x, y;
        cin >> obj >> x >> y;
        int idx = x * C + y;
        if (obj == "ROCK") {
            world[idx].type = ROCK;
        } else if (obj == "RABBIT") {
            world[idx].type = RABBIT;
        } else if (obj == "FOX") {
            world[idx].type = FOX;
        }
    }

    // Desplazamientos N, E, S, O
    const int dx[4] = { -1, 0, 1, 0 };
    const int dy[4] = { 0, 1, 0, -1 };

    // Arrays auxiliares para conejos
    vector<char> move_r(size);        // si el conejo intenta moverse
    vector<int>  dest_r(size);        // destino (índice en el vector)
    vector<char> canRepro_r(size);    // si puede reproducirse
    vector<char> aliveEnd_r(size);    // si el conejo sigue vivo al final de su fase
    vector<int>  age0_r(size);        // edad de reproducción al inicio de la generación

    // Arrays auxiliares para zorros
    vector<char> move_f(size);
    vector<int>  dest_f(size);
    vector<char> canRepro_f(size);
    vector<char> aliveEnd_f(size);
    vector<int>  age0_f(size);
    vector<int>  hungry0(size);
    vector<int>  hunger1(size);       // hambre tras la decisión de esta generación

    // Arrays para resolución de conflictos
    vector<int> best_age(size);       // para conejos y zorros según la fase
    vector<int> best_hunger(size);    // solo zorros
    vector<int> winner(size);         // índice del origen ganador

    // Bucle principal de generaciones
    for (int gen = 0; gen < N_GEN; ++gen) {

        // ========================
        // FASE DE CONEJOS
        // ========================

        // Reseteo de arrays auxiliares de conejos
        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            move_r[i] = 0;
            dest_r[i] = -1;
            canRepro_r[i] = 0;
            aliveEnd_r[i] = 0;
            age0_r[i] = 0;
        }

        // Decisiones de movimiento de conejos
        #pragma omp parallel for
        for (int idx = 0; idx < size; ++idx) {
            if (world[idx].type != RABBIT) continue;

            int x = idx / C;
            int y = idx % C;

            age0_r[idx] = world[idx].repro;
            canRepro_r[idx] = (age0_r[idx] >= GEN_PROC_RABBITS);

            // Buscar celdas vacías adyacentes en el mundo actual
            int cand[4];
            int pc = 0;
            for (int d = 0; d < 4; ++d) {
                int nx = x + dx[d];
                int ny = y + dy[d];
                if (nx >= 0 && nx < R && ny >= 0 && ny < C) {
                    int nidx = nx * C + ny;
                    if (world[nidx].type == EMPTY) {
                        cand[pc++] = nidx;
                    }
                }
            }

            if (pc > 0) {
                int choice = (gen + x + y) % pc;
                dest_r[idx] = cand[choice];
                move_r[idx] = 1;
            } else {
                move_r[idx] = 0;
            }
        }

        // world_after_r: copiamos rocas y zorros del mundo actual
        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            world_after_r[i].type = EMPTY;
            world_after_r[i].repro = 0;
            world_after_r[i].hunger = 0;

            if (world[i].type == ROCK || world[i].type == FOX) {
                world_after_r[i] = world[i];
            }
        }

        // Resolución de conflictos de conejos en el destino
        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            best_age[i] = -1;
            winner[i] = -1;
        }

        for (int idx = 0; idx < size; ++idx) {
            if (world[idx].type != RABBIT || !move_r[idx]) continue;
            int d = dest_r[idx];
            int a = age0_r[idx];

            if (best_age[d] == -1 || a > best_age[d]) {
                if (best_age[d] != -1) {
                    int prev = winner[d];
                    if (prev >= 0) aliveEnd_r[prev] = 0;
                }
                best_age[d] = a;
                winner[d] = idx;
                aliveEnd_r[idx] = 1;

                world_after_r[d].type = RABBIT;
                world_after_r[d].repro = age0_r[idx];
                world_after_r[d].hunger = 0;
            }
        }

        // Conejos que se quedan en su sitio
        for (int idx = 0; idx < size; ++idx) {
            if (world[idx].type != RABBIT) continue;

            if (!move_r[idx]) {
                aliveEnd_r[idx] = 1;
                if (world_after_r[idx].type == EMPTY) {
                    world_after_r[idx].type = RABBIT;
                    world_after_r[idx].repro = age0_r[idx];
                    world_after_r[idx].hunger = 0;
                }
            }
        }

        // Nacimientos de conejos (padre se movió y podía reproducirse)
        for (int idx = 0; idx < size; ++idx) {
            if (world[idx].type != RABBIT) continue;
            if (!move_r[idx]) continue;
            if (!aliveEnd_r[idx]) continue;
            if (!canRepro_r[idx]) continue;

            if (world_after_r[idx].type == EMPTY) {
                world_after_r[idx].type = RABBIT;
                world_after_r[idx].repro = 0;
                world_after_r[idx].hunger = 0;
            }
        }

        // Actualización de edades de reproducción de conejos
        vector<int> next_age_r(size, 0);

        // Padres
        for (int idx = 0; idx < size; ++idx) {
            if (world[idx].type != RABBIT) continue;
            if (!aliveEnd_r[idx]) continue;

            int pos = move_r[idx] ? dest_r[idx] : idx;
            if (canRepro_r[idx]) {
                next_age_r[pos] = 0;
            } else {
                next_age_r[pos] = age0_r[idx] + 1;
            }
        }

        // Hijos
        for (int idx = 0; idx < size; ++idx) {
            if (world[idx].type != RABBIT) continue;
            if (!move_r[idx]) continue;
            if (!aliveEnd_r[idx]) continue;
            if (!canRepro_r[idx]) continue;

            next_age_r[idx] = 0;
        }

        for (int i = 0; i < size; ++i) {
            if (world_after_r[i].type == RABBIT) {
                world_after_r[i].repro = next_age_r[i];
            }
        }

        // ========================
        // FASE DE ZORROS
        // ========================

        // Reseteo de arrays auxiliares de zorros
        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            move_f[i]      = 0;
            dest_f[i]      = -1;
            canRepro_f[i]  = 0;
            aliveEnd_f[i]  = 0;
            age0_f[i]      = 0;
            hungry0[i]     = 0;
            hunger1[i]     = 0;
        }

        // world_next: copiamos rocas y conejos del mundo después de conejos
        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            world_next[i].type = EMPTY;
            world_next[i].repro = 0;
            world_next[i].hunger = 0;

            if (world_after_r[i].type == ROCK || world_after_r[i].type == RABBIT) {
                world_next[i] = world_after_r[i];
            }
        }

        // Decisiones de movimiento de zorros
        for (int idx = 0; idx < size; ++idx) {
            if (world_after_r[idx].type != FOX) continue;

            int x = idx / C;
            int y = idx % C;

            age0_f[idx] = world_after_r[idx].repro;
            hungry0[idx] = world_after_r[idx].hunger;
            canRepro_f[idx] = (age0_f[idx] >= GEN_PROC_FOXES);

            // Buscar conejos adyacentes
            int cand_rab[4];
            int pr = 0;
            for (int d = 0; d < 4; ++d) {
                int nx = x + dx[d];
                int ny = y + dy[d];
                if (nx >= 0 && nx < R && ny >= 0 && ny < C) {
                    int nidx = nx * C + ny;
                    if (world_after_r[nidx].type == RABBIT) {
                        cand_rab[pr++] = nidx;
                    }
                }
            }

            if (pr > 0) {
                int choice = (gen + x + y) % pr;
                dest_f[idx] = cand_rab[choice];
                move_f[idx] = 1;
                aliveEnd_f[idx] = 1;
                hunger1[idx] = 0; // comió conejo
            } else {
                int nh = hungry0[idx] + 1;
                if (nh >= GEN_FOOD_FOXES) {
                    move_f[idx] = 0;
                    aliveEnd_f[idx] = 0; // muere de hambre
                    hunger1[idx] = nh;
                } else {
                    int cand_emp[4];
                    int pe = 0;
                    for (int d = 0; d < 4; ++d) {
                        int nx = x + dx[d];
                        int ny = y + dy[d];
                        if (nx >= 0 && nx < R && ny >= 0 && ny < C) {
                            int nidx = nx * C + ny;
                            if (world_after_r[nidx].type == EMPTY) {
                                cand_emp[pe++] = nidx;
                            }
                        }
                    }

                    hunger1[idx] = nh;
                    if (pe > 0) {
                        int choice = (gen + x + y) % pe;
                        dest_f[idx] = cand_emp[choice];
                        move_f[idx] = 1;
                        aliveEnd_f[idx] = 1;
                    } else {
                        // se queda en su sitio, sigue vivo, hambre aumentada
                        move_f[idx] = 0;
                        aliveEnd_f[idx] = 1;
                    }
                }
            }
        }

        // Resolución de conflictos de zorros en el destino
        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            best_age[i] = -1;
            best_hunger[i] = 0;
            winner[i] = -1;
        }

        for (int idx = 0; idx < size; ++idx) {
            if (world_after_r[idx].type != FOX) continue;
            if (!aliveEnd_f[idx]) continue;
            if (!move_f[idx]) continue;

            int d = dest_f[idx];
            int a = age0_f[idx];
            int h = hunger1[idx];

            if (best_age[d] == -1) {
                best_age[d] = a;
                best_hunger[d] = h;
                winner[d] = idx;
            } else {
                if (a > best_age[d] ||
                    (a == best_age[d] && h < best_hunger[d])) {
                    best_age[d] = a;
                    best_hunger[d] = h;
                    winner[d] = idx;
                }
            }
        }

        // Colocamos zorros que se movieron y ganaron el conflicto
        for (int d = 0; d < size; ++d) {
            int idx = winner[d];
            if (idx == -1) continue;

            // Si el destino tenía un conejo, el zorro se lo come (el conejo desaparece)
            if (world_next[d].type == RABBIT) {
                world_next[d].type = EMPTY;
            }

            world_next[d].type = FOX;
            world_next[d].repro = age0_f[idx];
            world_next[d].hunger = hunger1[idx];
        }


        // Zorros que se quedan en su sitio
        for (int idx = 0; idx < size; ++idx) {
            if (world_after_r[idx].type != FOX) continue;
            if (!aliveEnd_f[idx]) continue;
            if (move_f[idx]) continue; // los que se movieron ya se gestionaron como ganadores o muertos

            if (world_next[idx].type == EMPTY) {
                world_next[idx].type = FOX;
                world_next[idx].repro = age0_f[idx];   // se ajustará luego
                world_next[idx].hunger = hunger1[idx];
            }
        }

        // Calculamos edades finales de reproducción de zorros
        vector<int> next_age_f(size, 0);

        // Padres en su posición final
        for (int idx = 0; idx < size; ++idx) {
            if (world_after_r[idx].type != FOX) continue;
            if (!aliveEnd_f[idx]) continue;

            bool moved = move_f[idx];
            int pos;

            if (moved) {
                int d = dest_f[idx];
                // Si no ganó el conflicto, este zorro no existe al final
                if (winner[d] != idx) continue;
                pos = d;
            } else {
                pos = idx;
            }

            if (world_next[pos].type != FOX) continue;

            bool reproduced = false;
            if (moved && canRepro_f[idx] && winner[dest_f[idx]] == idx) {
                reproduced = true;
            }

            if (reproduced) {
                next_age_f[pos] = 0;
            } else {
                next_age_f[pos] = age0_f[idx] + 1;
            }
        }

        // Nacimientos de zorros (solo si el zorro se movió, tenía edad y ganó su destino)
        for (int idx = 0; idx < size; ++idx) {
            if (world_after_r[idx].type != FOX) continue;
            if (!aliveEnd_f[idx]) continue;       // murió de hambre
            if (!move_f[idx]) continue;           // no se movió
            if (!canRepro_f[idx]) continue;       // no alcanzó edad
            if (winner[dest_f[idx]] != idx) continue; // perdió conflicto


            if (world_next[idx].type == EMPTY) {
                world_next[idx].type = FOX;
                world_next[idx].repro = 0; // se fijará ahora a 0 igual
                world_next[idx].hunger = 0;
            }
            next_age_f[idx] = 0;
        }

        // Aplicamos las edades finales de los zorros
        for (int i = 0; i < size; ++i) {
            if (world_next[i].type == FOX) {
                world_next[i].repro = next_age_f[i];
            }
        }

        // Intercambiamos mundos: el nuevo estado pasa a ser el actual
        world.swap(world_next);
    }

    // Contamos objetos finales
    int finalN = 0;
    for (int i = 0; i < size; ++i) {
        if (world[i].type != EMPTY) ++finalN;
    }

    // Imprimimos cabecera final (N_GEN pasa a 0 en la salida, como en el ejemplo)
    cout << GEN_PROC_RABBITS << ' '
         << GEN_PROC_FOXES << ' '
         << GEN_FOOD_FOXES << ' '
         << 0 << ' '
         << R << ' '
         << C << ' '
         << finalN << "\n";

    // Imprimimos objetos finales en el formato pedido
    for (int x = 0; x < R; ++x) {
        for (int y = 0; y < C; ++y) {
            int idx = x * C + y;
            if (world[idx].type == ROCK) {
                cout << "ROCK " << x << ' ' << y << "\n";
            } else if (world[idx].type == RABBIT) {
                cout << "RABBIT " << x << ' ' << y << "\n";
            } else if (world[idx].type == FOX) {
                cout << "FOX " << x << ' ' << y << "\n";
            }
        }
    }

    return 0;
}