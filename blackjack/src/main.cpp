#include "GameEngine.h"
#include "Types.h"

int main() {
    GameEngine engine(1, 1, STARTING_CHIPS);
    engine.run();
    return 0;
}
