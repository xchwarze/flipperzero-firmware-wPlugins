#include <stdio.h>
#include <furi.h>
#include <gui/gui.h>
#include <furi_hal.h>
#include <stdlib.h>
#include "utils.h"

#define BACKLIGHT_ON 1

// Определение структуры клетки
typedef struct {
    int32_t x, y; // Позиция клетки
    int32_t energy; // Уровень энергии
    int32_t dna;
} Cell;

// Глобальные переменные
int32_t max_cells = 2000;
Cell cells[2000]; // Начальный массив на 1000 клеток
int32_t cell_count = 15; // Начальное количество клеток

bool is_position_occupied(int32_t x, int32_t y) {
    for(int32_t i = 0; i < cell_count; i++) {
        if(cells[i].x == x && cells[i].y == y) {
            return true; // Позиция занята
        }
    }
    return false; // Позиция свободна
}

void move_cell(int32_t i) {
    int32_t move_x = ((cells[i].dna) % 3) - 1; // -1, 0 или 1
    int32_t move_y = ((cells[i].dna) % 3) - 1; // -1, 0 или 1

    // Обновление позиции с проверкой границ сетки
    int32_t new_x = cells[i].x + move_x;
    int32_t new_y = cells[i].y + move_y;

    // Ограничиваем координаты в пределах сетки
    new_x = (new_x < 0) ? 0 : (new_x > 63) ? 63 : new_x; // 32 - 1
    new_y = (new_y < 0) ? 0 : (new_y > 63) ? 63 : new_y; // 16 - 1

    // Проверка на занятость новой позиции
    if(!is_position_occupied(new_x, new_y)) {
        cells[i].energy -= 1;
        cells[i].x = new_x;
        cells[i].y = new_y;
    }
}

// Основная логика
static void logic_game() {
    for(int32_t i = 0; i < cell_count; i++) {
        // Проверка на границы квадрата и уменьшение энергии
        if(cells[i].x < 0 || cells[i].x >= 64 || cells[i].y < 0 || cells[i].y >= 64) {
            cells[i].energy =
                0; // Если клетка выходит за границы, она теряет энергию (можно задать другие правила)
        }
        cells[i].energy -= 1;
        // Проверка на смерть клетки
        if(cells[i].energy <= 0) {
            // Удаление клетки
            for(int j = i; j < cell_count - 1; j++) {
                cells[j] = cells[j + 1]; // Сдвиг всех клеток влево
            }
            cell_count--; // Уменьшаем количество клеток
            i--; // Возвращаемся назад, чтобы проверить новую клетку на этой позиции
            continue; // Переходим к следующей итерации цикла
        }

        // Фотосинтез: добавляем энергию в зависимости от зоны
        cells[i].energy += (64 - cells[i].y) % 10;
        move_cell(i);

        // Если энергии достаточно, клетка делится
        if(cells[i].energy > 100 && cell_count < max_cells) {
            int32_t new_x = cells[i].x + (rand() % 3 - 1);
            int32_t new_y = cells[i].y + (rand() % 3 - 1);
            int32_t new_dna;

            if(perform_action_with_chance(10)) {
                new_dna = (cells[i].dna + rand()) % 64;
            } else {
                new_dna = cells[i].dna;
            }

            // Проверка на занятость позиции
            if(!is_position_occupied(new_x, new_y)) {
                cells[cell_count++] =
                    (Cell){.x = new_x, .y = new_y, .energy = cells[i].energy / 2, .dna = new_dna};
                cells[i].energy /= 2;
            } else {
                // Если позиция занята, клетка умирает
                cells[i].energy = 0;
            }
        }
    }
}

// Функция отрисовки на экране
static void draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
    logic_game();

    for(int32_t i = 0; i < cell_count; i++) {
        // Рисуем клетку на экране
        canvas_draw_dot(canvas, cells[i].x, cells[i].y);
    }
    // Устанавливаем шрифт для отображения количества клеток
    canvas_set_font(canvas, FontSecondary);

    // Создаем буфер для хранения строки с количеством клеток
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "Cells: %ld", cell_count);

    // Рисуем строку на экране
    canvas_draw_str(canvas, 80, 8, buffer);
}

// Функция обработки ввода для выхода из эффекта
static void input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

// Основная функция приложения
int32_t cells_app_main(void* p) {
    UNUSED(p);

    // Текущий элемент события типа InputEvent
    InputEvent event;

    // Очередь событий
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    // Инициализация пяти клеток с случайными координатами и начальными значениями энергии
    for(int i = 0; i < cell_count; i++) {
        cells[i] = (Cell){
            .x = rand() % 32,
            .y = rand() % 16,
            .energy = rand() % 100,
            .dna = rand() % 64}; // Случайные начальные координаты и энергия
    }

    // Создаем ViewPort для отображения анимации
    ViewPort* view_port = view_port_alloc();

    // Устанавливаем callback для отрисовки эффекта
    view_port_draw_callback_set(view_port, draw_callback, NULL);

    // Устанавливаем callback для обработки ввода
    view_port_input_callback_set(view_port, input_callback, event_queue);

    // Настройка GUI и добавление ViewPort на весь экран
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Основной цикл эффекта
    while(true) {
        // Проверка событий в очереди
        furi_check(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk);

        // Прерывание цикла при нажатии кнопки "назад"
        if(event.key == InputKeyBack) {
            break;
        }

        // Пауза для создания плавности эффекта
        furi_delay_ms(1000);
    }

    // Очистка ресурсов
    furi_message_queue_free(event_queue);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    return 0;
}
