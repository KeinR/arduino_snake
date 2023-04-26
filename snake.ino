#include "OLED_Driver.h"
#include "GUI_paint.h"
#include "DEV_Config.h"
#include "Debug.h"
#include "ImageData.h"

#define SCR_WIDTH 400
#define SCR_HEIGHT 400
#define SPAWN_MARGIN 10
#define GRID_WIDTH 40
#define GRID_HEIGHT 40
#define FRAME_DELAY 500

#define I_UP    2
#define I_DOWN  3
#define I_LEFT  4
#define I_RIGHT 5

typedef struct {
    int x;
    int y;
} point_t;

typedef struct {
    point_t units[128];
    int len;
    int targetLen;
} tail_t;

typedef struct {
    tail_t tail;
    int vx;
    int vy;
    int cx;
    int cy;
    unsigned long nextRun;
    bool gameOver;
} program_t;

void moveTail(tail_t *tail, int x, int y) {
    if (tail->len < tail->targetLen) {
        tail->units[tail->len++] = (point_t){x, y};
    } else {
        for (int i = 1; i < tail->len; i++) {
            tail->units[i-1] = tail->units[i];
        }
    }
}

void addTail(tail_t *tail, int n) {
    tail->targetLen += n;
    Serial.print("Add tail length ");
    Serial.print(n);
    Serial.println("");
}

tail_t mkTail(int x, int y) {
    tail_t result;
    result.len = 1;
    result.targetLen = 1;
    result.units[0] = (point_t) {x, y};
    return result;
}

void putCherry(program_t *p) {
    Serial.println("Put cherry");
    p->cx = random(0, GRID_WIDTH);
    p->cy = random(0, GRID_HEIGHT);
}

program_t mkProgram(int startX, int startY) {
    program_t p;
    p.tail = mkTail(random(SPAWN_MARGIN, GRID_WIDTH), random(SPAWN_MARGIN, GRID_HEIGHT));
    p.vx = 0;
    p.vy = 0;
    p.lastRun = 0;
    p.gameOver = false;
    putCherry(&p);
    return p;
}

void recordCherry(program_t *p) {
    addTail(&p->tail, 1);
}

void testCherry(program_t *p) {
    point_t head = p->tail.units[p->tail.len - 1];
    if (head.x == p->cx && head.y == p->cy) {
        recordCherry(p);
        putCherry(p);
    }
}

void renderBox(int x, int y) {
    int cx = (int)(SCR_WIDTH * ((float)x / GRID_WIDTH));
    int cy = (int)(SCR_HEIGHT * ((float)y / GRID_HEIGHT));
    int width = SCR_WIDTH / GRID_WIDTH;
    int height = SCR_HEIGHT / GRID_HEIGHT;
    Paint_DrawRectangle(cx, cy, width, height, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

void renderTail(tail_t *t) {
    for (int i = 0; i < t->len; i++) {
        renderBox(t->units[i].x, t->units[i].y);
    }
}

void prepareRender() {
    OLED_1in5_rgb_Clear();  
}

void render(program_t *p) {
    prepareRender();
    renderTail(&p->tail);
    renderBox(p->cx, p->cy);
}

bool nextRun(program_t *p) {
    unsigned long now = millis();
    if (p->nextRun <= now) {
        p->nextRun = now + FRAME_DELAY;
        return true;
    } else {
        return false;
    }
}

bool moveSnake(program_t *p) {
    point_t head = p->tail.units[p->tail.len - 1];
    head.x += p->vx;
    head.y += p->vy;
    return head.x >= 0 && head.x < GRID_WIDTH && head.y >= 0 && head.y < GRID_HEIGHT;
}

void getInput(program_t *p) {
    if (digitalRead(I_UP) == HIGH) {
        p->vy = -1;
        p->vx = 0;
    }
    if (digitalRead(I_DOWN) == HIGH) {
        p->vy = 1;
        p->vx = 0;
    }
    if (digitalRead(I_LEFT) == HIGH) {
        p->vy = 0;
        p->vx = -1;
    }
    if (digitalRead(I_RIGHT) == HIGH) {
        p->vy = 0;
        p->vx = 1;
    }

}

void gameOver(program_t *p) {
    p->gameOver = true;
}

void runProgram(program_t *p) {
    if (!p->gameOver && nextRun(p)) {
        getInput(p);
        if (moveSnake(p)) {
            testCherry(p);
        } else {
            gameOver(p);
        }
        render(p);
    }
}

program_t program;

void setup() {
    randomSeed(analogRead(0));

    pinMode(I_UP, INPUT);
    pinMode(I_DOWN, INPUT);
    pinMode(I_LEFT, INPUT);
    pinMode(I_RIGHT, INPUT);

    System_Init();
    Serial.begin(9600);
    if(USE_IIC) {
        Serial.println("Error: Only USE_SPI_4W, Please revise DEV_config.h !!!"); // ??????????
        return 0;
    }

    Serial.println("Initializing...");
    OLED_1in5_rgb_Init();
    //Driver_Delay_ms(500); 
    OLED_1in5_rgb_Clear();  

    program = mkProgram();
    Serial.println("Initialized.");
}

void loop() {
    runProgram(&program);
}
