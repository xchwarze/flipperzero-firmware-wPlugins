// utils.c
#include <stdlib.h>
#include "utils.h"

bool perform_action_with_chance(int chance_percent) {
    if(chance_percent < 0 || chance_percent > 100) {
        return false; // Вероятность должна быть в диапазоне 0-100%
    }

    int random_value = rand() % 100; // Генерируем случайное число от 0 до 99
    return random_value < chance_percent;
}
